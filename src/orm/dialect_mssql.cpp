// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#define YBORM_SOURCE

#include <algorithm>
#include "dialect_mssql.h"
#include "util/string_utils.h"
#include "orm/expression.h"

namespace Yb {

using namespace std;
using namespace Yb::StrUtils;

MssqlDialect::MssqlDialect()
    : SqlDialect(_T("MSSQL"), _T(""), false)
{}

const String
MssqlDialect::select_curr_value(const String &seq_name)
{
    throw SqlDialectError(_T("No sequences, please"));
}

const String
MssqlDialect::select_next_value(const String &seq_name)
{
    throw SqlDialectError(_T("No sequences, please"));
}

const String
MssqlDialect::select_last_inserted_id(const String &table_name)
{
    return _T("SELECT SCOPE_IDENTITY()");
}

const String
MssqlDialect::sql_value(const Value &x)
{
    return x.sql_str();
}

const String
MssqlDialect::grant_insert_id_statement(const String &table_name, bool on)
{
    return _T("SET IDENTITY_INSERT ") + table_name
        + (on? _T(" ON"): _T(" OFF")) + _T(";");
}

const String
MssqlDialect::type2sql(int t)
{
    switch (t)
    {
        case Value::INTEGER:    return _T("INT");           break;
        case Value::LONGINT:    return _T("BIGINT");        break;
        case Value::STRING:     return _T("VARCHAR");       break;
        case Value::DECIMAL:    return _T("DECIMAL(16, 6)"); break;
        case Value::DATETIME:   return _T("DATETIME");      break;
        case Value::FLOAT:      return _T("FLOAT"); break;
    }
    throw SqlDialectError(_T("Bad type"));
}

const String
MssqlDialect::create_sequence(const String &seq_name)
{
    throw SqlDialectError(_T("No sequences, please"));
}

const String
MssqlDialect::drop_sequence(const String &seq_name)
{
    throw SqlDialectError(_T("No sequences, please"));
}

const String
MssqlDialect::suffix_create_table()
{
    return String();
}

const String
MssqlDialect::autoinc_flag()
{
    return _T("IDENTITY(1,1)");
}

bool
MssqlDialect::explicit_null()
{
    return true;
}

int
MssqlDialect::pager_model()
{
    return (int)PAGER_ORACLE;
}

// schema introspection

bool
MssqlDialect::table_exists(SqlConnection &conn, const String &table)
{
    return really_get_tables(conn, table, false, true).size() != 0;
}

bool
MssqlDialect::view_exists(SqlConnection &conn, const String &table)
{
    return really_get_tables(conn, table, true, true).size() != 0;
}

Strings
MssqlDialect::get_tables(SqlConnection &conn)
{
    return really_get_tables(conn, String(), false, false);
}

Strings
MssqlDialect::get_views(SqlConnection &conn)
{
    return really_get_tables(conn, String(), true, false);
}

Strings
MssqlDialect::really_get_tables(SqlConnection &conn,
        const String &table, bool view, bool show_system)
{
    auto_ptr<SqlCursor> cursor = conn.new_cursor();
    String query = _T("SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE 1=1");
    if (!view)
        query += _T(" AND TABLE_TYPE <> 'VIEW'");
    else
        query += _T(" AND TABLE_TYPE = 'VIEW'");
    if (!str_empty(table))
        query += _T(" AND UPPER(TABLE_NAME) = UPPER('") + table + _T("')");
    Strings tables;
    cursor->prepare(query);
    Values params;
    SqlResultSet rs = cursor->exec(params);
    for (SqlResultSet::iterator i = rs.begin(); i != rs.end(); ++i)
    {
        tables.push_back((*i)[0].second.as_string());
    }
    return tables;
}

ColumnsInfo
MssqlDialect::get_columns(SqlConnection &conn, const String &table)
{
    ColumnsInfo ci;
    auto_ptr<SqlCursor> cursor = conn.new_cursor();
    String q1 = _T("SELECT COLUMN_NAME, DATA_TYPE, COLUMN_DEFAULT,")
        _T(" IS_NULLABLE, CHARACTER_MAXIMUM_LENGTH SZ,")
        _T(" NUMERIC_PRECISION PREC, NUMERIC_SCALE SCALE")
        _T(" FROM INFORMATION_SCHEMA.COLUMNS")
        _T(" WHERE TABLE_NAME = '") + table + _T("'");
    cursor->prepare(q1);
    Values params1;
    SqlResultSet rs1 = cursor->exec(params1);
    for (SqlResultSet::iterator i = rs1.begin(); i != rs1.end(); ++i)
    {
        ColumnInfo x;
        int prec = 0, scale = 0;
        for (Row::const_iterator j = i->begin(); j != i->end(); ++j)
        {
            if (_T("COLUMN_NAME") == j->first)
            {
                x.name = str_to_upper(j->second.as_string());
            }
            else if (_T("DATA_TYPE") == j->first)
            {
                x.type = str_to_upper(j->second.as_string());
            }
            else if (_T("SZ") == j->first && !j->second.is_null())
            {
                if (x.type == _T("VARCHAR"))
                    x.size = j->second.as_integer();
            }
            else if (_T("COLUMN_DEFAULT") == j->first && !j->second.is_null())
            {
                x.default_value = j->second.as_string();
                while (_T("(") == str_substr(x.default_value, 0, 1)
                        && _T(")") == str_substr(x.default_value,
                        str_length(x.default_value) - 1, 1))
                {
                    x.default_value = str_substr(
                            x.default_value, 1, str_length(x.default_value) - 2);
                }
                if (x.type == _T("DATETIME") && x.default_value == _T("getdate()"))
                    x.default_value = _T("CURRENT_TIMESTAMP");
            }
            else if (_T("IS_NULLABLE") == j->first && !j->second.is_null())
            {
                x.notnull = (j->second.as_string() != _T("YES"));
            }
            else if (_T("PREC") == j->first)
            {
                if (!j->second.is_null())
                    prec = j->second.as_integer();
            }
            else if (_T("SCALE") == j->first)
            {
                if (!j->second.is_null())
                    scale = j->second.as_integer();
            }
        }
        if (_T("DECIMAL") == x.type && prec && scale) {
            x.type = _T("DECIMAL(") + to_string(prec) + _T(", ")
                + to_string(scale) + _T(")");
        }
        ci.push_back(x);
    }
    String q2 = _T("SELECT Col.Column_Name, Tab.CONSTRAINT_TYPE, RC2.TABLE_NAME")
        _T(" from INFORMATION_SCHEMA.TABLE_CONSTRAINTS Tab")
        _T(" JOIN INFORMATION_SCHEMA.CONSTRAINT_COLUMN_USAGE Col")
        _T(" ON (Col.Constraint_Name = Tab.Constraint_Name")
        _T(" AND Col.Table_Name = Tab.Table_Name)")
        _T(" LEFT JOIN INFORMATION_SCHEMA.REFERENTIAL_CONSTRAINTS RC")
        _T(" ON Col.CONSTRAINT_NAME = RC.CONSTRAINT_NAME")
        _T(" LEFT JOIN INFORMATION_SCHEMA.TABLE_CONSTRAINTS RC2")
        _T(" ON RC.UNIQUE_CONSTRAINT_NAME = RC2.CONSTRAINT_NAME")
        _T(" WHERE Tab.Constraint_Type IN ('PRIMARY KEY', 'FOREIGN KEY')")
        _T(" AND Col.Table_Name = '") + table + _T("'");
    cursor->prepare(q2);
    Values params2;
    SqlResultSet rs2 = cursor->exec(params2);
    for (SqlResultSet::iterator i = rs2.begin(); i != rs2.end(); ++i)
    {
        String key_name = str_to_upper((*i)[0].second.as_string()),
            key_type = str_to_upper((*i)[1].second.as_string()),
            fk_table = (*i)[2].second.nvl(String()).as_string();
        for (ColumnsInfo::iterator j = ci.begin(); j != ci.end(); ++j)
        {
            if (j->name == key_name)
            {
                if (_T("PRIMARY KEY") == key_type)
                {
                    j->pk = true;
                }
                else if (_T("FOREIGN KEY") == key_type)
                {
                    j->fk_table = fk_table;
                }
                break;
            }
        }
    }
    return ci;
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
