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
        case Value::INTEGER:    return _T("NUMBER");    break;
        case Value::LONGINT:    return _T("NUMBER");    break;
        case Value::STRING:     return _T("VARCHAR2");  break;
        case Value::DATETIME:   return _T("DATE");      break;
        case Value::FLOAT:
        case Value::DECIMAL:    return _T("NUMBER");    break;
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
    Strings s = get_tables(conn);
    for (Strings::iterator i = s.begin(); i != s.end(); ++i)
    {
        if (*i == table)
        {
            return true;
        }
    }
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
    Strings table;
    auto_ptr<SqlCursor> cursor = conn.new_cursor();
    String query = _T("SELECT table_name FROM all_tables where owner = (SELECT USER FROM DUAL)");
    cursor->prepare(query);
    Values params;
    SqlResultSet rs = cursor->exec(params);
    for (SqlResultSet::iterator i = rs.begin(); i != rs.end(); ++i)
    {
        table.push_back((*i)[0].second.as_string());
    }
    return table;
}

Strings
OracleDialect::get_views(SqlConnection &conn)
{
    return Strings();
}

ColumnsInfo
OracleDialect::get_columns(SqlConnection &conn, const String &table)
{
    ColumnsInfo ci;
    auto_ptr<SqlCursor> cursor = conn.new_cursor();
    String query = _T("SELECT col.column_name, col.data_type, col.data_length, col.nullable, col.data_default FROM ALL_TAB_COLUMNS col WHERE col.TABLE_NAME = '") +table
        + _T("' AND col.OWNER = (SELECT USER FROM DUAL)");
    cursor->prepare(query);
    Values params;
    SqlResultSet rs = cursor->exec(params);
    for (SqlResultSet::iterator i = rs.begin(); i != rs.end(); ++i)
    {
        ColumnInfo x;
        for (Row::const_iterator j = i->begin(); j != i->end(); ++j)
        {
            if (_T("COLUMN_NAME") == j->first)
            {
                x.name = str_to_upper(j->second.as_string());
            }
            else if (_T("DATA_TYPE") == j->first)
            {
                x.type = str_to_upper(j->second.as_string());
                if (_T("VARCHAR2") == x.type)
                {
                    ++j;
                    if (_T("DATA_LENGTH") == j->first)
                    {
                        x.size = j->second.as_integer();
                    }
                }
            }
            else if (_T("NULLABLE") == j->first)
            {
                x.notnull = _T("N") == j->second.as_string();
            }
            else if (_T("DATA_DEFAULT") == j->first)
            {
                if (!j->second.is_null())
                    x.default_value = j->second.as_string();
            }
        }
        ci.push_back(x);
    }
    String querypk = _T("SELECT cols.position FROM all_constraints cons, all_cons_columns cols WHERE cols.table_name = '") +table+ _T("' AND cons.constraint_type = 'P' AND cons.constraint_name = cols.constraint_name AND cons.owner = cols.owner");
    cursor->prepare(querypk);
    Values paramspk;
    SqlResultSet rspk = cursor->exec(paramspk);
    for (SqlResultSet::iterator i = rspk.begin(); i != rspk.end(); ++i)
    {
        Row::const_iterator j = i->begin();
        if (j != i-> end())
        {
            ci[j->second.as_integer() - 1].pk = true;
        }
    }

    String q2 =_T("SELECT substr(c_src.COLUMN_NAME, 1, 20) as SRC_COLUMN, c_dest.TABLE_NAME as DEST_TABLE, substr(c_dest.COLUMN_NAME, 1, 20) as DEST_COLUMN FROM ALL_CONSTRAINTS c_list, ALL_CONS_COLUMNS c_src, ALL_CONS_COLUMNS c_dest WHERE c_list.CONSTRAINT_NAME = c_src.CONSTRAINT_NAME AND c_list.R_CONSTRAINT_NAME = c_dest.CONSTRAINT_NAME AND c_list.CONSTRAINT_TYPE = 'R' AND c_src.TABLE_NAME = '")
    + table + _T("'");
    cursor->prepare(q2);
    Values params2;
    SqlResultSet rs2 = cursor->exec(params2);
    for (SqlResultSet::iterator i = rs2.begin(); i != rs2.end(); ++i)
    {
        String fk_column, fk_table, fk_table_key;
        for (Row::const_iterator j = i->begin(); j != i->end(); ++j)
        {
            if (_T("DEST_TABLE") == j->first)
            {
                if (!j->second.is_null())
                {
                    fk_table = j->second.as_string();
                }
            }
            else if (_T("DEST_COLUMN") == j->first)
            {
                if (!j->second.is_null())
                {
                    fk_table_key = j->second.as_string();
                }
            }
            else if (_T("SRC_COLUMN") == j->first)
            {
                if (!j->second.is_null())
                {
                    fk_column = j->second.as_string();
                }
            }
        }
        for (ColumnsInfo::iterator k = ci.begin(); k != ci.end(); ++k)
        {
            if (k->name == fk_column)
            {
                k->fk_table = fk_table;
                k->fk_table_key = fk_table_key;
                break;
            }
        }
    }
    return ci;
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
