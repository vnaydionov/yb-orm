// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#define YBORM_SOURCE

#include <algorithm>
#include "dialect_mysql.h"
#include "util/string_utils.h"
#include "orm/expression.h"

namespace Yb {

using namespace std;
using namespace Yb::StrUtils;

MysqlDialect::MysqlDialect()
    : SqlDialect(_T("MYSQL"), _T("DUAL"), false)
{}

const String
MysqlDialect::select_curr_value(const String &seq_name)
{
    throw SqlDialectError(_T("No sequences, please"));
}

const String
MysqlDialect::select_next_value(const String &seq_name)
{
    throw SqlDialectError(_T("No sequences, please"));
}

const String
MysqlDialect::select_last_inserted_id(const String &table_name)
{
    return _T("SELECT LAST_INSERT_ID() LID");
}

const String
MysqlDialect::sql_value(const Value &x)
{
    return x.sql_str();
}
const String
MysqlDialect::type2sql(int t)
{
    switch (t)
    {
        case Value::INTEGER:    return _T("INT");           break;
        case Value::LONGINT:    return _T("BIGINT");        break;
        case Value::STRING:     return _T("VARCHAR");       break;
        case Value::DECIMAL:    return _T("DECIMAL(16,6)"); break;
        case Value::DATETIME:   return _T("TIMESTAMP");     break;
        case Value::FLOAT:      return _T("DOUBLE"); break;
    }
    throw SqlDialectError(_T("Bad type"));
}

const String
MysqlDialect::create_sequence(const String &seq_name)
{
    throw SqlDialectError(_T("No sequences, please"));
}

const String
MysqlDialect::drop_sequence(const String &seq_name)
{
    throw SqlDialectError(_T("No sequences, please"));
}

const String
MysqlDialect::suffix_create_table()
{
    return _T(" ENGINE=INNODB DEFAULT CHARSET=utf8");
}

const String
MysqlDialect::autoinc_flag()
{
    return _T("AUTO_INCREMENT");
}

bool
MysqlDialect::explicit_null()
{
    return true;
}

const String
MysqlDialect::not_null_default(const String &not_null_clause,
        const String &default_value)
{
    if (str_empty(not_null_clause))
        return default_value;
    if (str_empty(default_value))
        return not_null_clause;
    return not_null_clause + _T(" ") + default_value;
}

int
MysqlDialect::pager_model()
{
    return (int)PAGER_MYSQL;
}

// schema introspection

bool
MysqlDialect::table_exists(SqlConnection &conn, const String &table)
{
    return really_get_tables(conn, table, false, true).size() != 0;
}

bool
MysqlDialect::view_exists(SqlConnection &conn, const String &table)
{
    return really_get_tables(conn, table, true, true).size() != 0;
}

Strings
MysqlDialect::get_tables(SqlConnection &conn)
{
    return really_get_tables(conn, String(), false, false);
}

Strings
MysqlDialect::get_views(SqlConnection &conn)
{
    return really_get_tables(conn, String(), true, false);
}

Strings
MysqlDialect::really_get_tables(SqlConnection &conn,
        const String &table, bool view, bool show_system)
{
    auto_ptr<SqlCursor> cursor = conn.new_cursor();
    String query = _T("SHOW TABLE STATUS WHERE 1=1");
    if (!view)
        query += _T(" AND Comment != 'VIEW'");
    else
        query += _T(" AND Comment = 'VIEW'");
    if (!str_empty(table))
        query += _T(" AND UPPER(NAME) = UPPER('") + table + _T("')");
    Strings tables;
    cursor->prepare(query);
    Values params;
    SqlResultSet rs = cursor->exec(params);
    for (SqlResultSet::iterator i = rs.begin(); i != rs.end(); ++i)
    {
        // do not force upper case here:
        tables.push_back((*i)[0].second.as_string());
    }
    return tables;
}

ColumnsInfo
MysqlDialect::get_columns(SqlConnection &conn, const String &table)
{
    ColumnsInfo ci;
    auto_ptr<SqlCursor> cursor = conn.new_cursor();
    String query = _T("SHOW COLUMNS FROM ") + table;
    cursor->prepare(query);
    Values params;
    SqlResultSet rs = cursor->exec(params);

    for (SqlResultSet::iterator i = rs.begin(); i != rs.end(); ++i)
    {
        ColumnInfo x;
        for (Row::const_iterator j = i->begin(); j != i->end(); ++j)
        {
            if (_T("FIELD") == j->first)
            {
                x.name = str_to_upper(j->second.as_string());
                //cout << "Field \n";
            }
            else if (_T("TYPE") == j->first)
            {
                x.type = str_to_upper(j->second.as_string());
                int open_par = str_find(x.type, _T('('));
                //cout << "Type \n";
                if (-1 != open_par) {
                    // split type size into its own field
                    String new_type = str_substr(x.type, 0, open_par);
                    if (_T("INT") == new_type
                            || _T("BIGINT") == new_type
                            || _T("TIMESTAMP") == new_type
                            || _T("DOUBLE") == new_type)
                    {
                        x.type = new_type;
                    }
                    else if (_T("DECIMAL") == new_type)
                    {
                        // do nothing
                    }
                    else
                    {
                        try {
                            from_string(str_substr(x.type, open_par + 1,
                                    str_length(x.type) - open_par - 2), x.size);
                            x.type = new_type;
                        }
                        catch (const std::exception &) {}
                    }
                }
            }
            else if (_T("NULL") == j->first)
            {
                x.notnull = _T("NO") == j->second.as_string();
                //cout << "Null \n";
            }
            else if (_T("DEFAULT") == j->first)
            {
                if (!j->second.is_null())
                    x.default_value = j->second.as_string();
                    //cout << "Default\n";
            }
            else if (_T("KEY") == j->first)
            {
                x.pk = _T("PRI") == j->second.as_string();
                //cout << "Key\n";
            }
            /*It is unclear how to add structure Exstra
            else if (_T("Exstra") == j->first)
            {
                x.pk = _T("0") != j->second.as_string();
            }*/
        }

        //cout << "\nstok \n";
        ci.push_back(x);
    }

    String q2 =_T("select COLUMN_NAME, REFERENCED_TABLE_NAME,REFERENCED_COLUMN_NAME ")
               _T("from information_schema.KEY_COLUMN_USAGE ")
               _T("where TABLE_SCHEMA=(select schema()from dual) and TABLE_NAME='")
               + table +
               _T("' and CONSTRAINT_NAME<> 'PRIMORY' and REFERENCED_TABLE_NAME is not null");
    cursor->prepare(q2);
    Values params2;
    SqlResultSet rs2 = cursor->exec(params2);
    for (SqlResultSet::iterator i = rs2.begin(); i != rs2.end(); ++i)
    {
        String fk_column, fk_table, fk_table_key;
        for (Row::const_iterator j = i->begin(); j != i->end(); ++j)
        {
            //cout << "\nkey\n";
            if (_T("REFERENCED_TABLE_NAME") == j->first)
            {
                //cout << "\ntable \n";
                if (!j->second.is_null()) {
                    fk_table = j->second.as_string();
                    //cout << "\n table2 \n";
                }
            }
            else if (_T("REFERENCED_COLUMN_NAME") == j->first)
            {
                //cout << "\n name \n";
                if (!j->second.is_null()) {
                    fk_table_key = j->second.as_string();
                    //cout << "\nname2 \n";
                }
            }
            else if (_T("COLUMN_NAME") == j->first)
            {
                //cout << "\n non \n";
                if (!j->second.is_null()) {
                    fk_column = j->second.as_string();
                    //cout << "\nnon2 \n";
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
