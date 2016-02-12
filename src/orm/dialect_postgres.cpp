// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#define YBORM_SOURCE

#include <algorithm>
#include "dialect_postgres.h"
#include "util/string_utils.h"
#include "orm/expression.h"

namespace Yb {

using namespace std;
using namespace Yb::StrUtils;

PostgresDialect::PostgresDialect()
    : SqlDialect(_T("POSTGRES"), _T(""), true)
{}


const String
PostgresDialect::select_curr_value(const String &seq_name)
{
    return _T("CURRVAL('") + seq_name + _T("')");
}

const String
PostgresDialect::select_next_value(const String &seq_name)
{
    return _T("NEXTVAL('") + seq_name + _T("')");
}

const String
PostgresDialect::sql_value(const Value &x)
{
    return x.sql_str();
}

const String
PostgresDialect::type2sql(int t)
{
    switch (t)
    {
        case Value::INTEGER:    return _T("INTEGER");       break;
        case Value::LONGINT:    return _T("INTEGER");        break;
        case Value::STRING:     return _T("CHARACTER VARYING");       break;
        case Value::DECIMAL:    return _T("NUMERIC");       break;
        case Value::DATETIME:   return _T("TIMESTAMP");     break;
        case Value::FLOAT:      return _T("DOUBLE PRECISION"); break;
    }
    throw SqlDialectError(_T("Bad type"));
}

const String
PostgresDialect::create_sequence(const String &seq_name) {
    return _T("CREATE SEQUENCE ") + seq_name;
}

const String
PostgresDialect::drop_sequence(const String &seq_name) {
    return _T("DROP SEQUENCE ") + seq_name;
}

// schema introspection
bool
PostgresDialect::table_exists(SqlConnection &conn, const String &table)
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
PostgresDialect::view_exists(SqlConnection &conn, const String &table)
{
    return false;
}

Strings
PostgresDialect::get_tables(SqlConnection &conn)
{
    Strings table;
    auto_ptr<SqlCursor> cursor = conn.new_cursor();
    String query = _T("SELECT table_name FROM information_schema.tables "
                      "WHERE table_schema = 'public';");
    cursor->prepare(query);
    Values params;
    SqlResultSet rs = cursor->exec(params);
    for (SqlResultSet::iterator i = rs.begin(); i != rs.end(); ++i)
    {
        table.push_back(str_to_upper((*i)[0].second.as_string()));
    }
    return table;
}

Strings
PostgresDialect::get_views(SqlConnection &conn)
{
    return Strings();
}

ColumnsInfo
PostgresDialect::get_columns(SqlConnection &conn, const String &table)
{
    ColumnsInfo ci;
    Values params;
    auto_ptr<SqlCursor> cursor = conn.new_cursor();
    String query = _T("SELECT * FROM information_schema.columns WHERE table_name = '")
        + str_to_lower(table) + _T("'");
    cursor->prepare(query);
    SqlResultSet rs = cursor->exec(params);
    for (SqlResultSet::iterator i = rs.begin(); i != rs.end(); ++i)
    {
        ColumnInfo x;
        for (Row::const_iterator j = i->begin(); j != i->end(); ++j)
        {
            if (_T("COLUMN_NAME") == str_to_upper(j->first))
            {
                x.name = str_to_upper(j->second.as_string());
            }
            else if (_T("DATA_TYPE") == str_to_upper(j->first))
            {
                if (starts_with(str_to_upper(j->second.as_string()),
                            _T("TIMESTAMP")))
                {
                    x.type = _T("TIMESTAMP");
                }
                else
                {
                    x.type = str_to_upper(j->second.as_string());
                }
                int open_par = str_find(x.type, _T('('));
                if (-1 != open_par) {
                    String new_type = str_substr(x.type, 0, open_par);
                    try {
                        from_string(str_substr(x.type, open_par + 1,
                                str_length(x.type) - open_par - 2), x.size);
                        x.type = new_type;
                    }
                    catch (const std::exception &) {}
                }
            }
            else if (_T("IS_NULLABLE") == str_to_upper(j->first))
            {
                  x.notnull = _T("NO") == str_to_upper(j->second.as_string());
            }
            else if (_T("COLUMN_DEFAULT") == str_to_upper(j->first))
            {
                if (!j->second.is_null())
                {
                    if (str_to_upper(j->second.as_string()) == _T("NEXTVAL('T_ORM_TEST_ID_SEQ'::REGCLASS)"))
                    {
                        x.default_value = String();
                    }
                    else if (str_to_upper(j->second.as_string()) == _T("NEXTVAL('T_ORM_XML_ID_SEQ'::REGCLASS)"))
                    {
                        x.default_value = String();
                    }
                    else if (str_to_upper(j->second.as_string()) == _T("NOW()"))
                    {
                        x.default_value = _T("CURRENT_TIMESTAMP");
                    }
                    else
                    {
                        x.default_value = str_to_upper(j->second.as_string());
                    }
                }
            }
            else if (_T("CHARACTER_MAXIMUM_LENGTH") == str_to_upper(j->first))
            {
                if (!j->second.is_null())
                    x.size = j->second.as_integer();
            }
        }
        ci.push_back(x);
    }
    cursor.reset(NULL);
    cursor.reset(conn.new_cursor().release());
    String queryPk =
        _T("SELECT c.column_name")
        _T(" FROM information_schema.table_constraints tc")
        _T(" JOIN information_schema.constraint_column_usage AS ccu")
        _T(" USING (constraint_schema, constraint_name)")
        _T(" JOIN information_schema.columns AS c")
        _T(" ON c.table_schema = tc.constraint_schema")
        _T(" AND tc.table_name = c.table_name")
        _T(" AND ccu.column_name = c.column_name")
        _T(" WHERE constraint_type = 'PRIMARY KEY' and tc.table_name = '")
        + str_to_lower(table) + _T("'");
    cursor->prepare(queryPk);
    SqlResultSet rs2 = cursor->exec(params);
    for (SqlResultSet::iterator i = rs2.begin(); i != rs2.end(); ++i)
    {
        String pk_name = (*i)[0].second.as_string();
        for (ColumnsInfo::iterator j = ci.begin(); j != ci.end(); ++j)
            if (pk_name == j->name) {
                j->pk = true;
                break;
            }
    }
    cursor.reset(NULL);
    cursor.reset(conn.new_cursor().release());
    String qFk =
        _T("SELECT tc.constraint_name, tc.table_name, kcu.column_name,")
        _T(" ccu.table_name foreign_table_name, ccu.column_name foreign_column_name")
        _T(" FROM information_schema.table_constraints tc")
        _T(" JOIN information_schema.key_column_usage kcu")
        _T(" ON tc.constraint_name = kcu.constraint_name")
        _T(" JOIN information_schema.constraint_column_usage ccu")
        _T(" ON ccu.constraint_name = tc.constraint_name")
        _T(" WHERE constraint_type = 'FOREIGN KEY' AND tc.table_name= '")
        + str_to_lower(table) + _T("'");
    cursor->prepare(qFk);
    SqlResultSet rs3 = cursor->exec(params);
    for (SqlResultSet::iterator i = rs3.begin(); i != rs3.end(); ++i)
    {
        String fk_column, fk_table, fk_table_key;
        for (Row::const_iterator j = i->begin(); j != i->end(); ++j)
        {
            if (_T("FOREIGN_TABLE_NAME") == str_to_upper(j->first))
            {
               if (!j->second.is_null())
                    fk_table = str_to_upper(j->second.as_string());
            }
            else if (_T("FOREIGN_COLUMN_NAME") == str_to_upper(j->first))
            {
               if (!j->second.is_null())
                    fk_table_key = str_to_upper(j->second.as_string());
            }
            else if (_T("COLUMN_NAME") == str_to_upper(j->first))
            {
                if (!j->second.is_null())
                    fk_column = str_to_upper(j->second.as_string());
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
