#include <orm/SqlDriver.h>
#include <time.h>
#include <orm/tiodbc.hpp>
#include <boost/lexical_cast.hpp>
#include <util/Singleton.h>
#include <util/str_utils.hpp>
#include <iostream>
#include <sstream>

using namespace std;
using Yb::StrUtils::str_to_upper;

#if 0
#define DBG(x) do{ char __s[40]; OStringStream __log; time_t __t = time(NULL); \
    strcpy(__s, ctime(&__t)); __s[strlen(__s) - 1] = 0; \
    __log << WIDEN(__s) << _T(": ") << x << '\n'; cerr << NARROW(__log.str()); }while(0)
#else
#define DBG(x) do{ if (log_) { log_->debug(NARROW(x)); } }while(0)
#endif

namespace Yb {

DBError::DBError(const String &msg)
    : BaseError(msg)
{}

GenericDBError::GenericDBError(const String &err)
    : DBError(_T("Database error, details: ") + err)
{}

NoDataFound::NoDataFound(const String &msg)
    : DBError(_T("Data wasn't found, details: ") + msg)
{}

BadSQLOperation::BadSQLOperation(const String &msg)
    : DBError(msg)
{}

BadOperationInMode::BadOperationInMode(const String &msg)
    : DBError(msg)
{}

SqlDialectError::SqlDialectError(const String &msg)
    : DBError(msg)
{}

SqlDriverError::SqlDriverError(const String &msg)
    : DBError(msg)
{}

SqlDialect::~SqlDialect() {}

class OracleDialect: public SqlDialect
{
public:
    OracleDialect()
        : SqlDialect(_T("ORACLE"), _T("DUAL"), true)
    {}
    const String select_curr_value(const String &seq_name)
    { return seq_name + _T(".CURRVAL"); }
    const String select_next_value(const String &seq_name)
    { return seq_name + _T(".NEXTVAL"); }
    const String sql_value(const Value &x)
    {
        if (x.get_type() == Value::DATETIME)
            return _T("date") + x.sql_str();
        return x.sql_str();
    }
};

class PostgresDialect: public SqlDialect
{
public:
    PostgresDialect()
        : SqlDialect(_T("POSTGRES"), _T(""), true)
    {}
    const String select_curr_value(const String &seq_name)
    { return _T("CURRVAL('") + seq_name + _T("')"); }
    const String select_next_value(const String &seq_name)
    { return _T("NEXTVAL('") + seq_name + _T("')"); }
    const String sql_value(const Value &x)
    {
        return x.sql_str();
    }
};

class InterbaseDialect: public SqlDialect
{
public:
    InterbaseDialect()
        : SqlDialect(_T("INTERBASE"), _T("RDB$DATABASE"), true)
    {}
    const String select_curr_value(const String &seq_name)
    { return _T("GEN_ID(") + seq_name + _T(", 0)"); }
    const String select_next_value(const String &seq_name)
    { return _T("GEN_ID(") + seq_name + _T(", 1)"); }
    const String sql_value(const Value &x)
    {
        return x.sql_str();
    }
};

class MysqlDialect: public SqlDialect
{
public:
    MysqlDialect()
        : SqlDialect(_T("MYSQL"), _T("DUAL"), false)
    {}
    const String select_curr_value(const String &seq_name)
    { throw SqlDialectError(_T("No sequences, please")); }
    const String select_next_value(const String &seq_name)
    { throw SqlDialectError(_T("No sequences, please")); }
    const String sql_value(const Value &x)
    {
        return x.sql_str();
    }
};

typedef SingletonHolder<ItemRegistry<SqlDialect> > theDialectRegistry;

void register_std_dialects()
{
    auto_ptr<SqlDialect> dialect;
    SqlDialect *p;
    dialect.reset((SqlDialect *)new OracleDialect());
    p = dialect.get();
    theDialectRegistry::instance().register_item(
            p->get_name(), dialect);
    dialect.reset((SqlDialect *)new PostgresDialect());
    p = dialect.get();
    theDialectRegistry::instance().register_item(
            p->get_name(), dialect);
    dialect.reset((SqlDialect *)new MysqlDialect());
    p = dialect.get();
    theDialectRegistry::instance().register_item(
            p->get_name(), dialect);
    dialect.reset((SqlDialect *)new InterbaseDialect());
    p = dialect.get();
    theDialectRegistry::instance().register_item(
            p->get_name(), dialect);
}

SqlDialect *sql_dialect(const String &name)
{
    if (theDialectRegistry::instance().empty())
        register_std_dialects();
    SqlDialect *dialect = theDialectRegistry::instance().find_item(name);
    if (!dialect)
        throw SqlDialectError(_T("Unknown dialect: ") + name);
    return dialect;
}

bool register_sql_dialect(auto_ptr<SqlDialect> dialect)
{
    if (theDialectRegistry::instance().empty())
        register_std_dialects();
    SqlDialect *p = dialect.get();
    return theDialectRegistry::instance().register_item(
            p->get_name(), dialect);
}

const Strings list_sql_dialects()
{
    if (theDialectRegistry::instance().empty())
        register_std_dialects();
    return theDialectRegistry::instance().list_items();
}

SqlConnectBackend::~SqlConnectBackend() {}
SqlDriver::~SqlDriver() {}

class OdbcConnectBackend: public SqlConnectBackend
{
    auto_ptr<tiodbc::connection> conn_;
    auto_ptr<tiodbc::statement> stmt_;
public:
    void open(SqlDialect *dialect, const String &dsn,
            const String &user, const String &passwd)
    {
        close();
        conn_.reset(new tiodbc::connection());
        stmt_.reset(new tiodbc::statement());
        if (!conn_->connect(dsn, user, passwd, 10, false))
            throw DBError(conn_->last_error_ex());
    }

