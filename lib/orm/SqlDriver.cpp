#include "SqlDriver.h"
#include <time.h>
#include <tiodbc.hpp>
#include <boost/lexical_cast.hpp>
#include <util/Singleton.h>
#include <util/str_utils.hpp>
#include <iostream>

using namespace std;
using Yb::StrUtils::str_to_upper;

namespace Yb {

DBError::DBError(const string &msg)
    : logic_error(msg)
{}

GenericDBError::GenericDBError(const string &err)
    : DBError("Database error, details: " + err)
{}

NoDataFound::NoDataFound(const string &msg)
    : DBError("Data wasn't found, details: " + msg)
{}

BadSQLOperation::BadSQLOperation(const string &msg)
    : DBError(msg)
{}

BadOperationInMode::BadOperationInMode(const string &msg)
    : DBError(msg)
{}

SqlDialectError::SqlDialectError(const string &msg)
    : DBError(msg)
{}

SqlDriverError::SqlDriverError(const string &msg)
    : DBError(msg)
{}

SqlDialect::~SqlDialect() {}

class OracleDialect: public SqlDialect
{
public:
    OracleDialect()
        : SqlDialect("ORACLE", "DUAL", true)
    {}
    const string select_curr_value(const string &seq_name)
    { return seq_name + ".CURRVAL"; }
    const string select_next_value(const string &seq_name)
    { return seq_name + ".NEXTVAL"; }
};

class InterbaseDialect: public SqlDialect
{
public:
    InterbaseDialect()
        : SqlDialect("INTERBASE", "RDB$DATABASE", true)
    {}
    const string select_curr_value(const string &seq_name)
    { return "GEN_ID(" + seq_name + ", 0)"; }
    const string select_next_value(const string &seq_name)
    { return "GEN_ID(" + seq_name + ", 1)"; }
};

class MysqlDialect: public SqlDialect
{
public:
    MysqlDialect()
        : SqlDialect("MYSQL", "DUAL", false)
    {}
    const string select_curr_value(const string &seq_name)
    { throw SqlDialectError("No sequences, please"); }
    const string select_next_value(const string &seq_name)
    { throw SqlDialectError("No sequences, please"); }
};

typedef SingletonHolder<ItemRegistry<SqlDialect> > theDialectRegistry;

void register_std_dialects()
{
    SqlDialect *dialect;
    dialect = (SqlDialect *)new OracleDialect();
    theDialectRegistry::instance().register_item(dialect->get_name(), dialect);
    dialect = (SqlDialect *)new MysqlDialect();
    theDialectRegistry::instance().register_item(dialect->get_name(), dialect);
    dialect = (SqlDialect *)new InterbaseDialect();
    theDialectRegistry::instance().register_item(dialect->get_name(), dialect);
}

SqlDialect *sql_dialect(const string &name)
{
    if (theDialectRegistry::instance().empty())
        register_std_dialects();
    SqlDialect *dialect = theDialectRegistry::instance().find_item(name);
    if (!dialect)
        throw SqlDialectError("Unknown dialect: " + name);
    return dialect;
}

bool register_sql_dialect(SqlDialect *dialect)
{
    return theDialectRegistry::instance().register_item(
            dialect->get_name(), dialect);
}

const Names list_sql_dialects()
{
    if (theDialectRegistry::instance().empty())
        register_std_dialects();
    return theDialectRegistry::instance().list_items();
}

SqlConnectBackend::~SqlConnectBackend() {}
SqlDriver::~SqlDriver() {}

class OdbcConnectBackend: public SqlConnectBackend
{
    tiodbc::connection conn_;
    tiodbc::statement stmt_;
public:
    void open(SqlDialect *dialect, const string &dsn,
            const string &user, const string &passwd)
    {
        if (!conn_.connect(dsn, user, passwd, 10, false))
            throw DBError(conn_.last_error());
    }

    void close()
    {
        conn_.disconnect();
    }

    void commit()
    {
        if (!conn_.commit())
            throw DBError(conn_.last_error());
    }

    void rollback()
    {
        if (!conn_.rollback())
            throw DBError(conn_.last_error());
    }

    void exec_direct(const string &sql)
    {
        if (!stmt_.execute_direct(conn_, sql))
            throw DBError(stmt_.last_error());
    }

