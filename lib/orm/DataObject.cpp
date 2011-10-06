// -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#include <orm/DataObject.h>
#include <algorithm>
#include <iostream>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <util/str_utils.hpp>

using namespace std;

namespace Yb {

SessionV2::~SessionV2()
{
    Objects objects_copy = objects_;
#if 0
    using namespace boost::lambda;
    for_each(objects_copy.begin(), objects_copy.end(),
             bind(&SessionV2::detach, this, _1));
#else
    Objects::iterator i = objects_copy.begin(), iend = objects_copy.end();
    for (; i != iend; ++i)
        detach(*i);
#endif
}

void SessionV2::add_to_identity_map(DataObjectPtr obj)
{
    if (obj->assigned_key()) {
        const Key &key = obj->key();
        IdentityMap::iterator i = identity_map_.find(key);
        if (i != identity_map_.end())
            return;
        identity_map_[key] = obj;
    }
}

void SessionV2::save(DataObjectPtr obj)
{
    add_to_identity_map(obj);
    Objects::iterator i = objects_.find(obj);
    if (i == objects_.end()) {
        objects_.insert(obj);
        obj->set_session(this);
    }
}

void SessionV2::detach(DataObjectPtr obj)
{
    if (obj->assigned_key()) {
        const Key &key = obj->key();
        IdentityMap::iterator i = identity_map_.find(key);
        if (i != identity_map_.end())
            identity_map_.erase(i);
    }
    Objects::iterator i = objects_.find(obj);
    if (i != objects_.end()) {
        objects_.erase(i);
        obj->forget_session();
    }
}

DataObject::Ptr SessionV2::get_lazy(const Key &key)
{
    IdentityMap::iterator i = identity_map_.find(key);
    if (i != identity_map_.end())
        return i->second;
    DataObjectPtr new_obj =
        DataObject::create_new(schema_->get_table(key.first),
                               DataObject::Ghost);
    ValueMap::const_iterator it = key.second.begin(),
        end = key.second.end();
    for (; it != end; ++it)
        new_obj->set(it->first, it->second);
    new_obj->set_session(this);
    identity_map_[key] = new_obj;
    return new_obj;
}

typedef map<string, Rows> RowsByTable;

void add_row_to_rows_by_table(RowsByTable &rows_by_table,
                              const string &tbl_name, const ValueMap &row)
{
    RowsByTable::iterator k = rows_by_table.find(tbl_name);
    if (k == rows_by_table.end()) {
        Rows rows(1);
        rows[0] = row;
        rows_by_table.insert(pair<string, Rows> (tbl_name, rows));
    }
    else {
        Rows &rows = k->second;
        rows.push_back(row);
    }
}

void SessionV2::flush_tbl_new_keyed(const Table &tbl, Objects &keyed_objs)
{
    Rows rows;
    Objects::iterator i, iend = keyed_objs.end();
    for (i = keyed_objs.begin(); i != iend; ++i)
        rows.push_back((*i)->values());
    engine_->insert(tbl.get_name(), rows, StringSet());
}

void SessionV2::flush_tbl_new_unkeyed(const Table &tbl, Objects &unkeyed_objs)
{
    bool sql_seq = engine_->get_dialect()->has_sequences();
    bool use_seq = sql_seq && !tbl.get_seq_name().empty();
    bool use_autoinc = !sql_seq &&
        (tbl.get_autoinc() || !tbl.get_seq_name().empty());
    Objects::iterator i, iend = unkeyed_objs.end();
    if (use_seq) {
        string pk = tbl.get_synth_pk();
        for (i = unkeyed_objs.begin(); i != iend; ++i)
            (*i)->set(pk, engine_->get_next_value(tbl.get_seq_name()));
    }
    Rows rows;
    for (i = unkeyed_objs.begin(); i != iend; ++i)
        rows.push_back((*i)->values(!use_autoinc));
    vector<LongInt> ids = engine_->insert
        (tbl.get_name(), rows, StringSet(), use_autoinc);
    if (use_autoinc) {
        string pk = tbl.get_synth_pk();
        i = unkeyed_objs.begin();
        for (int p = 0; i != iend; ++i, ++p)
            (*i)->set(pk, ids[p]);
    }
    // Refresh fk values for all slaves and
    // add flushed objects to the identity_map_
    for (i = unkeyed_objs.begin(); i != iend; ++i) {
        (*i)->refresh_slaves_fkeys();
        add_to_identity_map(*i);
    }
}

void SessionV2::flush_new()
{
    SqlDialect *dialect = engine_->get_dialect();
    bool sql_seq = dialect->has_sequences();
    Objects::iterator i = objects_.begin(), iend = objects_.end();
    for (; i != iend; ++i)
        if ((*i)->status() == DataObject::New)
            (*i)->depth(-1);
    for (i = objects_.begin(); i != iend; ++i)
        if ((*i)->status() == DataObject::New)
            (*i)->calc_depth(0);
    int max_depth = -1;
    typedef map<string, Objects> ObjectsByTable;
    typedef map<int, ObjectsByTable> GroupsByDepth;
    GroupsByDepth groups_by_depth;
    for (i = objects_.begin(); i != iend; ++i)
        if ((*i)->status() == DataObject::New)
    {
        int d = (*i)->depth();
        if (d > max_depth)
            max_depth = d;
        GroupsByDepth::iterator k = groups_by_depth.find(d);
        if (k == groups_by_depth.end()) {
            groups_by_depth.insert(pair<int, ObjectsByTable> 
                                   (d, ObjectsByTable()));
            k = groups_by_depth.find(d);
        }
        ObjectsByTable &objs_by_table = k->second;
        const string &tbl_name = (*i)->table().get_name();
        ObjectsByTable::iterator q = objs_by_table.find(tbl_name);
        if (q == objs_by_table.end()) {
            objs_by_table.insert(pair<string, Objects>
                                 (tbl_name, Objects()));
            q = objs_by_table.find(tbl_name);
        }
        Objects &objs = q->second;
        objs.insert(*i);
    }
    for (int d = 0; d <= max_depth; ++d) {
        GroupsByDepth::iterator k = groups_by_depth.find(d);
        if (k == groups_by_depth.end())
            continue;
        ObjectsByTable &objs_by_table = k->second;
        ObjectsByTable::iterator j = objs_by_table.begin(), jend = objs_by_table.end();
        for (; j != jend; ++j) {
            const string &tbl_name = j->first;
            const Table &tbl = schema_->get_table(tbl_name);
            Objects &objs = j->second, unkeyed_objs, keyed_objs;
            Objects::iterator l, lend = objs.end();
            for (l = objs.begin(); l != lend; ++l) {
                if ((*l)->assigned_key())
                    keyed_objs.insert(*l);
                else
                    unkeyed_objs.insert(*l);
            }
            flush_tbl_new_keyed(tbl, keyed_objs);
            flush_tbl_new_unkeyed(tbl, unkeyed_objs);
        }
    }
    for (i = objects_.begin(); i != iend; ++i)
        if ((*i)->status() == DataObject::New)
            (*i)->set_status(DataObject::Ghost);
}

void SessionV2::flush_update(IdentityMap &idmap_copy)
{
    typedef map<string, Rows> RowsByTable;
    RowsByTable rows_by_table;
    IdentityMap::iterator i = idmap_copy.begin(), iend = idmap_copy.end();
    for (; i != iend; ++i)
        if (i->second->status() == DataObject::Dirty) {
            const string &tbl_name = i->first.first;
            add_row_to_rows_by_table(rows_by_table, tbl_name,
                                     i->second->values());
            i->second->set_status(DataObject::Ghost);
        }
    RowsByTable::iterator j = rows_by_table.begin(),
        jend = rows_by_table.end();
    for (; j != jend; ++j) {
        engine_->update(j->first, j->second,
                        schema_->get_table(j->first).pk_fields());
    }
}

void SessionV2::flush_delete(IdentityMap &idmap_copy)
{
    typedef vector<Key> Keys;
    typedef map<string, Keys> KeysByTable;
    typedef map<int, KeysByTable> GroupsByDepth;
    int max_depth = -1;
    GroupsByDepth groups_by_depth;
    IdentityMap::iterator i = idmap_copy.begin(), iend = idmap_copy.end();
    for (; i != iend; ++i)
        if (i->second->status() == DataObject::ToBeDeleted)
    {
        int d = i->second->depth();
        if (d > max_depth)
            max_depth = d;
        GroupsByDepth::iterator k = groups_by_depth.find(d);
        if (k == groups_by_depth.end()) {
            groups_by_depth.insert(pair<int, KeysByTable> 
                                   (d, KeysByTable()));
            k = groups_by_depth.find(d);
        }
        KeysByTable &keys_by_table = k->second;
        const string &tbl_name = i->second->table().get_name();
        KeysByTable::iterator q = keys_by_table.find(tbl_name);
        if (q == keys_by_table.end()) {
            keys_by_table.insert(pair<string, Keys> (tbl_name, Keys()));
            q = keys_by_table.find(tbl_name);
        }
        Keys &keys = q->second;
        keys.push_back(i->first);
        i->second->set_status(DataObject::Deleted);
    }
    for (int d = max_depth; d >= 0; --d) {
        GroupsByDepth::iterator k = groups_by_depth.find(d);
        if (k == groups_by_depth.end())
            continue;
        KeysByTable &keys_by_table = k->second;
        KeysByTable::iterator j = keys_by_table.begin(),
            jend = keys_by_table.end();
        for (; j != jend; ++j)
            engine_->delete_from(j->first, j->second);
    }
}

void SessionV2::flush()
{
    IdentityMap idmap_copy = identity_map_;
    flush_new();
    flush_update(idmap_copy);
    flush_delete(idmap_copy);
    // Delete the deleted objects
    Objects obj_copy = objects_;
    Objects::iterator i = obj_copy.begin(), iend = obj_copy.end();
    for (; i != iend; ++i)
        if ((*i)->status() == DataObject::Deleted) {
            IdentityMap::iterator k = identity_map_.find((*i)->key());
            if (k != identity_map_.end())
                identity_map_.erase(k);
            objects_.erase(objects_.find(*i));
        }
}

void DataObject::set_session(SessionV2 *session)
{
    YB_ASSERT(!session_);
    session_ = session;
}

void DataObject::forget_session()
{
    YB_ASSERT(session_);
    session_ = NULL;
}

void DataObject::set(int i, const Value &v)
{
    const Column &c = table_.get_column(i);
    lazy_load(c);
    if (c.is_pk() && session_ != NULL
        && values_[i] != v && !values_[i].is_null())
    {
        throw ReadOnlyColumn(table_.get_name(), c.get_name());
    }
    values_[i] = v;
    if (c.is_pk()) {
        update_key();
    }
    else {
        if (status_ == Sync)
            status_ = Dirty;
    }
}

void DataObject::update_key()
{
    key_.first = table_.get_name();
    assigned_key_ = true;
    ValueMap new_values;
    for (size_t i = 0; i < table_.size(); ++i) {
        const Column &c = table_.get_column(i);
        if (c.is_pk()) {
            new_values[c.get_name()] = values_[i];
            if (values_[i].is_null())
                assigned_key_ = false;
        }
    }
    key_.second.swap(new_values);
}

const Key &DataObject::key()
{
    if (key_.first.empty())
        update_key();
    return key_;
}

const ValueMap DataObject::values(bool include_key)
{
    ValueMap values;
    for (size_t i = 0; i < table_.size(); ++i) {
        const Column &c = table_.get_column(i);
        if (!c.is_pk() || include_key)
            values.insert(pair<string, Value> (c.get_name(), values_[i]));
    }
    return values;
}

bool DataObject::assigned_key()
{
    if (key_.first.empty())
        update_key();
    return assigned_key_;
}

void DataObject::load()
{
    YB_ASSERT(session_ != NULL);
    KeyFilter f(key());
    Strings cols;
    Columns::const_iterator it = table_.begin(), end = table_.end();
    for (; it != end; ++it)
        cols.push_back(it->get_name());
    RowsPtr result = session_->engine()->select
        (StrList(cols), table_.get_name(), f);
    if (result->size() != 1)
        throw ObjectNotFoundByKey(table_.get_name() + "("
                                  + f.get_sql() + ")");
    fill_from_row(*result->begin());
}

void DataObject::calc_depth(int d, DataObject *parent)
{
    if (d > depth_) {
        depth_ = d;
        MasterRelations::iterator i = master_relations_.begin(),
            iend = master_relations_.end();
        for (; i != iend; ++i)
            (*i)->calc_depth(d + 1, parent);
    }
}

void DataObject::link(DataObject *master, DataObject::Ptr slave,
                      const Relation &r)
{
    // Try to find relation object in master's relations
    RelationObject *ro = NULL;
    MasterRelations::iterator j = master->master_relations().begin(),
        jend = master->master_relations().end();
    for (; j != jend; ++j)
        if ((*j)->relation_info() == r) {
            ro = (*j).get();
            break;
        }
    // Create one if it doesn't exist, master will own it
    if (!ro) {
        RelationObject::Ptr new_ro = RelationObject::create_new(r, master);
        master->master_relations().insert(new_ro);
        ro = new_ro.get();
    }
    // Register slave in the relation
    ro->slave_objects().insert(slave);
    slave->slave_relations().insert(ro);
    slave->calc_depth(master->depth() + 1, master);
}

void DataObject::link(DataObject *master, DataObject::Ptr slave,
                      const string &relation_name, int mode)
{
    // Find relation in metadata.
    Schema &schema(master->table().schema());
    const Relation *r = schema.find_relation
        (master->table().get_class(), relation_name,
         slave->table().get_class(), mode);
    link(master, slave, *r);
}

DataObject::Ptr DataObject::get_master(
    DataObject::Ptr obj, const std::string &relation_name)
{
    YB_ASSERT(obj.get());
    YB_ASSERT(obj->session_);
    // Find relation in metadata.
    Schema &schema(obj->table_.schema());
    const Relation *r = schema.find_relation
        (obj->table_.get_class(), relation_name, "", 1);
    YB_ASSERT(r != NULL);
    // Find FK value.
    string fk = obj->table_.get_fk_for(r);
    Strings parts;
    StrUtils::split_str(fk, ",", parts);
    ValueMap fk_values;
    Strings::iterator i = parts.begin(),
        end = parts.end();
    const Table &master_tbl = schema.get_table(r->table(0));
    // TODO: support compound foreign keys
    //for (; i != end; ++i)
    //    fk_values[*i] = get(*i);
    fk_values[master_tbl.get_unique_pk()] = obj->get(*i);
    Key fkey(master_tbl.get_name(), fk_values);
    DataObject::Ptr master = obj->session_->get_lazy(fkey);
    link(master.get(), obj, *r);
    return master;
}

RelationObject *DataObject::get_slaves(
    const string &relation_name)
{
    YB_ASSERT(session_);
    // Find relation in metadata.
    Schema &schema(table_.schema());
    const Relation *r = schema.find_relation
        (table_.get_class(), relation_name, "", 0);
    YB_ASSERT(r != NULL);
    // Try to find relation object in master's relations
    RelationObject *ro = NULL;
    MasterRelations::iterator j = master_relations_.begin(),
        jend = master_relations_.end();
    for (; j != jend; ++j)
        if ((*j)->relation_info() == *r) {
            ro = (*j).get();
            break;
        }
    // Create one if it doesn't exist, master will own it
    if (!ro) {
        RelationObject::Ptr new_ro = RelationObject::create_new(*r, this);
        master_relations_.insert(new_ro);
        ro = new_ro.get();
    }
    return ro;
}

DataObject::~DataObject()
{}

void DataObject::fill_from_row(const Row &r)
{
    status_ = Sync;
    Row::const_iterator i = r.begin(), iend = r.end();
    for (; i != iend; ++i) {
        set(i->first, i->second);
    }
    status_ = Sync;
}

void DataObject::refresh_slaves_fkeys()
{
    MasterRelations::iterator i = master_relations_.begin(),
        iend = master_relations_.end();
    for (; i != iend; ++i)
        (*i)->refresh_slaves_fkeys();
}

void DataObject::delete_object(DeletionMode mode, int depth)
{
    if (mode != DelUnchecked)
        delete_master_relations(DelDryRun, depth + 1);
    if (mode != DelDryRun) {
        delete_master_relations(DelUnchecked, depth + 1);
        exclude_from_slave_relations();
        if (status_ == New) {
            status_ = Deleted;
        }
        else {
            depth_ = depth;
            status_ = ToBeDeleted;
        }
    }
}

void DataObject::delete_master_relations(DeletionMode mode, int depth)
{
    MasterRelations::iterator i = master_relations_.begin(),
        iend = master_relations_.end();
    for (; i != iend; ++i)
        (*i)->delete_master(mode, depth);
    if (mode != DelDryRun) {
        MasterRelations empty;
        master_relations_.swap(empty);
    }
}

void DataObject::exclude_from_slave_relations()
{
    SlaveRelations::iterator i = slave_relations_.begin(),
        iend = slave_relations_.end();
    for (; i != iend; ++i)
        (*i)->exclude_slave(this);
}

void DataObject::set_free_from(RelationObject *rel)
{
    string fk = table_.get_fk_for(&rel->relation_info());
    Strings parts;
    StrUtils::split_str(fk, ",", parts);
    Strings::iterator i = parts.begin(), end = parts.end();
    for (; i != end; ++i)
        set(*i, Value());
}

void RelationObject::calc_depth(int d, DataObject *parent)
{
    SlaveObjects::iterator i = slave_objects_.begin(),
        iend = slave_objects_.end();
    for (; i != iend; ++i) {
        if (parent && i->get() == parent)
            throw CycleDetected();
        (*i)->calc_depth(d, parent);
    }
}

void RelationObject::delete_master(DeletionMode mode, int depth)
{
    if (relation_info_.cascade() == Relation::Nullify) {
        if (mode != DelDryRun) {
            SlaveObjects::iterator i = slave_objects_.begin(),
                iend = slave_objects_.end();
            for (; i != iend; ++i)
                (*i)->set_free_from(this);
            SlaveObjects empty;
            slave_objects_.swap(empty);
        }
    }
    else if (relation_info_.cascade() == Relation::Delete) {
        SlaveObjects slaves_copy = slave_objects_;
        SlaveObjects::iterator i = slaves_copy.begin(),
            iend = slaves_copy.end();
        for (; i != iend; ++i)
            (*i)->delete_object(mode, depth);
        if (mode != DelDryRun)
            YB_ASSERT(!slave_objects_.size());
    }
    else {
        if (slave_objects_.size() != 0)
            throw CascadeDeleteError(relation_info_);
    }
}

const Key RelationObject::gen_fkey() const
{
    Schema &schema = master_object_->table().schema();
    const Table &master_tbl = schema.get_table(relation_info_.table(0)),
        &slave_tbl = schema.get_table(relation_info_.table(1));
    Strings parts;
    StrUtils::split_str(slave_tbl.get_fk_for(&relation_info_), ",", parts);
    // TODO: support compound foreign keys
    ValueMap fk_values;
    fk_values[*parts.begin()] = master_object_->get(master_tbl.get_unique_pk());
    return Key(slave_tbl.get_name(), fk_values);
}

size_t RelationObject::count_slaves()
{
    if (status_ == Sync)
        return slave_objects_.size();
    YB_ASSERT(master_object_->session());
    SessionV2 &session = *master_object_->session();
    Schema &schema = master_object_->table().schema();
    const Table &master_tbl = schema.get_table(relation_info_.table(0)),
        &slave_tbl = schema.get_table(relation_info_.table(1));
    KeyFilter f(gen_fkey());
    Strings cols;
    cols.push_back("COUNT(*) RCNT");
    RowsPtr result = session.engine()->
        select(StrList(cols), slave_tbl.get_name(), f);
    if (result->size() != 1)
        throw ObjectNotFoundByKey("COUNT(*) FOR " + slave_tbl.get_name() + "("
                                  + f.get_sql() + ")");
    return result->begin()->begin()->second.as_longint();
}

void RelationObject::lazy_load_slaves()
{
    if (status_ != Incomplete)
        return;
    YB_ASSERT(master_object_->session());
    SessionV2 &session = *master_object_->session();
    Schema &schema = master_object_->table().schema();
    const Table &master_tbl = schema.get_table(relation_info_.table(0)),
        &slave_tbl = schema.get_table(relation_info_.table(1));
    Strings cols;
    Columns::const_iterator j = slave_tbl.begin(), jend = slave_tbl.end();
    for (; j != jend; ++j)
        cols.push_back(j->get_name());
    RowsPtr result = session.engine()->
        select(StrList(cols), slave_tbl.get_name(), KeyFilter(gen_fkey()));
    Rows::const_iterator k = result->begin(), kend = result->end();
    for (; k != kend; ++k) {
        ValueMap pk_values;
        j = slave_tbl.begin(), jend = slave_tbl.end();
        for (; j != jend; ++j)
            if (j->is_pk())
                pk_values[j->get_name()] = k->find(j->get_name())->second;
        Key pkey(slave_tbl.get_name(), pk_values);
        DataObject::Ptr o = session.get_lazy(pkey);
        o->fill_from_row(*k);
        DataObject::link(master_object_, o, relation_info_);
    }
    status_ = Sync;
}

void RelationObject::refresh_slaves_fkeys()
{
    Schema &schema = master_object_->table().schema();
    const Table &master_tbl = schema.get_table(relation_info_.table(0)),
        &slave_tbl = schema.get_table(relation_info_.table(1));
    string pk = master_tbl.get_synth_pk();
    Value pk_val = master_object_->get(pk);
    Strings parts;
    StrUtils::split_str(slave_tbl.get_fk_for(&relation_info_), ",", parts);
    string fk = *parts.begin();
    SlaveObjects::iterator i = slave_objects_.begin(),
        iend = slave_objects_.end();
    for (; i != iend; ++i)
        (*i)->set(fk, pk_val);
}

void RelationObject::exclude_slave(DataObject *obj)
{
    SlaveObjects::iterator i = slave_objects_.begin(),
        iend = slave_objects_.end();
    for (; i != iend; ++i) {
        if (i->get() == obj)
            break;
    }
    YB_ASSERT(i != slave_objects_.end());
    slave_objects_.erase(i);
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