    void close()
    {
        stmt_.reset(NULL);
        conn_.reset(NULL);
    }

    void commit()
    {
        if (!conn_->commit())
            throw DBError(conn_->last_error_ex());
    }

    void rollback()
    {
        if (!conn_->rollback())
            throw DBError(conn_->last_error_ex());
    }

    void exec_direct(const String &sql)
    {
        stmt_.reset(new tiodbc::statement());
        if (!stmt_->execute_direct(*conn_, sql))
            throw DBError(stmt_->last_error_ex());
    }

    RowPtr fetch_row()
    {
        if (!stmt_->fetch_next())
            return RowPtr();
        int col_count = stmt_->count_columns();
        RowPtr row(new Row);
        for (int i = 0; i < col_count; ++i) {
            tiodbc::field_impl f = stmt_->field(i + 1);
            String name = str_to_upper(f.get_name());
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
                String val = f.as_string();
                if (f.is_null() != 1)
                    v = Value(val);
            }
            row->push_back(RowItem(name, v));
        }
        return row;
    }

    void prepare(const String &sql)
    {
        stmt_.reset(new tiodbc::statement());
        if (!stmt_->prepare(*conn_, sql))
            throw DBError(stmt_->last_error_ex());
    }

    void exec(const Values &params)
    {
        for (unsigned i = 0; i < params.size(); ++i) {
            if (params[i].get_type() == Value::DATETIME) {
                TIMESTAMP_STRUCT ts;
                memset(&ts, 0, sizeof(ts));
                DateTime t = params[i].as_date_time();
                ts.year = t.date().year();
                ts.month = t.date().month();
                ts.day = t.date().day();
                ts.hour = (SQLUSMALLINT)t.time_of_day().hours();
                ts.minute = (SQLUSMALLINT)t.time_of_day().minutes();
                ts.second = (SQLUSMALLINT)t.time_of_day().seconds();
                ts.fraction = 0; // TODO
                stmt_->param(i + 1).set_as_date_time(
                        ts,
                        params[i].is_null());
            }
            else {
                String value;
                if (!params[i].is_null())
                    value = params[i].as_string();
                stmt_->param(i + 1).set_as_string(value, params[i].is_null());
            }
        }
        if (!stmt_->execute())
            throw DBError(stmt_->last_error_ex());
    }
};

class OdbcDriver: public SqlDriver
{
public:
    OdbcDriver():
        SqlDriver(_T("ODBC"))
    {}
    auto_ptr<SqlConnectBackend> create_backend()
    {
        auto_ptr<SqlConnectBackend> p(
                (SqlConnectBackend *)new OdbcConnectBackend());
        return p;
    }
};

