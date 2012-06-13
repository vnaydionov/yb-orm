#include <time.h>
#include <iostream>
#include <sstream>
#include <orm/SqlDriver.h>
#include <orm/OdbcDriver.h>
#include <util/Singleton.h>

using namespace std;

#if 0
#define DBG(x) do { char __s[40]; std::ostringstream __log; time_t __t = time(NULL); \
    strcpy(__s, ctime(&__t)); __s[strlen(__s) - 1] = 0; \
    __log << __s << ": " << NARROW(x) << '\n'; \
    std::cerr << __log.str(); } while(0)
#else
#define DBG(x) do { if (log_) { log_->debug(NARROW(x)); } } while(0)
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
            return _T("timestamp") + x.sql_str();
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

void
SqlConnect::mark_bad(const std::exception &e)
{
    if (!bad_) {
        string s = e.what();
        size_t pos = s.find('\n');
        if (pos != string::npos)
            s = s.substr(0, pos);
        DBG(_T("mark connection bad, because of ") + String(WIDEN(s)));
        bad_ = true;
    }
}

SqlConnect::SqlConnect(const String &driver_name,
        const String &dialect_name, const String &db,
        const String &user, const String &passwd)
    : source_(SqlSource(db, driver_name, dialect_name, db, user, passwd))
    , driver_(sql_driver(source_.get_driver_name()))
    , dialect_(sql_dialect(source_.get_dialect_name()))
    , activity_(false)
    , echo_(false)
    , bad_(false)
    , free_since_(0)
    , log_(NULL)
{
    backend_.reset(driver_->create_backend().release());
    backend_->open(dialect_, source_.get_db(), source_.get_user(), source_.get_passwd());
}

SqlConnect::SqlConnect(const SqlSource &source)
    : source_(source)
    , driver_(sql_driver(source_.get_driver_name()))
    , dialect_(sql_dialect(source_.get_dialect_name()))
    , activity_(false)
    , echo_(false)
    , bad_(false)
    , free_since_(0)
    , log_(NULL)
{
    backend_.reset(driver_->create_backend().release());
    backend_->open(dialect_, source_.get_db(), source_.get_user(), source_.get_passwd());
}

SqlConnect::~SqlConnect()
{
    bool err = false;
    try {
        if (activity_)
            rollback();
    }
    catch (const std::exception &e) { err = true; }
    try {
        backend_->close();
    }
    catch (const std::exception &e) { err = true; }
    if (err)
        DBG(_T("error while closing connection"));
}

SqlResultSet
SqlConnect::exec_direct(const String &sql)
{
    try {
        if (echo_)
            DBG(_T("exec_direct: ") + sql);
        activity_ = true;
        backend_->exec_direct(sql);
        return SqlResultSet(*this);
    }
    catch (const std::exception &e) {
        mark_bad(e);
        throw;
    }
}

RowPtr
SqlConnect::fetch_row()
{
    try {
        RowPtr row = backend_->fetch_row();
        if (echo_) {
            if (row.get()) {
                std::ostringstream out;
                out << "fetch: ";
                Row::const_iterator j = row->begin(),
                                    jend = row->end();
                for (; j != jend; ++j)
                    out << NARROW(j->first) << "="
                        << NARROW(j->second.sql_str()) << " ";
                DBG(WIDEN(out.str()));
            }
            else
                DBG(_T("fetch: no more rows"));
        }
        return row;
    }
    catch (const std::exception &e) {
        mark_bad(e);
        throw;
    }
}

RowsPtr
SqlConnect::fetch_rows(int max_rows)
{
    try {
        RowsPtr rows(new Rows);
        SqlResultSet result(*this);
        copy_no_more_than_n(result.begin(), result.end(),
                            max_rows, back_inserter(*rows));
        return rows;
    }
    catch (const std::exception &e) {
        mark_bad(e);
        throw;
    }
}

void
SqlConnect::prepare(const String &sql)
{
    try {
        if (echo_)
            DBG(_T("prepare: ") + sql);
        activity_ = true;
        backend_->prepare(sql);
    }
    catch (const std::exception &e) {
        mark_bad(e);
        throw;
    }
}

SqlResultSet
SqlConnect::exec(const Values &params)
{
    try {
        if (echo_) {
            std::ostringstream out;
            out << "exec prepared:";
            for (unsigned i = 0; i < params.size(); ++i)
                out << " p" << (i + 1) << "=\""
                    << NARROW(params[i].sql_str()) << "\"";
            DBG(WIDEN(out.str()));
        }
        backend_->exec(params);
        return SqlResultSet(*this);
    }
    catch (const std::exception &e) {
        mark_bad(e);
        throw;
    }
}

void
SqlConnect::commit()
{
    try {
        activity_ = false;
        if (echo_)
            DBG(_T("commit"));
        backend_->commit();
    }
    catch (const std::exception &e) {
        mark_bad(e);
        throw;
    }
}

void
SqlConnect::rollback()
{
    try {
        activity_ = false;
        if (echo_)
            DBG(_T("rollback"));
        backend_->rollback();
    }
    catch (const std::exception &e) {
        mark_bad(e);
        throw;
    }
}

void
SqlConnect::clear()
{
    try {
        rollback();
    }
    catch (const std::exception &e) { }
    backend_->clear_statement();
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
