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
    return really_get_tables(conn, table, false, true).size() != 0;
}

bool
InterbaseDialect::view_exists(SqlConnection &conn, const String &table)
{
    return really_get_tables(conn, table, true, true).size() != 0;
}

Strings
InterbaseDialect::get_tables(SqlConnection &conn)
{
    return really_get_tables(conn, String(), false, false);
}

Strings
InterbaseDialect::get_views(SqlConnection &conn)
{
    return really_get_tables(conn, String(), true, false);
}

Strings
InterbaseDialect::really_get_tables(SqlConnection &conn,
        const String &table, bool view, bool show_system)
{
    Strings tables;
    auto_ptr<SqlCursor> cursor = conn.new_cursor();
    String query = _T("SELECT R.RDB$RELATION_NAME FROM RDB$RELATIONS R")
                   _T(" WHERE 1=1");
    if (!show_system)
        query += _T(" AND R.RDB$SYSTEM_FLAG = 0");
    if (!view)
        query += _T(" AND R.RDB$VIEW_SOURCE IS NULL");
    else
        query += _T(" AND R.RDB$VIEW_SOURCE IS NOT NULL");
    if (!str_empty(table))
        query += _T(" AND UPPER(R.RDB$RELATION_NAME) = UPPER('")
            + table + _T("')");
    cursor->prepare(query);
    Values params;
    SqlResultSet rs = cursor->exec(params);
    for (SqlResultSet::iterator i = rs.begin(); i != rs.end(); ++i)
    {
        tables.push_back(trim_trailing_space((*i)[0].second.as_string()));
    }
    return tables;
}

