// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#define YBORM_SOURCE

#include <algorithm>
#include "dialect_oracle.h"
#include "orm/expression.h"
#include "util/string_utils.h"

namespace Yb {

using namespace std;
using namespace Yb::StrUtils;

OracleDialect::OracleDialect()
    : SqlDialect(_T("ORACLE"), _T("DUAL"), true)
{}

const String 
OracleDialect::select_curr_value(const String &seq_name)
{
    return seq_name + _T(".CURRVAL");
}

const String 
OracleDialect::select_next_value(const String &seq_name)
{
    return seq_name + _T(".NEXTVAL"); 
}

const String 
OracleDialect::sql_value(const Value &x)
{
    if (x.get_type() == Value::DATETIME)
        return _T("timestamp") + x.sql_str();
    return x.sql_str();
}

const String 
OracleDialect::type2sql(int t) {
    switch (t) {
        case Value::INTEGER:    return _T("NUMBER(10)");    break;
        case Value::LONGINT:    return _T("NUMBER(20)");    break;
        case Value::STRING:     return _T("VARCHAR2");      break;
        case Value::DATETIME:   return _T("DATE");          break;
        case Value::FLOAT:
        case Value::DECIMAL:    return _T("NUMBER");        break;
    }
    throw SqlDialectError(_T("Bad type"));
}

const String 
OracleDialect::create_sequence(const String &seq_name) 
{
    return _T("CREATE SEQUENCE ") + seq_name;
}

const String 
OracleDialect::drop_sequence(const String &seq_name) 
{
    return _T("DROP SEQUENCE ") + seq_name;
}

const String 
OracleDialect::sysdate_func() 
{
    return _T("SYSDATE"); 
}

int 
OracleDialect::pager_model() 
{
    return (int)PAGER_ORACLE;
}

// schema introspection

bool 
OracleDialect::table_exists(SqlConnection &conn, const String &table)
{ 
    return false; 
}

bool 
OracleDialect::view_exists(SqlConnection &conn, const String &table)
{ 
    return false; 
}

Strings 
OracleDialect::get_tables(SqlConnection &conn)
{
    return Strings();
}

Strings 
OracleDialect::get_views(SqlConnection &conn)
{ 
    return Strings(); 
}
ColumnsInfo 
OracleDialect::get_columns(SqlConnection &conn, const String &table)
{ 
    return ColumnsInfo(); 
}

// schema introspection

/*static Strings
really_get_tables(SqlConnection &conn, const String &type,
        const String &name, bool filter_system)
{
    Strings tables;
    auto_ptr<SqlCursor> cursor = conn.new_cursor();
    String q = _T("SELECT name FROM sqlite_master WHERE type=?");
    Values params;
    params.push_back(Value(type));
    if (!str_empty(name))
    {
        q += _T(" AND UPPER(name)=UPPER(?)");
        params.push_back(Value(name));
    }
    if (filter_system)
    {
        q += _T(" AND UPPER(name) NOT IN (?)");
        params.push_back(Value(_T("SQLITE_SEQUENCE")));
    }
    cursor->prepare(q);
    SqlResultSet rs = cursor->exec(params);
    for (SqlResultSet::iterator i = rs.begin(); i != rs.end(); ++i)
    {
        tables.push_back(str_to_upper((*i)[0].second.as_string()));
    }
    return tables;
}

bool
SQLite3Dialect::table_exists(SqlConnection &conn, const String &table)
{
    Strings r = really_get_tables(conn, _T("table"), table, false);
    return r.size() == 1;
}

bool
SQLite3Dialect::view_exists(SqlConnection &conn, const String &table)
{
    Strings r = really_get_tables(conn, _T("view"), table, false);
    return r.size() == 1;
}

Strings
SQLite3Dialect::get_tables(SqlConnection &conn)
{
    return really_get_tables(conn, _T("table"), _T(""), true);
}

Strings
SQLite3Dialect::get_views(SqlConnection &conn)
{
    return really_get_tables(conn, _T("view"), _T(""), true);
}

ColumnsInfo
SQLite3Dialect::get_columns(SqlConnection &conn, const String &table)
{
    ColumnsInfo ci;
    auto_ptr<SqlCursor> cursor = conn.new_cursor();
    cursor->prepare(_T("PRAGMA table_info('") + table + _T("')"));
    Values params;
    SqlResultSet rs = cursor->exec(params);
    for (SqlResultSet::iterator i = rs.begin(); i != rs.end(); ++i)
    {
        ColumnInfo x;
        for (Row::const_iterator j = i->begin(); j != i->end(); ++j)
        {
            if (_T("NAME") == j->first)
            {
                x.name = str_to_upper(j->second.as_string());
            }
            else if (_T("TYPE") == j->first)
            {
                x.type = str_to_upper(j->second.as_string());
                int open_par = str_find(x.type, _T('('));
                if (-1 != open_par) {
                    // split type size into its own field
                    String new_type = str_substr(x.type, 0, open_par);
                    try {
                        from_string(str_substr(x.type, open_par + 1,
                                str_length(x.type) - open_par - 2), x.size);
                        x.type = new_type;
                    }
                    catch (const std::exception &) {}
                }
            }
            else if (_T("NOTNULL") == j->first)
            {
                x.notnull = _T("0") != j->second.as_string();
            }
            else if (_T("DFLT_VALUE") == j->first)
            {
                if (!j->second.is_null())
                    x.default_value = j->second.as_string();
            }
            else if (_T("PK") == j->first)
            {
                x.pk = _T("0") != j->second.as_string();
            }
        }
        ci.push_back(x);
    }
    cursor->prepare(_T("PRAGMA foreign_key_list('") + table + _T("')"));
    SqlResultSet rs2 = cursor->exec(params);
    for (SqlResultSet::iterator i = rs2.begin(); i != rs2.end(); ++i)
    {
        String fk_column, fk_table, fk_table_key;
        for (Row::const_iterator j = i->begin(); j != i->end(); ++j)
        {
            if (_T("TABLE") == j->first)
            {
                if (!j->second.is_null())
                    fk_table = j->second.as_string();
            }
            else if (_T("FROM") == j->first)
            {
                if (!j->second.is_null())
                    fk_column = j->second.as_string();
            }
            else if (_T("TO") == j->first)
            {
                if (!j->second.is_null())
                    fk_table_key = j->second.as_string();
            }
        }
        for (ColumnsInfo::iterator k = ci.begin(); k != ci.end(); ++k)
        {
            if (k->name == fk_column) {
                k->fk_table = fk_table;
                k->fk_table_key = fk_table_key;
                break;
            }
        }
    }
    return ci;
}*/

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
