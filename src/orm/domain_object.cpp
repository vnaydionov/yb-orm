// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#define YBORM_SOURCE

#ifdef YB_DEBUG_DOMAIN_REG
#include <iostream>
#endif
#include "orm/domain_object.h"

namespace Yb {

NoDataObject::NoDataObject()
    : ORMError(_T("No ROW data is associated with DomainObject"))
{}

NoSessionBaseGiven::NoSessionBaseGiven()
    : ORMError(_T("No session given to the WeakObject"))
{}

AlreadySavedInAnotherSession::AlreadySavedInAnotherSession()
    : ORMError(_T("Object has been already saved in some other session"))
{}

CouldNotSaveEmptyObject::CouldNotSaveEmptyObject()
    : ORMError(_T("Attempt to save an empty object failed"))
{}

OutOfManagedList::OutOfManagedList(int pos, int sz)
    : ORMError(_T("Trying to access index ") +
            to_string(pos) +
            _T(" that falls out of ManagedList of size ") +
            to_string(sz))
{}

InvalidIterator::InvalidIterator()
    : ORMError(_T("Trying to use an invalid iterator"))
{}

namespace {
typedef std::pair<Tables, Relations> SchemaInfo;
}

int DomainObject::init_flag_ = 0;
void *DomainObject::pending_ = NULL;

bool DomainObject::register_table_meta(Table::Ptr tbl)
{
#ifdef YB_DEBUG_DOMAIN_REG
    std::cerr << "register_table_meta(" << tbl->name()
              << "), init_flag_=" << init_flag_ << std::endl;
#endif
    if (init_flag_)
        return false;
    SchemaInfo *&items =
        *reinterpret_cast<SchemaInfo **> (&pending_);
    if (!items)
        items = new SchemaInfo();
    items->first.push_back(tbl);
    return true;
}

bool DomainObject::register_relation_meta(Relation::Ptr rel)
{
#ifdef YB_DEBUG_DOMAIN_REG
    std::cerr << "register_relation_meta(" << rel->side(0)
              << ", " << rel->side(1)
              << "), init_flag_=" << init_flag_ << std::endl;
#endif
    if (init_flag_)
        return false;
    SchemaInfo *&items =
        *reinterpret_cast<SchemaInfo **> (&pending_);
    if (!items)
        items = new SchemaInfo();
    items->second.push_back(rel);
    return true;
}

void DomainObject::save_registered(Schema &schema)
{
#ifdef YB_DEBUG_DOMAIN_REG
    std::cerr << "save_registered(), init_flag_=" << init_flag_ << std::endl;
#endif
    if (init_flag_)
        return;
    init_flag_ = 1;
    SchemaInfo *&items =
        *reinterpret_cast<SchemaInfo **> (&pending_);
    if (items) {
        Tables &tables = items->first;
        Tables::iterator i = tables.begin(), iend = tables.end();
        for (; i != iend; ++i) {
#ifdef YB_DEBUG_DOMAIN_REG
            std::cerr << "table: " << (*i)->name() << std::endl;
#endif
            schema.add_table(*i);
        }
        Relations &relations = items->second;
        Relations::iterator j = relations.begin(), jend = relations.end();
        for (; j != jend; ++j) {
#ifdef YB_DEBUG_DOMAIN_REG
            std::cerr << "relation: " << (*j)->side(0) << "," << (*j)->side(1) << std::endl;
#endif
            schema.add_relation(*j);
        }
        delete items;
        items = NULL;
    }
}

void DomainObject::check_ptr() const
{
    if (!shptr_get(d_))
        throw NoDataObject();
}

DomainObject::DomainObject(const Table &table,
        DomainObject *owner, const String &prop_name)
    : owner_(owner)
    , prop_name_(prop_name)
{}

DataObject::Ptr DomainObject::get_data_object(bool check) const
{
    if (owner_) {
        DataObject::Ptr r(DataObject::get_master(owner_->get_data_object(check), prop_name_));
        if (!r.get() && check)
            throw NoDataObject();
        return r;
    }
    if (check)
        check_ptr();
    return d_;
}

void DomainObject::set_data_object(DataObject::Ptr d)
{
    if (owner_) {
        owner_->check_ptr();
        DomainObject master(d);
        owner_->link_to_master(master, prop_name_);
    }
    else
        d_ = d;
}

DomainObject::DomainObject()
    : owner_(NULL)
{}

DomainObject::DomainObject(DataObject::Ptr d)
    : d_(d)
    , owner_(NULL)
{}

DomainObject::DomainObject(const Table &table)
    : d_(DataObject::create_new(table))
    , owner_(NULL)
{}

DomainObject::DomainObject(const Schema &schema, const String &table_name)
    : d_(DataObject::create_new(schema.table(table_name)))
    , owner_(NULL)
{}

DomainObject::DomainObject(Session &session, const Key &key)
    : d_(session.get_lazy(key))
    , owner_(NULL)
{}

DomainObject::DomainObject(Session &session, const String &tbl_name,
        LongInt id)
    : d_(session.get_lazy(session.schema().table(tbl_name).mk_key(id)))
    , owner_(NULL)
{}

DomainObject::DomainObject(const DomainObject &obj)
    : d_(obj.get_data_object(false))
    , owner_(NULL)
{}

DomainObject::~DomainObject()
{}

DomainObject &DomainObject::operator=(const DomainObject &obj)
{
    if (this != &obj)
        set_data_object(obj.get_data_object(false));
    return *this;
}

void DomainObject::save(Session &session)
{
    session.save(get_data_object());
}

void DomainObject::detach(Session &session)
{
    session.detach(get_data_object());
}

bool DomainObject::is_empty() const
{
    return !shptr_get(get_data_object(false));
}

Session *DomainObject::get_session() const
{
    return get_data_object()->session();
}

const Value &DomainObject::get(const String &col_name) const
{
    return get_data_object()->get(col_name);
}

void DomainObject::set(const String &col_name, const Value &value)
{
    get_data_object()->set(col_name, value);
}

const Value &DomainObject::get(int col_num) const
{
    return get_data_object()->get(col_num);
}

void DomainObject::set(int col_num, const Value &value)
{
    get_data_object()->set(col_num, value);
}

const DomainObject DomainObject::get_master(
        const String &relation_name) const
{
    return DomainObject(
            DataObject::get_master(get_data_object(), relation_name));
}

RelationObject *DomainObject::get_slaves_ro(
        const String &relation_name) const
{
    return get_data_object()->get_slaves(relation_name);
}

ManagedList<DomainObject> DomainObject::get_slaves(
        const String &relation_name) const
{
    DataObject::Ptr p = get_data_object();
    return ManagedList<DomainObject>(p->get_slaves(relation_name), p);
}

void DomainObject::link_to_master(DomainObject &master,
        const String relation_name)
{
    DataObject::link_slave_to_master(get_data_object(),
            master.get_data_object(), relation_name);
}

void DomainObject::link_to_slave(DomainObject &slave,
        const String relation_name)
{
    DataObject::link_master_to_slave(get_data_object(),
            slave.get_data_object(), relation_name);
}

void DomainObject::delete_object()
{
    get_data_object()->delete_object();
}

DataObject::Status DomainObject::status() const
{
    return get_data_object()->status();
}

const Table &DomainObject::get_table() const
{
    return get_data_object()->table();
}

int DomainObject::cmp(const DomainObject &x) const
{
    DataObject::Ptr p1 = get_data_object(false), p2 = x.get_data_object(false);
    if (p1 == p2)
        return 0;
    if (!shptr_get(p1))
        return -1;
    if (!shptr_get(p2))
        return 1;
    if (p1->raw_values() == p2->raw_values())
        return 0;
    return p1->raw_values() < p2->raw_values()? -1: 1;
}

ElementTree::ElementPtr DomainObject::xmlize(
        int depth, const String &alt_name) const
{
    Session *s = get_session();
    if (!s)
        throw NoSessionBaseGiven();
    return deep_xmlize(*s, get_data_object(), depth, alt_name);
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