ColumnsInfo
InterbaseDialect::get_columns(SqlConnection &conn, const String &table)
{
    ColumnsInfo col_mass;
    auto_ptr<SqlCursor> cursor = conn.new_cursor();

    String query =
        _T("SELECT RF.RDB$FIELD_NAME NAME, T1.RDB$TYPE_NAME TYPE_NAME,")
        _T(" RF.RDB$DEFAULT_SOURCE DFLT, F.RDB$FIELD_LENGTH SZ,")
        _T(" F.RDB$FIELD_PRECISION PREC, F.RDB$FIELD_SCALE SCALE,")
        _T(" RF.RDB$NULL_FLAG NOT_NULL")
        _T(" FROM RDB$RELATIONS R")
        _T(" JOIN RDB$RELATION_FIELDS RF")
        _T(" ON R.RDB$RELATION_NAME = RF.RDB$RELATION_NAME")
        _T(" JOIN RDB$FIELDS F ON RF.RDB$FIELD_SOURCE = F.RDB$FIELD_NAME")
        _T(" JOIN RDB$TYPES T1 ON F.RDB$FIELD_TYPE = T1.RDB$TYPE")
        _T(" AND T1.RDB$FIELD_NAME = 'RDB$FIELD_TYPE'")
        _T(" WHERE R.RDB$RELATION_NAME='") + table + _T("'")
        _T(" ORDER BY RF.RDB$FIELD_POSITION");
    cursor->prepare(query);
    Values params;
    SqlResultSet rs = cursor->exec(params);
    for (SqlResultSet::iterator i = rs.begin(); i != rs.end(); ++i)
    {
        ColumnInfo x;
        int prec = 0, scale = 0;
        for (Row::const_iterator j = i->begin(); j != i->end(); ++j)
        {
            if (_T("NAME") == j->first)
            {
                x.name = str_to_upper(trim_trailing_space(j->second.as_string()));
            }
            else if (_T("TYPE_NAME") == j->first)
            {
                x.type = str_to_upper(trim_trailing_space(j->second.as_string()));
                if (_T("VARYING") == x.type)
                    x.type = _T("VARCHAR");
                else if (_T("LONG") == x.type)
                    x.type = _T("INTEGER");
                else if (_T("INT64") == x.type)
                    x.type = _T("BIGINT");
                else if (_T("DOUBLE") == x.type)
                    x.type = _T("DOUBLE PRECISION");
            }
            else if (_T("DFLT") == j->first)
            {
                if (!j->second.is_null()) {
                    x.default_value = trim_trailing_space(j->second.as_string());
                    if (starts_with(x.default_value, _T("DEFAULT ")))
                        x.default_value = str_substr(x.default_value,
                                str_length(_T("DEFAULT ")));
                }
            }
            else if (_T("SZ") == j->first)
            {
                if (!j->second.is_null() && _T("VARCHAR") == x.type)
                    x.size = j->second.as_integer();
            }
            else if (_T("PREC") == j->first)
            {
                if (!j->second.is_null())
                    prec = j->second.as_integer();
            }
            else if (_T("SCALE") == j->first)
            {
                if (!j->second.is_null())
                    scale = -j->second.as_integer();
            }
            else if (_T("NOT_NULL") == j->first)
            {
                if (j->second.is_null())
                    x.notnull = false;
                else
                    x.notnull = j->second.as_integer() != 0;
            }
        }
        if (_T("BIGINT") == x.type && prec && scale) {
            x.type = _T("DECIMAL(") + to_string(prec) + _T(", ")
                + to_string(scale) + _T(")");
        }
        col_mass.push_back(x);
    }
    String query2 = _T("SELECT s.RDB$FIELD_NAME COLUMN_NAME, ")
                    _T("rc.RDB$CONSTRAINT_TYPE CON_TYPE, i2.RDB$RELATION_NAME FK_TABLE, ")
                    _T("s2.RDB$FIELD_NAME FK_TABLE_KEY,(s.RDB$FIELD_POSITION + 1) ")
                    _T("FROM RDB$INDEX_SEGMENTS s ")
                    _T("LEFT JOIN RDB$INDICES i ON i.RDB$INDEX_NAME = s.RDB$INDEX_NAME ")
                    _T("LEFT JOIN RDB$RELATION_CONSTRAINTS rc ON rc.RDB$INDEX_NAME = s.RDB$INDEX_NAME ")
                    _T("LEFT JOIN RDB$REF_CONSTRAINTS refc ON rc.RDB$CONSTRAINT_NAME = refc.RDB$CONSTRAINT_NAME ")
                    _T("LEFT JOIN RDB$RELATION_CONSTRAINTS rc2 ON rc2.RDB$CONSTRAINT_NAME = refc.RDB$CONST_NAME_UQ ")
                    _T("LEFT JOIN RDB$INDICES i2 ON i2.RDB$INDEX_NAME = rc2.RDB$INDEX_NAME ")
                    _T("LEFT JOIN RDB$INDEX_SEGMENTS s2 ON i2.RDB$INDEX_NAME = s2.RDB$INDEX_NAME ")
                    _T("WHERE i.RDB$RELATION_NAME = '") + table + _T("' ")
                    _T("AND rc.RDB$CONSTRAINT_TYPE = 'FOREIGN KEY'");
    cursor->prepare(query2);
    Values params2;
    SqlResultSet rs2 = cursor->exec(params2);
    for (SqlResultSet::iterator i = rs2.begin(); i != rs2.end(); ++i)
    {
        String fk_column, fk_table, fk_table_key;
        for (Row::const_iterator j = i->begin(); j != i->end(); ++j)
        {
            if (_T("FK_TABLE") == j->first)
            {
                if (!j->second.is_null()) {
                    fk_table = trim_trailing_space(j->second.as_string());
                }
            }
            else if (_T("FK_TABLE_KEY") == j->first)
            {
                if (!j->second.is_null()) {
                    fk_table_key = trim_trailing_space(j->second.as_string());
                }
            }
            else if (_T("COLUMN_NAME") == j->first)
            {
                if (!j->second.is_null()) {
                    fk_column = trim_trailing_space(j->second.as_string());
                }
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
    String query3 = _T("SELECT i.RDB$FIELD_NAME ")
                    _T("FROM RDB$RELATION_CONSTRAINTS rc INNER JOIN RDB$INDEX_SEGMENTS i ")
                    _T("ON rc.RDB$INDEX_NAME = i.RDB$INDEX_NAME ")
                    _T("WHERE rc.RDB$CONSTRAINT_TYPE='PRIMARY KEY' ")
                    _T("AND rc.RDB$RELATION_NAME = '") + table + _T("';");
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
                if (k->name == trim_trailing_space(j->second.as_string()))
                    k->pk = true;
            }
        }
    }
    return col_mass;
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