typedef SingletonHolder<ItemRegistry<SqlDriver> > theDriverRegistry;

void register_std_drivers()
{
    auto_ptr<SqlDriver> driver((SqlDriver *)new OdbcDriver());
    SqlDriver *p = driver.get();
    theDriverRegistry::instance().register_item(p->get_name(), driver);
}

SqlDriver *sql_driver(const String &name)
{
    if (theDriverRegistry::instance().empty())
        register_std_drivers();
    SqlDriver *driver = theDriverRegistry::instance().find_item(name);
    if (!driver)
        throw SqlDriverError(_T("Unknown driver: ") + name);
    return driver;
}

bool register_sql_driver(auto_ptr<SqlDriver> driver)
{
    if (theDriverRegistry::instance().empty())
        register_std_drivers();
    SqlDriver *p = driver.get();
    return theDriverRegistry::instance().register_item(
            p->get_name(), driver);
}

const Strings list_sql_drivers()
{
    if (theDriverRegistry::instance().empty())
        register_std_drivers();
    return theDriverRegistry::instance().list_items();
}

Row::iterator find_in_row(Row &row, const String &name)
{
    Row::iterator i = row.begin(), iend = row.end();
    for (; i != iend && i->first != name; ++i);
    return i;
}

Row::const_iterator find_in_row(const Row &row, const String &name)
{
    Row::const_iterator i = row.begin(), iend = row.end();
    for (; i != iend && i->first != name; ++i);
    return i;
}

bool SqlResultSet::fetch(Row &row)
{
    RowPtr p = conn_.fetch_row();
    if (!p.get())
        return false;
    row.swap(*p);
    return true;
}

SqlConnect::SqlConnect(const String &driver_name,
        const String &dialect_name, const String &db,
        const String &user, const String &passwd)
    : driver_(sql_driver(driver_name))
    , dialect_(sql_dialect(dialect_name))
    , db_(db)
    , user_(user)
    , activity_(false)
    , echo_(false)
    , log_(NULL)
{
    backend_.reset(driver_->create_backend().release());
    backend_->open(dialect_, db, user, passwd);
}

SqlConnect::~SqlConnect()
{
    if (activity_)
        rollback();
    backend_->close();
}

SqlResultSet
SqlConnect::exec_direct(const String &sql)
{
    if (echo_) {
        DBG(_T("exec_direct: ") + sql);
    }
    activity_ = true;
    backend_->exec_direct(sql);
    return SqlResultSet(*this);
}

RowPtr
SqlConnect::fetch_row()
{
    RowPtr row = backend_->fetch_row();
    if (echo_) {
        if (row.get()) {
            OStringStream out;
            out << _T("fetch: ");
            Row::const_iterator j = row->begin(), jend = row->end();
            for (; j != jend; ++j)
                out << j->first << _T("=") << j->second.sql_str() << _T(" ");
            DBG(out.str());
        }
        else {
            DBG(_T("fetch: no more rows"));
        }
    }
    return row;
}

RowsPtr
SqlConnect::fetch_rows(int max_rows)
{
    RowsPtr rows(new Rows);
    SqlResultSet result(*this);
    copy_no_more_than_n(result.begin(), result.end(),
                        max_rows, back_inserter(*rows));
    return rows;
}

void
SqlConnect::prepare(const String &sql)
{
    if (echo_) {
        DBG(_T("prepare: ") + sql);
    }
    activity_ = true;
    backend_->prepare(sql);
}

SqlResultSet
SqlConnect::exec(const Values &params)
{
    if (echo_) {
        OStringStream out;
        out << _T("exec prepared:");
        for (unsigned i = 0; i < params.size(); ++i)
            out << _T(" p") << (i + 1) << _T("=\"") << params[i].sql_str() << _T("\"");
        DBG(out.str());
    }
    backend_->exec(params);
    return SqlResultSet(*this);
}

void
SqlConnect::commit()
{
    if (echo_)
        DBG(_T("commit"));
    backend_->commit();
    activity_ = false;
}

void
SqlConnect::rollback()
{
    if (echo_)
        DBG(_T("rollback"));
    backend_->rollback();
    activity_ = false;
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
