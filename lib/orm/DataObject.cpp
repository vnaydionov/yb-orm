// -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#include <orm/DataObject.h>
#include <algorithm>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <util/str_utils.hpp>

using namespace std;

namespace Yb {

const string
FilterBackendByKeyFields::do_get_sql() const
{
    ostringstream sql;
    sql << "1=1";
    const Table &t = schema_.get_table(key_.first);
    Columns::const_iterator i = t.begin(), iend = t.end();
    for (; i != iend; ++i) {
        ValuesMap::const_iterator j = key_.second.find(i->get_name());
        if (j != key_.second.end()) {
            sql << " AND " << i->get_name() << " = " << j->second.sql_str();
        }
    }
    return sql.str();
}

const string
FilterBackendByKeyFields::do_collect_params_and_build_sql(Values &seq) const
{
    ostringstream sql;
    sql << "1=1";
    const Table &t = schema_.get_table(key_.first);
    Columns::const_iterator i = t.begin(), iend = t.end();
    for (; i != iend; ++i) {
        ValuesMap::const_iterator j = key_.second.find(i->get_name());
        if (j != key_.second.end()) {
            seq.push_back(j->second);
            sql << " AND " << i->get_name() << " = ?";
        }
    }
    return sql.str();
}

SessionV2::~SessionV2()
{
    Objects objects_copy = objects_;
    using namespace boost::lambda;
    for_each(objects_copy.begin(), objects_copy.end(),
             bind(&SessionV2::detach, this, _1));
}

void SessionV2::save(DataObjectPtr obj)
{
    if (obj->assigned_key()) {
        const Key &key = obj->key();
        IdentityMap::iterator i = identity_map_.find(key);
        if (i != identity_map_.end())
            return;
        identity_map_[key] = obj;
    }
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
    ValuesMap::const_iterator it = key.second.begin(),
        end = key.second.end();
    for (; it != end; ++it)
        new_obj->set(it->first, it->second);
    new_obj->set_session(this);
    identity_map_[key] = new_obj;
    return new_obj;
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

void DataObject::update_key()
{
    key_.first = table_.get_name();
    assigned_key_ = true;
    ValuesMap new_values;
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

bool DataObject::assigned_key()
{
    if (key_.first.empty())
        update_key();
    return assigned_key_;
}

void DataObject::load()
{
    YB_ASSERT(session_ != NULL);
    FilterByKeyFields f(table_.schema(), key());
    FieldList cols;
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

void DataObject::link(DataObject *master, DataObject *slave,
                      const Relation *r)
{
    YB_ASSERT(r != NULL);
    // Try to find relation object in master's relations
    RelationObject *ro = NULL;
    MasterRelations::iterator j = master->master_relations().begin(),
        jend = master->master_relations().end();
    for (; j != jend; ++j)
        if ((*j)->relation_info() == *r) {
            ro = (*j).get();
            break;
        }
    // Create one if it doesn't exist, master will own it
    if (!ro) {
        RelationObject::Ptr new_ro = RelationObject::create_new(*r, master);
        master->master_relations().insert(new_ro);
        ro = new_ro.get();
    }
    // Register slave in the relation
    ro->slave_objects().insert(slave);
    slave->slave_relations().insert(ro);
}

void DataObject::link(DataObject *master, DataObject *slave,
                      const string &relation_name, int mode)
{
    // Find relation in metadata.
    Schema &schema(master->table().schema());
    const Relation *r = schema.find_relation
        (master->table().get_class(), relation_name,
         slave->table().get_class(), mode);
    link(master, slave, r);
}

DataObject::Ptr DataObject::get_master(
    const string &relation_name)
{
    YB_ASSERT(session_);
    // Find relation in metadata.
    Schema &schema(table_.schema());
    const Relation *r = schema.find_relation
        (table_.get_class(), relation_name, "", 1);
    YB_ASSERT(r != NULL);
    // Find FK value.
    string fk = table_.get_fk_for(r);
    vector<string> parts;
    StrUtils::split_str(fk, ",", parts);
    ValuesMap fk_values;
    vector<string> ::iterator i = parts.begin(),
        end = parts.end();
    const Table &master_tbl = schema.get_table(r->table(0));
    // TODO: support compound foreign keys
    //for (; i != end; ++i)
    //    fk_values[*i] = get(*i);
    fk_values[master_tbl.get_unique_pk()] = get(*i);
    Key fkey(master_tbl.get_name(), fk_values);
    DataObject::Ptr master = session_->get_lazy(fkey);
    link(master.get(), this, r);
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

void DataObject::delete_object(DeletionMode mode)
{
    if (mode != DelUnchecked)
        delete_master_relations(DelDryRun);
    if (mode != DelDryRun) {
        delete_master_relations(DelUnchecked);
        exclude_from_slave_relations();
        status_ = status_ == New? Deleted: ToBeDeleted;
    }
}

void DataObject::delete_master_relations(DeletionMode mode)
{
    MasterRelations::iterator i = master_relations_.begin(),
        iend = master_relations_.end();
    for (; i != iend; ++i)
        (*i)->delete_master(mode);
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
    vector<string> parts;
    StrUtils::split_str(fk, ",", parts);
    vector<string> ::iterator i = parts.begin(), end = parts.end();
    for (; i != end; ++i)
        set(*i, Value());
}

void RelationObject::delete_master(DeletionMode mode)
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
            (*i)->delete_object(mode);
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
    vector<string> parts;
    StrUtils::split_str(slave_tbl.get_fk_for(&relation_info_), ",", parts);
    // TODO: support compound foreign keys
    ValuesMap fk_values;
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
    FilterByKeyFields f(schema, gen_fkey());
    FieldList cols;
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
    FilterByKeyFields f(schema, gen_fkey());
    FieldList cols;
    Columns::const_iterator j = slave_tbl.begin(), jend = slave_tbl.end();
    for (; j != jend; ++j)
        cols.push_back(j->get_name());
    RowsPtr result = session.engine()->
        select(StrList(cols), slave_tbl.get_name(), f);
    Rows::const_iterator k = result->begin(), kend = result->end();
    for (; k != kend; ++k) {
        ValuesMap pk_values;
        j = slave_tbl.begin(), jend = slave_tbl.end();
        for (; j != jend; ++j)
            if (j->is_pk())
                pk_values[j->get_name()] = k->find(j->get_name())->second;
        Key pkey(slave_tbl.get_name(), pk_values);
        DataObject::Ptr o = session.get_lazy(pkey);
        o->fill_from_row(*k);
        DataObject::link(master_object_, o.get(), &relation_info_);
    }
    status_ = Sync;
}

void RelationObject::exclude_slave(DataObject *obj)
{
    SlaveObjects::iterator i =
        find(slave_objects_.begin(), slave_objects_.end(), obj);
    YB_ASSERT(i != slave_objects_.end());
    slave_objects_.erase(i);
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