    RowsPtr fetch_rows(int max_rows)
    {
        RowsPtr rows(new Rows);
        for (int count = 0; max_rows < 0? 1: count < max_rows; ++count) {
            if (!stmt_.fetch_next())
                break;
            int col_count = stmt_.count_columns();
            Row row;
            for (int i = 0; i < col_count; ++i) {
                tiodbc::field_impl f = stmt_.field(i + 1);
                string name = str_to_upper(f.get_name());
                Value v;
                if (f.get_type() == SQL_DATE ||
                        f.get_type() == SQL_TIMESTAMP ||
                        f.get_type() == SQL_TYPE_TIMESTAMP)
                {
                    TIMESTAMP_STRUCT ts = f.as_date_time();
                    if (ts.year != 0 || f.is_null() != 1) {
                        v = Value(mk_datetime(ts.year, ts.month, ts.day,
                                    ts.hour, ts.minute, ts.second));
                    }
                }
                else {
                    string val = f.as_string();
                    if (f.is_null() != 1)
                        v = Value(val);
                }
                row[name] = v;
            }
            rows->push_back(row);
        }
        return rows;
    }

    void prepare(const string &sql)
    {
        if (!stmt_.prepare(conn_, sql))
            throw DBError(stmt_.last_error());
    }

    void exec(const Values &params)
    {
        for (int i = 0; i < params.size(); ++i) {
            if (params[i].get_type() == Value::DATETIME) {
                TIMESTAMP_STRUCT ts;
                memset(&ts, 0, sizeof(ts));
                DateTime t = params[i].as_date_time();
                ts.year = t.date().year();
                ts.month = t.date().month();
                ts.day = t.date().day();
                ts.hour = t.time_of_day().hours();
                ts.minute = t.time_of_day().minutes();
                ts.second = t.time_of_day().seconds();
                ts.fraction = 0; // TODO
                stmt_.param(i + 1).set_as_date_time(
                        ts,
                        params[i].is_null());
            }
            else {
                stmt_.param(i + 1).set_as_string(
                        params[i].is_null()?
                            string(): params[i].as_string(),
                        params[i].is_null());
            }
        }
        if (!stmt_.execute())
            throw DBError(stmt_.last_error());
    }
};

class OdbcDriver: public SqlDriver
{
public:
    OdbcDriver():
        SqlDriver("ODBC")
    {}
    SqlConnectBackend *create_backend()
    {
        return (SqlConnectBackend *)new OdbcConnectBackend();
    }
};

typedef SingletonHolder<ItemRegistry<SqlDriver> > theDriverRegistry;

void register_std_drivers()
{
    SqlDriver *driver;
    driver = (SqlDriver *)new OdbcDriver();
    theDriverRegistry::instance().register_item(driver->get_name(), driver);
}

SqlDriver *sql_driver(const string &name)
{
    if (theDriverRegistry::instance().empty())
        register_std_drivers();
    SqlDriver *driver = theDriverRegistry::instance().find_item(name);
    if (!driver)
        throw SqlDriverError("Unknown driver: " + name);
    return driver;
}

bool register_sql_driver(SqlDriver *driver)
{
    return theDriverRegistry::instance().register_item(
            driver->get_name(), driver);
}

const Names list_sql_drivers()
{
    if (theDriverRegistry::instance().empty())
        register_std_drivers();
    return theDriverRegistry::instance().list_items();
}

SqlConnect::SqlConnect(const string &driver_name,
        const string &dialect_name, const string &db,
        const string &user, const string &passwd)
    : driver_(sql_driver(driver_name))
    , dialect_(sql_dialect(dialect_name))
    , db_(db)
    , user_(user)
    , backend_(driver_->create_backend())
    , activity_(false)
    , echo_(false)
{
    backend_->open(dialect_, db, user, passwd);
}

SqlConnect::~SqlConnect()
{
    if (activity_)
        rollback();
    backend_->close();
}

void
SqlConnect::exec_direct(const string &sql)
{
    if (echo_) {
        cout << "exec_direct: " << sql << "\n";
    }
    activity_ = true;
    backend_->exec_direct(sql);
}

RowsPtr
SqlConnect::fetch_rows(int max_rows)
{
    RowsPtr rows(backend_->fetch_rows(max_rows));
    if (echo_) {
        cout << "fetch:\n";
        Rows::const_iterator i = rows->begin(), iend = rows->end();
        for (; i != iend; ++i) {
            Row::const_iterator j = i->begin(), jend = i->end();
            for (; j != jend; ++j)
                cout << j->first << "=" << j->second.sql_str() << " ";
            cout << "\n";
        }
    }
    return rows;
}

void
SqlConnect::prepare(const string &sql)
{
    if (echo_) {
        cout << "prepare: " << sql << "\n";
    }
    activity_ = true;
    backend_->prepare(sql);
}

void
SqlConnect::exec(const Values &params)
{
    if (echo_) {
        for (int i = 0; i < params.size(); ++i)
            cout << "exec: p[" << (i + 1) << "]=\"" << params[i].sql_str() << "\"\n";
    }
    backend_->exec(params);
}

void
SqlConnect::commit()
{
    backend_->commit();
    activity_ = false;
}

void
SqlConnect::rollback()
{
    backend_->rollback();
    activity_ = false;
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
