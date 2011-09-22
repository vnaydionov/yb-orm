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

/*
const Relation *DataObject::find_relation_info(
    const string &relation_name, int mode)
{
    Schema &schema(table_.schema());
    const Relation *r = NULL;
    Schema::RelMap::const_iterator
        it = schema.rels_lower_bound(table_.get_class()),
        end = schema.rels_upper_bound(table_.get_class());
    for (; it != end; ++it)
        if (it->second.side(1) == slave->table().get_class()) {
            if (!r) {
                if (relation_name.empty() ||
                    it->second.has_attr(mode, "property") == relation_name)
                {
                    r = &it->second;
                }
            }
            else {
                YB_ASSERT(!it->second.has_attr(mode, "property"));
            }
        }
*/    

void DataObject::link(DataObject *master, DataObject *slave,
                      const string &relation_name, int mode)
{
    // Find relation in metadata.
    Schema &schema(master->table().schema());
    const Relation *r = NULL;
    Schema::RelMap::const_iterator
        it = schema.rels_lower_bound(master->table().get_class()),
        end = schema.rels_upper_bound(master->table().get_class());
    for (; it != end; ++it)
        if (it->second.side(1) == slave->table().get_class()) {
            if (!r) {
                if (relation_name.empty() ||
                    it->second.has_attr(mode, "property") == relation_name)
                {
                    r = &it->second;
                }
            }
            else {
                YB_ASSERT(!it->second.has_attr(mode, "property"));
            }
        }
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
}

DataObject::Ptr DataObject::go_to_master(
    const std::string relation_name)
{
    /*
    
;
    RelationObject *go_to_slaves(const std::string relation_name = "");
    */
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
    string fk = table_.get_fk_for(rel->relation_info().master().get_name());
    vector<string> parts;
    StrUtils::split_str(fk, ",", parts);
    using namespace boost::lambda;
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
        }
    }
    else if (relation_info_.cascade() == Relation::Delete) {
        SlaveObjects::iterator i = slave_objects_.begin(),
            iend = slave_objects_.end();
        for (; i != iend; ++i)
            (*i)->delete_object(mode);
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
