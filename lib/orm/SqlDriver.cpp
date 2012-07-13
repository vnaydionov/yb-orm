#include <time.h>
#include <iostream>
#include <sstream>
#include <orm/SqlDriver.h>
#if defined(YB_USE_QT)
#include <orm/QtSqlDriver.h>
typedef Yb::QtSqlDriver DefaultSqlDriver;
#define DEFAULT_DRIVER _T("QODBC3")
#else
#include <orm/OdbcDriver.h>
typedef Yb::OdbcDriver DefaultSqlDriver;
#define DEFAULT_DRIVER _T("ODBC")
#endif
#include <util/str_utils.hpp>
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

bool SqlDialect::explicit_begin_trans() { return false; }

const String SqlDialect::select_last_inserted_id(const String &table_name) {
    throw SqlDialectError(_T("No autoincrement flag"));
}

bool SqlDialect::commit_ddl() { return false; }

bool SqlDialect::fk_internal() { return false; }

const String SqlDialect::suffix_create_table() { return String(); }

const String SqlDialect::primary_key_flag() { return String(); }

const String SqlDialect::autoinc_flag() { return String(); }

const String SqlDialect::sysdate_func() {
    return _T("CURRENT_TIMESTAMP");
}

const String SqlDialect::not_null_default(const String &not_null_clause,
        const String &default_value)
{
    if (str_empty(not_null_clause))
        return default_value;
    if (str_empty(default_value))
        return not_null_clause;
    return default_value + _T(" ") + not_null_clause;
}

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
    const String type2sql(int t) {
        switch (t) {
            case Value::INTEGER:    return _T("NUMBER(10)");    break;
            case Value::LONGINT:    return _T("NUMBER");        break;
            case Value::STRING:     return _T("VARCHAR2");      break;
            case Value::DECIMAL:    return _T("NUMBER");        break;
            case Value::DATETIME:   return _T("DATE");          break;
        }
        throw SqlDialectError(_T("Bad type"));
    }
    const String gen_sequence(const String &seq_name) {
        return _T("CREATE SEQUENCE ") + seq_name;
    }
    const String sysdate_func() { return _T("SYSDATE"); }
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
    const String type2sql(int t) {
        switch (t) {
            case Value::INTEGER:    return _T("INTEGER");       break;
            case Value::LONGINT:    return _T("BIGINT");        break;
            case Value::STRING:     return _T("VARCHAR");       break;
            case Value::DECIMAL:    return _T("DECIMAL");       break;
            case Value::DATETIME:   return _T("TIMESTAMP");     break;
        }
        throw SqlDialectError(_T("Bad type"));
    }
    const String gen_sequence(const String &seq_name) {
        return _T("CREATE SEQUENCE ") + seq_name;
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
    bool commit_ddl() { return true; }
    const String type2sql(int t) {
        switch (t) {
            case Value::INTEGER:    return _T("INTEGER");       break;
            case Value::LONGINT:    return _T("BIGINT");        break;
            case Value::STRING:     return _T("VARCHAR");       break;
            case Value::DECIMAL:    return _T("DECIMAL(16, 6)"); break;
            case Value::DATETIME:   return _T("TIMESTAMP");     break;
        }
        throw SqlDialectError(_T("Bad type"));
    }
    const String gen_sequence(const String &seq_name) {
        return _T("CREATE GENERATOR ") + seq_name;
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
    const String select_last_inserted_id(const String &table_name) {
        return _T("SELECT LAST_INSERT_ID() LID");
    }
    const String sql_value(const Value &x)
    {
        return x.sql_str();
    }
    const String type2sql(int t) {
        switch (t) {
            case Value::INTEGER:    return _T("INT");           break;
            case Value::LONGINT:    return _T("BIGINT");        break;
            case Value::STRING:     return _T("VARCHAR");       break;
            case Value::DECIMAL:    return _T("DECIMAL(16, 6)"); break;
            case Value::DATETIME:   return _T("TIMESTAMP");     break;
        }
        throw SqlDialectError(_T("Bad type"));
    }
    const String gen_sequence(const String &seq_name) {
        throw SqlDialectError(_T("No sequences, please"));
    }
    const String suffix_create_table() {
        return _T(" ENGINE=INNODB DEFAULT CHARSET=utf8");
    }
    const String autoinc_flag() { return _T("AUTO_INCREMENT"); }
    const String not_null_default(const String &not_null_clause,
            const String &default_value)
    {
        if (str_empty(not_null_clause))
            return default_value;
        if (str_empty(default_value))
            return not_null_clause;
        return not_null_clause + _T(" ") + default_value;
    }
};

class SQLite3Dialect: public SqlDialect
{
public:
    SQLite3Dialect()
        : SqlDialect(_T("SQLITE3"), _T(""), false)
    {}
    bool explicit_begin_trans() { return true; }
    const String select_curr_value(const String &seq_name)
    { throw SqlDialectError(_T("No sequences, please")); }
    const String select_next_value(const String &seq_name)
    { throw SqlDialectError(_T("No sequences, please")); }
    const String select_last_inserted_id(const String &table_name) {
        return _T("SELECT SEQ LID FROM SQLITE_SEQUENCE WHERE NAME = '")
            + table_name + _T("'");
    }
    const String sql_value(const Value &x)
    {
        return x.sql_str();
    }
    const String type2sql(int t) {
        switch (t) {
            case Value::INTEGER:    return _T("INTEGER");       break;
            case Value::LONGINT:    return _T("INTEGER");       break;
            case Value::STRING:     return _T("VARCHAR");       break;
            case Value::DECIMAL:    return _T("NUMERIC");       break;
            case Value::DATETIME:   return _T("TIMESTAMP");     break;
        }
        throw SqlDialectError(_T("Bad type"));
    }
    bool fk_internal() { return true; }
    const String gen_sequence(const String &seq_name) {
        throw SqlDialectError(_T("No sequences, please"));
    }
    const String primary_key_flag() { return _T("PRIMARY KEY"); }
    const String autoinc_flag() { return _T("AUTOINCREMENT"); }
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
    dialect.reset((SqlDialect *)new InterbaseDialect());
    p = dialect.get();
    theDialectRegistry::instance().register_item(
            p->get_name(), dialect);
    dialect.reset((SqlDialect *)new MysqlDialect());
    p = dialect.get();
    theDialectRegistry::instance().register_item(
            p->get_name(), dialect);
    dialect.reset((SqlDialect *)new SQLite3Dialect());
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

SqlCursorBackend::~SqlCursorBackend() {}
SqlConnectionBackend::~SqlConnectionBackend() {}
SqlDriver::~SqlDriver() {}

typedef SingletonHolder<ItemRegistry<SqlDriver> > theDriverRegistry;

void register_std_drivers()
{
    SqlDriver *p;
#if defined(YB_USE_QT)
    auto_ptr<SqlDriver> driver_qtsql((SqlDriver *)new QtSqlDriver(false));
    p = driver_qtsql.get();
    theDriverRegistry::instance().register_item(p->get_name(), driver_qtsql);
    auto_ptr<SqlDriver> driver_qodbc((SqlDriver *)new QtSqlDriver(true));
    p = driver_qodbc.get();
    theDriverRegistry::instance().register_item(p->get_name(), driver_qodbc);
#else
    auto_ptr<SqlDriver> driver_odbc((SqlDriver *)new OdbcDriver());
    p = driver_odbc.get();
    theDriverRegistry::instance().register_item(p->get_name(), driver_odbc);
#endif
}

SqlDriver *sql_driver(const String &name)
{
    if (theDriverRegistry::instance().empty())
        register_std_drivers();
    SqlDriver *driver = NULL;
    if (str_empty(name) || name == _T("DEFAULT"))
        driver = theDriverRegistry::instance().find_item(DEFAULT_DRIVER);
    else
        driver = theDriverRegistry::instance().find_item(name);
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

const SqlSource parse_url(const String &url)
{
    using namespace Yb::StrUtils;
    String dialect, driver = _T("DEFAULT"), host, db, user, passwd;
    Strings url_parts, proto_parts, user_host_parts, host_params;
    split_str(url, _T("://"), url_parts);
    if (url_parts.size() != 2)
        throw ValueError(_T("url parse error"));
    split_str_by_chars(url_parts[0], _T("+"), proto_parts, 2);
    if (proto_parts.size() == 1) {
        dialect = str_to_upper(proto_parts[0]);
    }
    else if (proto_parts.size() == 2) {
        dialect = str_to_upper(proto_parts[0]);
        driver = str_to_upper(proto_parts[1]);
    }
    else
        throw ValueError(_T("url parse error"));
    split_str_by_chars(url_parts[1], _T("@"), user_host_parts, 2);
    String host_etc;
    if (user_host_parts.size() == 1) {
        host_etc = user_host_parts[0];
    }
    else if (user_host_parts.size() == 2) {
        Strings user_parts;
        split_str_by_chars(user_host_parts[0], _T(":/"), user_parts, 2);
        if (user_parts.size() == 1) {
            user = user_parts[0];
        }
        else if (user_parts.size() == 2) {
            user = user_parts[0];
            passwd = user_parts[1];
        }
        else
            throw ValueError(_T("url parse error"));
        host_etc = user_host_parts[1];
    }
    else
        throw ValueError(_T("url parse error"));
    split_str_by_chars(host_etc, _T("?"), host_params, 2);
    host_etc = host_params[0];
    if (host_params.size() != 1 && host_params.size() != 2)
        throw ValueError(_T("url parse error"));
    ///
    db = host_etc;
    return SqlSource(_T(""), driver, dialect, db, user, passwd);
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

bool
SqlResultSet::fetch(Row &row)
{
    RowPtr p = cursor_.fetch_row();
    if (!p.get())
        return false;
    row.swap(*p);
    return true;
}

void
SqlResultSet::own(auto_ptr<SqlCursor> cursor)
{
    owned_cursor_.reset(NULL);
    owned_cursor_.reset(cursor.release());
}

SqlCursor::SqlCursor(SqlConnection &connection)
    : connection_(connection)
    , backend_(connection.backend_->new_cursor())
    , echo_(connection.echo_)
    , log_(connection.log_)
{}

SqlResultSet
SqlCursor::exec_direct(const String &sql)
{
    try {
        if (echo_)
            DBG(_T("exec_direct: ") + sql);
        connection_.activity_ = true;
        backend_->exec_direct(sql);
        return SqlResultSet(*this);
    }
    catch (const std::exception &e) {
        connection_.mark_bad(e);
        throw;
    }
}

void
SqlCursor::prepare(const String &sql)
{
    try {
        if (echo_)
            DBG(_T("prepare: ") + sql);
        connection_.activity_ = true;
        backend_->prepare(sql);
    }
    catch (const std::exception &e) {
        connection_.mark_bad(e);
        throw;
    }
}

SqlResultSet
SqlCursor::exec(const Values &params)
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
        connection_.activity_ = true;
        backend_->exec(params);
        return SqlResultSet(*this);
    }
    catch (const std::exception &e) {
        connection_.mark_bad(e);
        throw;
    }
}

RowPtr
SqlCursor::fetch_row()
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
        connection_.mark_bad(e);
        throw;
    }
}

