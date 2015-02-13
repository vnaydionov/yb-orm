// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-

#include "orm/dialect_sqlite.h"

namespace Yb {

SQLite3Dialect::SQLite3Dialect()
    : SqlDialect(_T("SQLITE"), _T(""), false)
{}

bool
SQLite3Dialect::native_driver_eats_slash()
{
    return false;
}

const String
SQLite3Dialect::select_curr_value(const String &seq_name)
{
    throw SqlDialectError(_T("No sequences, please"));
}

const String
SQLite3Dialect::select_next_value(const String &seq_name)
{
    throw SqlDialectError(_T("No sequences, please"));
}

const String
SQLite3Dialect::select_last_inserted_id(const String &table_name)
{
    return _T("SELECT SEQ LID FROM SQLITE_SEQUENCE WHERE NAME = '")
        + table_name + _T("'");
}

const String
SQLite3Dialect::sql_value(const Value &x)
{
    return x.sql_str();
}

const String
SQLite3Dialect::type2sql(int t)
{
    switch (t) {
        case Value::INTEGER:    return _T("INTEGER");       break;
        case Value::LONGINT:    return _T("INTEGER");       break;
        case Value::STRING:     return _T("VARCHAR");       break;
        case Value::DECIMAL:    return _T("NUMERIC");       break;
        case Value::DATETIME:   return _T("TIMESTAMP");     break;
        case Value::FLOAT:      return _T("DOUBLE PRECISION"); break;
    }
    throw SqlDialectError(_T("Bad type"));
}

bool
SQLite3Dialect::fk_internal()
{
    return true;
}

bool
SQLite3Dialect::has_for_update()
{
    return false;
}

const String
SQLite3Dialect::create_sequence(const String &seq_name)
{
    throw SqlDialectError(_T("No sequences, please"));
}

const String
SQLite3Dialect::drop_sequence(const String &seq_name)
{
    throw SqlDialectError(_T("No sequences, please"));
}

const String
SQLite3Dialect::primary_key_flag()
{
    return _T("PRIMARY KEY");
}

const String
SQLite3Dialect::autoinc_flag()
{
    return _T("AUTOINCREMENT");
}

// schema introspection

bool
SQLite3Dialect::table_exists(SqlConnection &conn, const String &table)
{
    return false;
}

bool
SQLite3Dialect::view_exists(SqlConnection &conn, const String &table)
{
    return false;
}

Strings
SQLite3Dialect::get_tables(SqlConnection &conn)
{
    return Strings();
}

Strings
SQLite3Dialect::get_views(SqlConnection &conn)
{
    return Strings();
}

ColumnsInfo
SQLite3Dialect::get_columns(SqlConnection &conn, const String &table)
{
    return ColumnsInfo();
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
