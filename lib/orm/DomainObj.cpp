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

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
