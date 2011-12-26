// -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#include <orm/DataObject.h>
#include <algorithm>
#include <iostream>
#if 0
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#endif
#include <util/str_utils.hpp>

using namespace std;

namespace Yb {

StringTooLong::StringTooLong(
        const String &table_name, const String &field_name,
        int max_len, const String &value)
    : ORMError(_T("Can't set value of ") + table_name + _T(".") + field_name +
            _T(" with '") + value + _T("', having max length ") +
            boost::lexical_cast<String>(max_len))
{}

BadTypeCast::BadTypeCast(
        const String &table_name, const String &field_name,
        const String &str_value, const String &type)
    : ORMError(_T("Can't cast field ") + table_name + _T(".") + field_name +
            _T(" = \"") + str_value + _T("\" to type ") + type)
{}

bool DataObjectResultSet::fetch(ObjectList &row)
{
    if (!it_.get())
        it_.reset(new SqlResultSet::iterator(rs_.begin()));
    if (rs_.end() == *it_)
        return false;
    ObjectList new_row;
    const Row &cur = **it_;
    size_t pos = 0;
    for (int i = 0; i < tables_.size(); ++i) {
        DataObject::Ptr d = DataObject::create_new
            (*tables_[i], DataObject::Sync);
        pos = d->fill_from_row(cur, pos);
        DataObject::Ptr e = session_.save_or_update(d);
        new_row.push_back(e);
    }
    row.swap(new_row);
    ++*it_;
    return true;
}

DataObjectResultSet::DataObjectResultSet(const SqlResultSet &rs, Session &session,
                                         const Strings &tables)
    : rs_(rs)
    , session_(session)
{
    const Schema &schema = session.schema();
    Strings::const_iterator i = tables.begin(), iend = tables.end();
    for (; i != iend; ++i)
        tables_.push_back(&schema.table(*i));
}
                                             
Session::~Session()
{
    Objects objects_copy = objects_;
#if 0
    using namespace boost::lambda;
    for_each(objects_copy.begin(), objects_copy.end(),
             bind(&Session::detach, this, _1));
#else
    Objects::iterator i = objects_copy.begin(), iend = objects_copy.end();
    for (; i != iend; ++i)
        detach(*i);
#endif
}

const String key2str(const Key &key)
{
    OStringStream out;
    out << _T("Key('") << key.first << _T("', {");
    ValueMap::const_iterator i = key.second.begin(), iend = key.second.end();
    for (; i != iend; ++i)
        out << _T("'") << i->first << _T("': ") << i->second.sql_str() << _T(", ");
    out << _T("}");
    return out.str();
}

DataObjectAlreadyInSession::DataObjectAlreadyInSession(const Key &key)
    : ORMError(_T("DataObject is already registered in the identity map: ") + key2str(key))
{}

DataObjectPtr Session::add_to_identity_map(DataObjectPtr obj, bool return_found)
{
    if (obj->assigned_key()) {
        const Key &key = obj->key();
        IdentityMap::iterator i = identity_map_.find(key);
        if (i != identity_map_.end()) {
            if (!return_found)
                throw DataObjectAlreadyInSession(key);
            DataObjectPtr eobj = i->second;
            const Table &table = obj->table();
            for (size_t i = 0; i < table.size(); ++i) {
                const Column &c = table.get_column(i);
                if (!c.is_pk())
                    eobj->values_[i] = obj->values_[i];
            }
            if (eobj->status_ == DataObject::Sync)
                eobj->status_ = DataObject::Dirty;
            return eobj;
        }
        identity_map_[key] = obj;
    }
    return obj;
}

void Session::save(DataObjectPtr obj0)
{
    DataObjectPtr obj = add_to_identity_map(obj0, false);
    Objects::iterator i = objects_.find(obj);
    if (i == objects_.end()) {
        objects_.insert(obj);
        obj->set_session(this);
    }
}

DataObjectPtr Session::save_or_update(DataObjectPtr obj0)
{
    DataObjectPtr obj = add_to_identity_map(obj0, true);
    Objects::iterator i = objects_.find(obj);
    if (i == objects_.end()) {
        objects_.insert(obj);
        obj->set_session(this);
    }
    return obj;
}

void Session::detach(DataObjectPtr obj)
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

void Session::load_collection(ObjectList &out,
                                const String &table_name,
                                const Filter &filter, 
                                const StrList &order_by,
                                int max,
                                const String &table_alias)
{
    Strings tables(1);
    tables[0] = table_name;
    DataObjectResultSet rs = load_collection(tables, filter, order_by, max);
    DataObjectResultSet::iterator i = rs.begin(), iend = rs.end();
    for (; i != iend; ++i)
        out.push_back((*i)[0]);
}

DataObjectResultSet Session::load_collection(
        const Strings &tables, const Filter &filter,
        const StrList &order_by, int max)
{
    Strings cols;
    Strings::const_iterator i = tables.begin(), iend = tables.end();
    for (; i != iend; ++i) {
        const Table &table = schema_->table(*i);
        Columns::const_iterator j = table.begin(), jend = table.end();
        for (; j != jend; ++j)
            cols.push_back(table.get_name() + _T(".") + j->get_name());
    }
    SqlResultSet result = engine_->select_iter
        (StrList(cols), StrList(tables), filter,
         StrList(), Filter(), order_by, max);
    return DataObjectResultSet(result, *this, tables);
}

DataObject::Ptr Session::get_lazy(const Key &key)
{
    IdentityMap::iterator i = identity_map_.find(key);
    if (i != identity_map_.end())
        return i->second;
    bool empty_key = true;
    ValueMap::const_iterator it = key.second.begin(),
        end = key.second.end();
    for (; it != end; ++it)
        if (!it->second.is_null())
            empty_key = false;
    if (empty_key)
        throw NullPK(key.first);
    DataObjectPtr new_obj =
        DataObject::create_new(schema_->table(key.first), DataObject::Ghost);
    for (it = key.second.begin(); it != end; ++it)
        new_obj->set(it->first, it->second);
    new_obj->set_session(this);
    identity_map_[key] = new_obj;
    return new_obj;
}

typedef map<String, Rows> RowsByTable;

void add_row_to_rows_by_table(RowsByTable &rows_by_table,
                              const String &tbl_name, const Row &row)
{
    RowsByTable::iterator k = rows_by_table.find(tbl_name);
    if (k == rows_by_table.end()) {
        Rows rows(1);
        rows[0] = row;
        rows_by_table.insert(pair<String, Rows> (tbl_name, rows));
    }
    else {
        Rows &rows = k->second;
        rows.push_back(row);
    }
}

void Session::flush_tbl_new_keyed(const Table &tbl, Objects &keyed_objs)
{
    Rows rows;
    Objects::iterator i, iend = keyed_objs.end();
    for (i = keyed_objs.begin(); i != iend; ++i) {
        (*i)->refresh_master_fkeys();
        rows.push_back((*i)->values());
    }
    engine_->insert(tbl.get_name(), rows, StringSet());
}

void Session::flush_tbl_new_unkeyed(const Table &tbl, Objects &unkeyed_objs)
{
    bool sql_seq = engine_->get_dialect()->has_sequences();
    bool use_seq = sql_seq && !tbl.get_seq_name().empty();
    bool use_autoinc = !sql_seq &&
        (tbl.get_autoinc() || !tbl.get_seq_name().empty());
    Objects::iterator i, iend = unkeyed_objs.end();
    if (use_seq) {
        String pk = tbl.get_synth_pk();
        for (i = unkeyed_objs.begin(); i != iend; ++i)
            (*i)->set(pk, engine_->get_next_value(tbl.get_seq_name()));
    }
    Rows rows;
    for (i = unkeyed_objs.begin(); i != iend; ++i) {
        (*i)->refresh_master_fkeys();
        rows.push_back((*i)->values(!use_autoinc));
    }
    vector<LongInt> ids = engine_->insert
        (tbl.get_name(), rows, StringSet(), use_autoinc);
    if (use_autoinc) {
        String pk = tbl.get_synth_pk();
        i = unkeyed_objs.begin();
        for (int p = 0; i != iend; ++i, ++p)
            (*i)->set(pk, ids[p]);
    }
    // Refresh fk values for all slaves and
    // add flushed objects to the identity_map_
    for (i = unkeyed_objs.begin(); i != iend; ++i) {
        (*i)->refresh_slaves_fkeys();
        add_to_identity_map(*i, false);
    }
}

void Session::flush_new()
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
    typedef map<String, Objects> ObjectsByTable;
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
        const String &tbl_name = (*i)->table().get_name();
        ObjectsByTable::iterator q = objs_by_table.find(tbl_name);
        if (q == objs_by_table.end()) {
            objs_by_table.insert(pair<String, Objects>
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
            const String &tbl_name = j->first;
            const Table &tbl = schema_->table(tbl_name);
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

void Session::flush_update(IdentityMap &idmap_copy)
{
    typedef map<String, Rows> RowsByTable;
    RowsByTable rows_by_table;
    IdentityMap::iterator i = idmap_copy.begin(), iend = idmap_copy.end();
    for (; i != iend; ++i)
        if (i->second->status() == DataObject::Dirty) {
            const String &tbl_name = i->first.first;
            i->second->refresh_master_fkeys();
            add_row_to_rows_by_table(rows_by_table, tbl_name,
                                     i->second->values());
            i->second->set_status(DataObject::Ghost);
        }
    RowsByTable::iterator j = rows_by_table.begin(),
        jend = rows_by_table.end();
    for (; j != jend; ++j) {
        engine_->update(j->first, j->second,
                        schema_->table(j->first).pk_fields());
    }
}

void Session::flush_delete(IdentityMap &idmap_copy)
{
    typedef vector<Key> Keys;
    typedef map<String, Keys> KeysByTable;
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
        const String &tbl_name = i->second->table().get_name();
        KeysByTable::iterator q = keys_by_table.find(tbl_name);
        if (q == keys_by_table.end()) {
            keys_by_table.insert(pair<String, Keys> (tbl_name, Keys()));
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

void Session::flush()
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

void DataObject::set_session(Session *session)
{
    YB_ASSERT(!session_);
    session_ = session;
}

void DataObject::forget_session()
{
    YB_ASSERT(session_);
    session_ = NULL;
}

const Value DataObject::get_typed_value(const Column &col, const Value &v)
{
    try {
        return v.get_typed_value(col.get_type());
    }
    catch (ValueBadCast &) {
        throw BadTypeCast(table_.get_name(), col.get_name(),
                          v.as_string(),
                          Value::get_type_name(col.get_type()));
    }
}

void DataObject::touch()
{
    if (status_ == Sync)
        status_ = Dirty;
}
    
void DataObject::set(int i, const Value &v)
{
    const Column &c = table_.get_column(i);
    lazy_load(&c);
    if (c.is_pk() && session_ != NULL
        && values_[i] != v && !values_[i].is_null())
    {
        throw ReadOnlyColumn(table_.get_name(), c.get_name());
    }
    if (c.is_ro() && !c.is_pk())
        throw ReadOnlyColumn(table_.get_name(), c.get_name());
    if (c.get_type() == Value::STRING) {
        String s = v.as_string();
        if (c.get_size() && c.get_size() < s.size())
            throw StringTooLong(table_.get_name(), c.get_name(),
                                c.get_size(), s);
        values_[i] = Value(s);
    }
    else {
        values_[i] = get_typed_value(c, v);
    }
    //values_[i] = v;
    if (c.is_pk()) {
        update_key();
    }
    else
        touch();
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

const Row DataObject::values(bool include_key)
{
    Row values;
    for (size_t i = 0; i < table_.size(); ++i) {
        const Column &c = table_.get_column(i);
        if (!c.is_pk() || include_key)
            values.push_back(RowItem(c.get_name(), values_[i]));
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
        throw ObjectNotFoundByKey(table_.get_name() + _T("(")
                                  + f.get_sql() + _T(")"));
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
    slave->lazy_load();
    // Find existing relation in slave's relations
    RelationObject *ro = NULL;
    SlaveRelations::iterator i = slave->slave_relations().begin(),
        iend = slave->slave_relations().end();
    for (; i != iend; ++i)
        if (&(*i)->relation_info() == &r) {
            ro = *i;
            slave->slave_relations().erase(i);
            ro->slave_objects().erase(slave);
            break;
        }
    // Try to find relation object in master's relations
    ro = NULL;
    MasterRelations::iterator j = master->master_relations().begin(),
        jend = master->master_relations().end();
    for (; j != jend; ++j)
        if (&(*j)->relation_info() == &r) {
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
    slave->touch();
}

void DataObject::link(DataObject *master, DataObject::Ptr slave,
                      const String &relation_name, int mode)
{
    // Find relation in metadata.
    const Schema &schema(master->table().schema());
    const Relation *r = schema.find_relation
        (master->table().get_class(), relation_name,
         slave->table().get_class(), mode);
    link(master, slave, *r);
}

DataObject::Ptr DataObject::get_master(
    DataObject::Ptr obj, const String &relation_name)
{
    YB_ASSERT(obj.get());
    YB_ASSERT(obj->session_);
    // Find relation in metadata.
    const Schema &schema(obj->table_.schema());
    const Relation *r = schema.find_relation
        (obj->table_.get_class(), relation_name, _T(""), 1);
    YB_ASSERT(r != NULL);
    // Find FK value.
    String fk = obj->table_.get_fk_for(r);
    Strings parts;
    StrUtils::split_str(fk, _T(","), parts);
    ValueMap fk_values;
    Strings::iterator i = parts.begin(),
        end = parts.end();
    const Table &master_tbl = r->table(0);
    // TODO: support compound foreign keys
    //for (; i != end; ++i)
    //    fk_values[*i] = get(*i);
    fk_values[master_tbl.get_unique_pk()] = obj->get(*i);
    Key fkey(master_tbl.get_name(), fk_values);
    DataObject::Ptr master = obj->session_->get_lazy(fkey);
    link(master.get(), obj, *r);
    return master;
}

bool DataObject::has_master(
    DataObject::Ptr obj, const String &relation_name)
{
    try {
        Ptr p = get_master(obj, relation_name);
        return true;
    }
    catch (const NullPK &) {
        return false;
    }
}

RelationObject *DataObject::get_slaves(
    const String &relation_name)
{
    YB_ASSERT(session_);
    // Find relation in metadata.
    const Schema &schema(table_.schema());
    const Relation *r = schema.find_relation
        (table_.get_class(), relation_name, _T(""), 0);
    YB_ASSERT(r != NULL);
    // Try to find relation object in master's relations
    RelationObject *ro = NULL;
    MasterRelations::iterator j = master_relations_.begin(),
        jend = master_relations_.end();
    for (; j != jend; ++j)
        if (&(*j)->relation_info() == r) {
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

size_t DataObject::fill_from_row(const Row &r, size_t pos)
{
    size_t i = 0;
    for (; i < table_.size(); ++i) {
        const Column &c = table_.get_column(i);
        values_[i] = get_typed_value(c, r[pos + i].second);
    }
    update_key();
    status_ = Sync;
    return pos + i;
}

void DataObject::refresh_slaves_fkeys()
{
    MasterRelations::iterator i = master_relations_.begin(),
        iend = master_relations_.end();
    for (; i != iend; ++i)
        (*i)->refresh_slaves_fkeys();
}

void DataObject::refresh_master_fkeys()
{
    SlaveRelations::iterator i = slave_relations_.begin(),
        iend = slave_relations_.end();
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
    String fk = table_.get_fk_for(&rel->relation_info());
    Strings parts;
    StrUtils::split_str(fk, _T(","), parts);
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
    const Table &master_tbl = relation_info_.table(0),
        &slave_tbl = relation_info_.table(1);
    Strings parts;
    StrUtils::split_str(slave_tbl.get_fk_for(&relation_info_), _T(","), parts);
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
    Session &session = *master_object_->session();
    const Table &master_tbl = relation_info_.table(0),
        &slave_tbl = relation_info_.table(1);
    KeyFilter f(gen_fkey());
    Strings cols;
    cols.push_back(_T("COUNT(*) RCNT"));
    RowsPtr result = session.engine()->
        select(StrList(cols), slave_tbl.get_name(), f);
    if (result->size() != 1)
        throw ObjectNotFoundByKey(_T("COUNT(*) FOR ") + slave_tbl.get_name() + _T("(")
                                  + f.get_sql() + _T(")"));
    return result->begin()->begin()->second.as_longint();
}

void RelationObject::lazy_load_slaves()
{
    if (status_ != Incomplete)
        return;
    YB_ASSERT(master_object_->session());
    Session &session = *master_object_->session();
    const Table &master_tbl = relation_info_.table(0),
        &slave_tbl = relation_info_.table(1);
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
                pk_values[j->get_name()] = find_in_row(*k, j->get_name())->second;
        Key pkey(slave_tbl.get_name(), pk_values);
        DataObject::Ptr o = session.get_lazy(pkey);
        o->fill_from_row(*k);
        DataObject::link(master_object_, o, relation_info_);
    }
    status_ = Sync;
}

void RelationObject::refresh_slaves_fkeys()
{
    const Table &master_tbl = relation_info_.table(0),
        &slave_tbl = relation_info_.table(1);
    String pk = master_tbl.get_synth_pk();
    Value pk_val = master_object_->get(pk);
    Strings parts;
    StrUtils::split_str(slave_tbl.get_fk_for(&relation_info_), _T(","), parts);
    String fk = *parts.begin();
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
