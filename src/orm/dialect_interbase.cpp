// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#define YBORM_SOURCE

#include <algorithm>
#include "dialect_interbase.h"
#include "util/string_utils.h"
#include "orm/expression.h"

namespace Yb {

using namespace std;
using namespace Yb::StrUtils;

InterbaseDialect::InterbaseDialect()
    : SqlDialect(_T("INTERBASE"), _T("RDB$DATABASE"), true)
{}

const String 
InterbaseDialect::select_curr_value(const String &seq_name)
{ 
    return _T("GEN_ID(") + seq_name + _T(", 0)"); 
}

const String 
InterbaseDialect::select_next_value(const String &seq_name)
{ 
    return _T("GEN_ID(") + seq_name + _T(", 1)"); 
}

const String 
InterbaseDialect::sql_value(const Value &x)
{
    return x.sql_str();
}

bool 
InterbaseDialect::commit_ddl() 
{ 
    return true;
}

const String 
InterbaseDialect::type2sql(int t) 
{
    switch (t) {
        case Value::INTEGER:    return _T("INTEGER");       break;
        case Value::LONGINT:    return _T("BIGINT");        break;
        case Value::STRING:     return _T("VARCHAR");       break;
        case Value::DECIMAL:    return _T("DECIMAL(16, 6)"); break;
        case Value::DATETIME:   return _T("TIMESTAMP");     break;
        case Value::FLOAT:      return _T("DOUBLE PRECISION"); break;
    }
    throw SqlDialectError(_T("Bad type"));
}

const String InterbaseDialect::create_sequence(const String &seq_name) 
{
    return _T("CREATE GENERATOR ") + seq_name;
}

const String 
InterbaseDialect::drop_sequence(const String &seq_name) 
{
    return _T("DROP GENERATOR ") + seq_name;
}

int 
InterbaseDialect::pager_model() 
{ 
    return (int)PAGER_INTERBASE; 
}

// schema introspection
bool 
InterbaseDialect::table_exists(SqlConnection &conn, const String &table)
{ 
    Strings t = get_tables(conn);
    for (Strings::iterator i = t.begin(); i != t.end(); ++i)
    {
        if (*i == table)
        {
            return true;
        }
    }
    return false;
}

bool 
InterbaseDialect::view_exists(SqlConnection &conn, const String &table)
{ 
    return false; 
}


Strings
InterbaseDialect::get_tables(SqlConnection &conn)
{
    Strings tables;
    auto_ptr<SqlCursor> cursor = conn.new_cursor();
    String query = _T("SELECT RDB$RELATIONS.RDB$RELATION_NAME FROM RDB$RELATIONS ")
                   _T("WHERE ((RDB$RELATIONS.RDB$SYSTEM_FLAG = 0) ") 
                   _T("AND (RDB$RELATIONS.RDB$VIEW_SOURCE IS NULL))");
    cursor->prepare(query);
    Values params;
    SqlResultSet rs = cursor->exec(params);
    for (SqlResultSet::iterator i = rs.begin(); i != rs.end(); ++i) {
        tables.push_back((*i)[0].second.as_string());
    }
    cout << tables[0];
    return tables;    
}

Strings 
InterbaseDialect::get_views(SqlConnection &conn)
{
    return Strings();
}

ColumnsInfo 
InterbaseDialect::get_columns(SqlConnection &conn, const String &table)
{ 
    ColumnsInfo col_mass;
    auto_ptr<SqlCursor> cursor = conn.new_cursor();

    String query = _T("SELECT RDB$RELATIONS.RDB$RELATION_NAME, RDB$RELATION_FIELDS.RDB$FIELD_NAME, ") 
                   _T("RDB$RELATION_FIELDS.RDB$NULL_FLAG, RDB$RELATION_FIELDS.RDB$DEFAULT_SOURCE, ")
                   _T("RDB$FIELDS.RDB$NULL_FLAG, RDB$FIELDS.RDB$FIELD_TYPE, RDB$TYPES.RDB$TYPE_NAME ")
                   _T("FROM RDB$RELATIONS INNER JOIN RDB$RELATION_FIELDS ON ")
                   _T("(RDB$RELATIONS.RDB$RELATION_NAME = RDB$RELATION_FIELDS.RDB$RELATION_NAME) INNER JOIN ")
                   _T("RDB$FIELDS ON (RDB$RELATION_FIELDS.RDB$FIELD_SOURCE = RDB$FIELDS.RDB$FIELD_NAME) INNER JOIN ")
                   _T("RDB$TYPES ON (RDB$FIELDS.RDB$FIELD_TYPE = RDB$TYPES.RDB$TYPE) ")
                   _T("WHERE (RDB$RELATIONS.RDB$RELATION_NAME='")
                   + table + 
                   _T("');");
    cout << query;
    cursor->prepare(query);
    Values params;
    SqlResultSet rs = cursor->exec(params);
    for (SqlResultSet::iterator i = rs.begin(); i != rs.end(); ++i)
    {
             
        ColumnInfo x;
        for (Row::const_iterator j = i->begin(); j != i->end(); ++j)
        {
            if (_T("RDB$FIELD_NAME") == j->first)
            {
                x.name = str_to_upper(j->second.as_string());
                cout << x.name << endl;
                continue;
            }
            if (_T("RDB$TYPE_NAME") == j->first)
            {
                x.type = str_to_upper(j->second.as_string());
                cout << x.type << endl;
                continue;
            }
            if (_T("RDB$DEFAULT_SOURCE") == j->first)
            {
                if (!j->second.is_null())
                    x.default_value = j->second.as_string();
                cout <<  x.default_value << endl;
                continue;
            }
            if (_T("RDB$NULL_FLAG") == j->first)
            {
                if (j->second.is_null())    
                    x.notnull = false;
                else
                    x.notnull = true;
                continue;            
            }
        }
        col_mass.push_back(x);
    }   
    return col_mass;
}
} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
