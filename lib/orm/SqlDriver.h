#ifndef YB__ORM__SQL_DRIVER__INCLUDED
#define YB__ORM__SQL_DRIVER__INCLUDED

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <iterator>
#include <util/Utility.h>
#include <util/Thread.h>
#include <util/ResultSet.h>
#include <util/nlogger.h>
#include <orm/Value.h>

namespace Yb {

template <class Item>
class ItemRegistry
{
    ItemRegistry(const ItemRegistry &);
    ItemRegistry &operator=(const ItemRegistry &);
public:
    typedef
#if defined(__BORLANDC__)
        SharedPtr<Item>::Type
#else
        typename SharedPtr<Item>::Type
#endif
            ItemPtr;
    typedef std::map<String, ItemPtr> Map;

    ItemRegistry() {}

    Item *find_item(const String &name)
    {
        ScopedLock lock(mutex_);
        typename Map::const_iterator it = map_.find(name);
        if (it != map_.end())
            return shptr_get(it->second);
        return NULL;
    }

    bool register_item(const String &name, std::auto_ptr<Item> item)
    {
        ScopedLock lock(mutex_);
        typename Map::const_iterator it = map_.find(name);
        if (it != map_.end())
            return false;
        map_.insert(
#if defined(__BORLANDC__)
            Map::value_type
#else
            typename Map::value_type
#endif
            (name, ItemPtr(item.release())));
        map_.insert(
#if defined(__BORLANDC__)
            Map::value_type
#else
            typename Map::value_type
#endif
            (name, ItemPtr(item.release())));
        return true;
    }

    const Strings list_items()
    {
        ScopedLock lock(mutex_);
        Strings names;
        typename Map::const_iterator it = map_.begin(), end = map_.end();
        for (; it != end; ++it)
            names.push_back(it->first);
        return names;
    }

    bool empty() {
        ScopedLock lock(mutex_);
        return map_.empty();
    }
private:
    Map map_;
    Mutex mutex_;
};

class DBError : public BaseError
{
public:
    DBError(const String &msg);
};

class GenericDBError: public DBError
{
public:
    GenericDBError(const String &err);
};

class NoDataFound : public DBError
{
public:
    NoDataFound(const String &msg = _T(""));
};

class BadSQLOperation : public DBError
{
public:
    BadSQLOperation(const String &msg);
};

class BadOperationInMode : public DBError
{
public:
    BadOperationInMode(const String &msg);
};

class SqlDialectError : public DBError
{
public:
    SqlDialectError(const String &msg);
};

class SqlDriverError : public DBError
{
public:
    SqlDriverError(const String &msg);
};

class SqlDialect: NonCopyable
{
    String name_, dual_;
    bool has_sequences_;
public:
    SqlDialect(const String &name, const String &dual,
            bool has_sequences)
        : name_(name)
        , dual_(dual)
        , has_sequences_(has_sequences)
    {}
    virtual ~SqlDialect();
    const String &get_name() { return name_; }
    const String &dual_name() { return dual_; }
    bool has_sequences() { return has_sequences_; }
    virtual const String select_curr_value(
            const String &seq_name) = 0;
    virtual const String select_next_value(
            const String &seq_name) = 0;
    virtual const String sql_value(const Value &x) = 0;
};

SqlDialect *sql_dialect(const String &name);
bool register_sql_dialect(std::auto_ptr<SqlDialect> dialect);
const Strings list_sql_dialects();

typedef std::pair<String, Value> RowItem;
typedef std::vector<RowItem > Row;
typedef std::auto_ptr<Row> RowPtr;
typedef std::vector<Row> Rows;
typedef std::auto_ptr<Rows> RowsPtr;
Row::iterator find_in_row(Row &row, const String &name);
Row::const_iterator find_in_row(const Row &row, const String &name);

class SqlCursorBackend: NonCopyable
{
public:
    SqlCursorBackend() {}
    virtual ~SqlCursorBackend();
    virtual void exec_direct(const String &sql) = 0;
    virtual void prepare(const String &sql) = 0;
    virtual void exec(const Values &params) = 0;
    virtual RowPtr fetch_row() = 0;
};

class SqlSource;

class SqlConnectionBackend: NonCopyable
{
public:
    SqlConnectionBackend() {}
    virtual ~SqlConnectionBackend();
    virtual void open(SqlDialect *dialect, const SqlSource &source) = 0;
    virtual std::auto_ptr<SqlCursorBackend> new_cursor() = 0;
    virtual void close() = 0;
    virtual void commit() = 0;
    virtual void rollback() = 0;
};

class SqlDriver: NonCopyable
{
    String name_;
public:
    SqlDriver(const String &name)
        : name_(name)
    {}
    virtual ~SqlDriver();
    const String &get_name() { return name_; }
    virtual std::auto_ptr<SqlConnectionBackend> create_backend() = 0;
};

SqlDriver *sql_driver(const String &name);
bool register_sql_driver(std::auto_ptr<SqlDriver> driver);
const Strings list_sql_drivers();

class SqlSource
{
    String id_, driver_name_, dialect_name_;
    String db_, user_, passwd_;
    String encoding_, host_;
    int port_, timeout_, autocommit_;
public:
    SqlSource()
        : port_(-1), timeout_(10), autocommit_(0)
    {}
    SqlSource(const String &id, const String &driver_name,
            const String &dialect_name,
            const String &db, const String &user = _T(""),
            const String &passwd = _T(""),
            const String &encoding = _T(""), const String &host = _T(""),
            int port = -1, int timeout = 10, int autocommit = 0)
        : id_(id), driver_name_(driver_name), dialect_name_(dialect_name)
        , db_(db), user_(user), passwd_(passwd)
        , encoding_(encoding), host_(host)
        , port_(port), timeout_(timeout), autocommit_(autocommit)
    {}

