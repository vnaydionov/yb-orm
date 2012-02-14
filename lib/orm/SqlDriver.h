#ifndef YB__ORM__SQL_DRIVER__INCLUDED
#define YB__ORM__SQL_DRIVER__INCLUDED

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <iterator>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
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
    typedef boost::shared_ptr<Item> ItemPtr;
    typedef std::map<String, ItemPtr> Map;

    ItemRegistry() {}

    Item *find_item(const String &name)
    {
        boost::mutex::scoped_lock lock(mutex_);
        typename Map::const_iterator it = map_.find(name);
        if (it != map_.end())
            return it->second.get();
        return NULL;
    }

    bool register_item(const String &name, std::auto_ptr<Item> item)
    {
        boost::mutex::scoped_lock lock(mutex_);
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
        boost::mutex::scoped_lock lock(mutex_);
        Strings names;
        typename Map::const_iterator it = map_.begin(), end = map_.end();
        for (; it != end; ++it)
            names.push_back(it->first);
        return names;
    }

    bool empty() {
        boost::mutex::scoped_lock lock(mutex_);
        return map_.empty();
    }
private:
    Map map_;
    boost::mutex mutex_;
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

class SqlDialect
{
    String name_, dual_;
    bool has_sequences_;
    SqlDialect(const SqlDialect &);
    SqlDialect &operator=(const SqlDialect &);
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

class SqlConnectBackend
{
    SqlConnectBackend(const SqlConnectBackend &);
    SqlConnectBackend &operator=(const SqlConnectBackend &);
public:
    SqlConnectBackend() {}
    virtual ~SqlConnectBackend();
    virtual void open(SqlDialect *dialect, const String &db,
        const String &user, const String &passwd) = 0;
    virtual void close() = 0;
    virtual void commit() = 0;
    virtual void rollback() = 0;
    virtual void exec_direct(const String &sql) = 0;
    virtual RowPtr fetch_row() = 0;
    virtual void prepare(const String &sql) = 0;
    virtual void exec(const Values &params) = 0;
};

class SqlDriver
{
    String name_;
    SqlDriver(const SqlDriver &);
    SqlDriver &operator=(const SqlDriver &);
public:
    SqlDriver(const String &name)
        : name_(name)
    {}
    virtual ~SqlDriver();
    const String &get_name() { return name_; }
    virtual std::auto_ptr<SqlConnectBackend> create_backend() = 0;
};

SqlDriver *sql_driver(const String &name);
bool register_sql_driver(std::auto_ptr<SqlDriver> driver);
const Strings list_sql_drivers();

class SqlConnect;

class SqlResultSet: public ResultSetBase<Row>
{
    SqlConnect &conn_;

    bool fetch(Row &row);
    SqlResultSet();
public:
    SqlResultSet(SqlConnect &conn): conn_(conn) {}
};

class SqlConnect
{
    SqlDriver *driver_;
    SqlDialect *dialect_;
    String db_, user_;
    std::auto_ptr<SqlConnectBackend> backend_;
    bool activity_, echo_;
    ILogger *log_;
    SqlConnect(const SqlConnect &);
    SqlConnect &operator=(const SqlConnect &);
public:
    SqlConnect(const String &driver_name,
            const String &dialect_name, const String &db,
            const String &user = _T(""), const String &passwd = _T(""));
    ~SqlConnect();
    SqlDriver *get_driver() { return driver_; }
    SqlDialect *get_dialect() { return dialect_; }
    const String &get_db() { return db_; }
    const String &get_user() { return user_; }
    void set_echo(bool echo) { echo_ = echo; }
    void set_logger(ILogger *log) { log_ = log; }
    SqlResultSet exec_direct(const String &sql);
    RowPtr fetch_row();
    RowsPtr fetch_rows(int max_rows = -1); // -1 = all
    void prepare(const String &sql);
    SqlResultSet exec(const Values &params);
    void commit();
    void rollback();
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__SQL_DRIVER__INCLUDED
