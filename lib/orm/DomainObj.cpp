// -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#include <orm/DomainObj.h>

using namespace std;

namespace Yb {

namespace {
    typedef std::pair<Tables, Relations> SchemaInfo;
}

char DomainObject::init_[16] = "VARKALOSHLIVKIE";

bool DomainObject::register_table_meta(Table::Ptr tbl)
{
    if (!memcmp(init_, "SHARKISHNYRYALI", 16))
        return false;
    SchemaInfo **items = (SchemaInfo **)&init_;
    if (!memcmp(init_, "VARKALOSHLIVKIE", 16)) {
        *items = new SchemaInfo();
        if (!memcmp(init_, "VARKALOSHLIVKIE", 16)) {
            SchemaInfo *p = new SchemaInfo();
            delete *items;
            *items = p;
        }
    }
    (*items)->first.push_back(tbl);
    return true;
}

bool DomainObject::register_relation_meta(Relation::Ptr rel)
{
    if (!memcmp(init_, "SHARKISHNYRYALI", 16))
        return false;
    SchemaInfo **items = (SchemaInfo **)&init_;
    if (!memcmp(init_, "VARKALOSHLIVKIE", 16)) {
        *items = new SchemaInfo();
        if (!memcmp(init_, "VARKALOSHLIVKIE", 16)) {
            SchemaInfo *p = new SchemaInfo();
            delete *items;
            *items = p;
        }
    }
    (*items)->second.push_back(rel);
    return true;
}

void DomainObject::save_registered(Schema &schema)
{
    if (!memcmp(init_, "SHARKISHNYRYALI", 16))
        return;
    if (memcmp(init_, "VARKALOSHLIVKIE", 16)) {
        SchemaInfo **items = (SchemaInfo **)&init_;
        Tables &tables = (*items)->first;
        Tables::iterator i = tables.begin(), iend = tables.end();
        for (; i != iend; ++i)
            schema.add_table(*i);
        Relations &relations = (*items)->second;
        Relations::iterator j = relations.begin(), jend = relations.end();
        for (; j != jend; ++j)
            schema.add_relation(*j);
        delete *items;
    }
    memcpy(init_, "SHARKISHNYRYALI", 16);
}

void DomainObject::check_ptr() const
{
    if (!shptr_get(d_))
        throw NoRawData();
}

DomainObject::DomainObject(const Table &table,
        DomainObject *owner, const String &prop_name)
    : owner_(owner)
    , prop_name_(prop_name)
{}

DataObject::Ptr DomainObject::get_data_object(bool check) const
{
    if (owner_) {
        owner_->check_ptr();
        return DataObject::get_master(owner_->d_, prop_name_);
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
    : d_(obj.get_data_object())
    , owner_(NULL)
{}

DomainObject::~DomainObject()
{}

DomainObject &DomainObject::operator=(const DomainObject &obj)
{
    if (this != &obj)
        set_data_object(obj.get_data_object());
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
    if (p1->values() == p2->values())
        return 0;
    return p1->values() < p2->values()? -1: 1;
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