    const String &get_id() const { return id_; }
    const String &get_driver_name() const { return driver_name_; }
    const String &get_dialect_name() const { return dialect_name_; }
    const String &get_db() const { return db_; }
    const String &get_user() const { return user_; }
    const String &get_passwd() const { return passwd_; }
    const String &get_encoding() const { return encoding_; }
    const String &get_host() const { return host_; }
    int get_port() const { return port_; }
    int get_timeout() const { return timeout_; }
    int get_autocommit() const { return autocommit_; }

    bool operator== (const SqlSource &obj) const { return id_ == obj.id_; }
    bool operator< (const SqlSource &obj) const { return id_ < obj.id_; }
};

class SqlCursor;
class SqlConnection;
class SqlPool;

class SqlResultSet: public ResultSetBase<Row>
{
    friend class SqlCursor;
    SqlCursor &cursor_;
    mutable std::auto_ptr<SqlCursor> owned_cursor_;
    bool fetch(Row &row);
    SqlResultSet(SqlCursor &cursor): cursor_(cursor) {}
public:
    SqlResultSet(const SqlResultSet &rs)
        : cursor_(rs.cursor_)
        , owned_cursor_(rs.owned_cursor_.release())
    {}
    void own(std::auto_ptr<SqlCursor> cursor);
};

class SqlCursor: NonCopyable
{
    friend class SqlConnection;
    SqlConnection &connection_;
    std::auto_ptr<SqlCursorBackend> backend_;
    bool echo_;
    ILogger *log_;
    SqlCursor(SqlConnection &connection);
public:
    SqlResultSet exec_direct(const String &sql);
    void prepare(const String &sql);
    SqlResultSet exec(const Values &params);
    RowPtr fetch_row();
    RowsPtr fetch_rows(int max_rows = -1); // -1 = all
};

class SqlConnection: NonCopyable
{
    friend class SqlPool;
    friend class SqlCursor;
    SqlSource source_;
    SqlDriver *driver_;
    SqlDialect *dialect_;
    std::auto_ptr<SqlConnectionBackend> backend_;
    std::auto_ptr<SqlCursor> cursor_;
    bool activity_, echo_, bad_;
    time_t free_since_;
    ILogger *log_;
    void mark_bad(const std::exception &e);
public:
    SqlConnection(const String &driver_name,
            const String &dialect_name, const String &db,
            const String &user = _T(""), const String &passwd = _T(""));
    explicit SqlConnection(const SqlSource &source);
    ~SqlConnection();
    const SqlSource &get_source() const { return source_; }
    SqlDriver *get_driver() const { return driver_; }
    SqlDialect *get_dialect() const { return dialect_; }
    const String &get_db() const { return source_.get_db(); }
    const String &get_user() const { return source_.get_user(); }
    void set_echo(bool echo) { echo_ = echo; }
    void set_logger(ILogger *log) { log_ = log; }
    std::auto_ptr<SqlCursor> new_cursor();
    bool bad() const { return bad_; }
    void commit();
    void rollback();
    void clear();
    SqlResultSet exec_direct(const String &sql);
    void prepare(const String &sql);
    SqlResultSet exec(const Values &params);
    RowPtr fetch_row();
    RowsPtr fetch_rows(int max_rows = -1); // -1 = all
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__SQL_DRIVER__INCLUDED
