#include <orm/DataObject.h>
#include <algorithm>
#include <boost/lambda/lambda.hpp>
#include <util/str_utils.hpp>

using namespace std;

namespace Yb {

void DataObject::load() {
    YB_ASSERT(session_ != NULL);
    ///
    status_ = Sync;
}

DataObject::~DataObject()
{
    if (!session_) {
        //could_be_deleted();
    }
}

bool DataObject::could_be_deleted() const
{
    MasterRelations::const_iterator
        i = master_relations_.begin(),
        iend = master_relations_.end();
    for (; i != iend; ++i)
        if (!(*i)->could_master_be_deleted())
            return false;
    return true;
}

void DataObject::delete_object()
{
    delete_master_relations();
    exclude_from_slave_relations();
    status_ = Deleted;
}

void DataObject::delete_master_relations()
{
    MasterRelations::iterator i = master_relations_.begin(),
        iend = master_relations_.end();
    for (; i != iend; ++i)
        (*i)->delete_master();
    MasterRelations empty;
    master_relations_.swap(empty);
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
    string fk = table_.get_fk_for(rel->relation_info().master());
    vector<string> parts;
    StrUtils::split_str(fk, ",", parts);
    using namespace boost::lambda;
    vector<string> ::iterator i = parts.begin(), end = parts.end();
    for (; i != end; ++i)
        set(*i, Value());
}

bool RelationObject::could_master_be_deleted() const
{
    if (relation_info_.cascade() == Relation::Nullify)
        return true;
    if (relation_info_.cascade() == Relation::Restrict)
        return slave_objects_.size() == 0;
    SlaveObjects::const_iterator i = slave_objects_.begin(),
        iend = slave_objects_.end();
    for (; i != iend; ++i)
        if (!(*i)->could_be_deleted())
            return false;
    return true;
}

void RelationObject::delete_master()
{
    if (relation_info_.cascade() == Relation::Nullify) {
        SlaveObjects::iterator i = slave_objects_.begin(),
            iend = slave_objects_.end();
        for (; i != iend; ++i)
            (*i)->set_free_from(this);
    }
    else if (relation_info_.cascade() == Relation::Delete) {
        SlaveObjects::iterator i = slave_objects_.begin(),
            iend = slave_objects_.end();
        for (; i != iend; ++i)
            (*i)->delete_object();
    }
    else {
        if (slave_objects_.size() != 0)
            throw CascadeDeleteError(relation_info_);
    }
}

void RelationObject::exclude_slave(DataObject *obj)
{
    SlaveObjects::iterator i = find(
            slave_objects_.begin(), slave_objects_.end(),
            obj);
    YB_ASSERT(i != slave_objects_.end());
    slave_objects_.erase(i);
}


} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