RowsPtr
SqlCursor::fetch_rows(int max_rows)
{
    try {
        RowsPtr rows(new Rows);
        SqlResultSet result(*this);
        copy_no_more_than_n(result.begin(), result.end(),
                            max_rows, back_inserter(*rows));
        return rows;
    }
    catch (const std::exception &e) {
        connection_.mark_bad(e);
        throw;
    }
}

void
SqlConnection::mark_bad(const std::exception &e)
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

SqlConnection::SqlConnection(const String &driver_name,
        const String &dialect_name, const String &db,
        const String &user, const String &passwd)
    : source_(SqlSource(db, driver_name, dialect_name, db, user, passwd))
    , driver_(sql_driver(source_.get_driver_name()))
    , dialect_(sql_dialect(source_.get_dialect_name()))
    , activity_(false)
    , echo_(false)
    , bad_(false)
    , explicit_trans_(false)
    , free_since_(0)
    , log_(NULL)
{
    source_.fix_driver_name(driver_->get_name());
    backend_.reset(driver_->create_backend().release());
    backend_->open(dialect_, source_);
}

SqlConnection::SqlConnection(const SqlSource &source)
    : source_(source)
    , driver_(sql_driver(source_.get_driver_name()))
    , dialect_(sql_dialect(source_.get_dialect_name()))
    , activity_(false)
    , echo_(false)
    , bad_(false)
    , explicit_trans_(false)
    , free_since_(0)
    , log_(NULL)
{
    source_.fix_driver_name(driver_->get_name());
    backend_.reset(driver_->create_backend().release());
    backend_->open(dialect_, source_);
}

