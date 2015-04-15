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
        //case Value::DECIMAL:    return _T("DECIMAL(16, 6)"); break;
        case Value::DECIMAL:    return _T("BIGINT");        break;
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
            return true;
    }
    return false;
}

bool 
InterbaseDialect::view_exists(SqlConnection &conn, const String &table)
{ 
    return false; 
}

String
InterbaseDialect::code2type(int code)
{   
    switch(code)
    {
        case 16: return _T("BIGINT"); break;
        case 8:  return _T("INTEGER"); break;
        case 37: return _T("VARCHAR"); break;
        case 14: return _T("CHAR"); break;
        case 35: return _T("TIMESTAMP"); break;
        case 27: return _T("DOUBLE PRECISION"); break;
        default: return _T("UNKNOW TYPE");
    }
}

String corrector_string(String str)
{
    for (int i = 0; i < str.size(); ++i)
        if (str[i] == ' ')
            str.resize(i);
    return str;
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
    String tmp;
    for (SqlResultSet::iterator i = rs.begin(); i != rs.end(); ++i) 
    {
        tables.push_back(str_to_upper(corrector_string((*i)[0].second.as_string())));
    }
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
                   _T("RDB$FIELDS.RDB$FIELD_LENGTH, ")
                   _T("RDB$RELATION_FIELDS.RDB$NULL_FLAG, RDB$RELATION_FIELDS.RDB$DEFAULT_SOURCE, ")
                   _T("RDB$FIELDS.RDB$FIELD_TYPE FROM RDB$RELATIONS INNER JOIN RDB$RELATION_FIELDS ON ")
                   _T("(RDB$RELATIONS.RDB$RELATION_NAME = RDB$RELATION_FIELDS.RDB$RELATION_NAME) ")
                   _T("INNER JOIN RDB$FIELDS ON (RDB$RELATION_FIELDS.RDB$FIELD_SOURCE = ")
                   _T("RDB$FIELDS.RDB$FIELD_NAME) WHERE RDB$RELATIONS.RDB$RELATION_NAME = '") 
                    + table + 
                   _T("';");
    cursor->prepare(query);
    Values params;
    SqlResultSet rs = cursor->exec(params);
    for (SqlResultSet::iterator i = rs.begin(); i != rs.end(); ++i)
    {
        ColumnInfo x;
        String tmp;
        for (Row::const_iterator j = i->begin(); j != i->end(); ++j)
        {
            if (_T("RDB$FIELD_NAME") == j->first)
            {
                x.name = str_to_upper(corrector_string(j->second.as_string()));
                continue;
            }
            if (_T("RDB$FIELD_TYPE") == j->first)
            {
                x.type = str_to_upper(code2type(j->second.as_integer()));
                continue;
            }
            if (_T("RDB$DEFAULT_SOURCE") == j->first)
            {
                if (!j->second.is_null())
                {   
                    x.default_value = corrector_string(j->second.as_string().substr(8, j->second.as_string().size()));                
                }
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
            /*if (_T("RDB$FIELD_LENGTH") == j->first)
            {
                x.size = j->second.as_integer();
                continue;
            }*/
        }
        col_mass.push_back(x);
    }
    cout << "\n\n\n\nSTART TEST:\n";
    cout << "len col mass: " << col_mass.size() << endl;
    cout << "len col[1].size" << col_mass[1].size <<  endl;
    cout << "\n\nEND TEST\n\n";
    String query2 = _T("SELECT s.RDB$FIELD_NAME  AS COLUMN_NAME, rc.RDB$CONSTRAINT_TYPE, i2.RDB$RELATION_NAME, ")
                    _T("s2.RDB$FIELD_NAME,(s.RDB$FIELD_POSITION + 1) FROM RDB$INDEX_SEGMENTS s ")
                    _T("LEFT JOIN RDB$INDICES i ON i.RDB$INDEX_NAME = s.RDB$INDEX_NAME ")
                    _T("LEFT JOIN RDB$RELATION_CONSTRAINTS rc ON rc.RDB$INDEX_NAME = s.RDB$INDEX_NAME ")
                    _T("LEFT JOIN RDB$REF_CONSTRAINTS refc ON rc.RDB$CONSTRAINT_NAME = refc.RDB$CONSTRAINT_NAME ")
                    _T("LEFT JOIN RDB$RELATION_CONSTRAINTS rc2 ON rc2.RDB$CONSTRAINT_NAME = refc.RDB$CONST_NAME_UQ ")
                    _T("LEFT JOIN RDB$INDICES i2 ON i2.RDB$INDEX_NAME = rc2.RDB$INDEX_NAME ")
                    _T("LEFT JOIN RDB$INDEX_SEGMENTS s2 ON i2.RDB$INDEX_NAME = s2.RDB$INDEX_NAME ")
                    _T("WHERE i.RDB$RELATION_NAME='") + table + _T("' ")
                    _T("AND rc.RDB$CONSTRAINT_TYPE='FOREIGN KEY'");
    cursor->prepare(query2);
    Values params2;
    SqlResultSet rs2 = cursor->exec(params2);
    for (SqlResultSet::iterator i = rs2.begin(); i != rs2.end(); ++i)
    {
        String fk_column, fk_table, fk_table_key;
        for (Row::const_iterator j = i->begin(); j != i->end(); ++j)
        {
            if (_T("RDB$RELATION_NAME") == j->first)
            {
                if (!j->second.is_null()) {
                    fk_table = corrector_string(j->second.as_string());
                }
                continue;
            }
            if (_T("RDB$FIELD_NAME") == j->first)
            {
                if (!j->second.is_null()) {
                    fk_table_key = corrector_string(j->second.as_string());
                }
                continue;
            }
            if (_T("COLUMN_NAME") == j->first)
            {
                if (!j->second.is_null()) {
                    fk_column = corrector_string(j->second.as_string());
                }
                continue;
            }
        }

        for (ColumnsInfo::iterator k = col_mass.begin(); k != col_mass.end(); ++k)
        {        
            if (k->name == fk_column)
            {
                k->fk_table = fk_table;
                k->fk_table_key = fk_table_key;
                break;
            }
        }
    }
    String query3 = _T("SELECT RDB$INDEX_SEGMENTS.RDB$FIELD_NAME ")
                    _T("FROM RDB$RELATION_CONSTRAINTS INNER JOIN RDB$INDEX_SEGMENTS ON ")
                    _T("RDB$RELATION_CONSTRAINTS.RDB$INDEX_NAME=RDB$INDEX_SEGMENTS.RDB$INDEX_NAME ")
                    _T("WHERE RDB$RELATION_CONSTRAINTS.RDB$CONSTRAINT_TYPE='PRIMARY KEY' ")
                    _T("AND RDB$RELATION_CONSTRAINTS.RDB$RELATION_NAME='")
                    + table +
                    _T("';");
    cursor->prepare(query3);
    Values params3;
    SqlResultSet rs3 = cursor->exec(params3);
    for (ColumnsInfo::iterator k = col_mass.begin(); k != col_mass.end(); ++k)
    {
        k->pk = false;        
        for (SqlResultSet::iterator i = rs3.begin(); i != rs3.end(); ++i)
        {
            for (Row::const_iterator j = i->begin(); j != i->end(); ++j)
            {
                if (k->name == corrector_string(j->second.as_string()))
                    k->pk = true;
            }
        }
    }
    return col_mass;
}
} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
