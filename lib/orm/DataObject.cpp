// -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#include <orm/DataObject.h>
#include <algorithm>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <util/str_utils.hpp>

using namespace std;

namespace Yb {

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
    if (!key_.first.size())
        update_key();
    return key_;
}

bool DataObject::assigned_key()
{
    if (!key_.first.size())
        update_key();
    return assigned_key_;
}

void DataObject::load()
{
    YB_ASSERT(session_ != NULL);
    ///
    status_ = Sync;
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
        (table().get_class(), relation_name, "", 1);
    YB_ASSERT(r != NULL);
    // Find FK value.
    string fk = table_.get_fk_for(r);
    vector<string> parts;
    StrUtils::split_str(fk, ",", parts);
    ValuesMap fk_values;
    vector<string> ::iterator i = parts.begin(),
        end = parts.end();
    for (; i != end; ++i)
        fk_values[*i] = get(*i);
    Key fkey(schema.find_table_by_class(r->side(0))
             .get_name(), fk_values);
    DataObject::Ptr master = session_->get_lazy(fkey);
    link(master.get(), this, r);
    return master;
}

RelationObject *DataObject::get_slaves(
    const string &relation_name)
{
    ///
    return NULL;
}

DataObject::~DataObject()
{}

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

void RelationObject::exclude_slave(DataObject *obj)
{
    SlaveObjects::iterator i =
        find(slave_objects_.begin(), slave_objects_.end(), obj);
    YB_ASSERT(i != slave_objects_.end());
    slave_objects_.erase(i);
}


} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