SqlConnection::SqlConnection(const String &url)
    : source_(parse_url(url))
    , driver_(sql_driver(source_.get_driver_name()))
    , dialect_(sql_dialect(source_.get_dialect_name()))
    , activity_(false)
    , echo_(false)
    , bad_(false)
    , explicit_trans_(false)
    , free_since_(0)
    , log_(NULL)
{
    source_.fix_driver_name(driver_->get_name());
    backend_.reset(driver_->create_backend().release());
    backend_->open(dialect_, source_);
}

SqlConnection::~SqlConnection()
{
    bool err = false;
    try {
        clear();
        if (activity_ && (explicit_trans_ || !dialect_->explicit_begin_trans()))
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

auto_ptr<SqlCursor>
SqlConnection::new_cursor()
{
    return auto_ptr<SqlCursor>(new SqlCursor(*this));
}

void
SqlConnection::begin_trans()
{
    try {
        activity_ = false;
        explicit_trans_ = true;
        if (echo_)
            DBG(_T("begin transaction"));
        if (dialect_->explicit_begin_trans())
            exec_direct(_T("BEGIN TRANSACTION"));
    }
    catch (const std::exception &e) {
        mark_bad(e);
        throw;
    }
}

void
SqlConnection::commit()
{
    try {
        activity_ = false;
        explicit_trans_ = false;
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
SqlConnection::rollback()
{
    try {
        activity_ = false;
        explicit_trans_ = false;
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
SqlConnection::clear()
{
    try {
        cursor_.reset(NULL);
    }
    catch (const std::exception &e) {
        mark_bad(e);
        throw;
    }
}

SqlResultSet
SqlConnection::exec_direct(const String &sql)
{
    auto_ptr<SqlCursor> cursor;
    try {
        cursor.reset(NULL);
        cursor.reset(new SqlCursor(*this));
    }
    catch (const std::exception &e) {
        mark_bad(e);
        throw;
    }
    SqlResultSet rs = cursor->exec_direct(sql);
    rs.own(cursor);
    return rs;
}

void
SqlConnection::prepare(const String &sql)
{
    try {
        cursor_.reset(NULL);
        cursor_.reset(new SqlCursor(*this));
    }
    catch (const std::exception &e) {
        mark_bad(e);
        throw;
    }
    cursor_->prepare(sql);
}

SqlResultSet
SqlConnection::exec(const Values &params)
{
    YB_ASSERT(cursor_.get());
    SqlResultSet rs = cursor_->exec(params);
    return rs;
}

RowPtr
SqlConnection::fetch_row()
{
    YB_ASSERT(cursor_.get());
    return cursor_->fetch_row();
}

RowsPtr
SqlConnection::fetch_rows(int max_rows)
{
    YB_ASSERT(cursor_.get());
    return cursor_->fetch_rows(max_rows);
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
