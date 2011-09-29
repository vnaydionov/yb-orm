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
#include <orm/Value.h>

namespace Yb {

template <class Item>
class ItemRegistry
{
    ItemRegistry(const ItemRegistry &);
    ItemRegistry &operator=(const ItemRegistry &);
public:
    typedef boost::shared_ptr<Item> ItemPtr;
    typedef std::map<std::string, ItemPtr> Map;

    ItemRegistry() {}

    Item *find_item(const std::string &name)
    {
        boost::mutex::scoped_lock lock(mutex_);
        typename Map::const_iterator it = map_.find(name);
        if (it != map_.end())
            return it->second.get();
        return NULL;
    }

    bool register_item(const std::string &name, std::auto_ptr<Item> item)
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
    DBError(const std::string &msg);
};

class GenericDBError: public DBError
{
public:
    GenericDBError(const std::string &err);
};

class NoDataFound : public DBError
{
public:
    NoDataFound(const std::string &msg = "");
};

class BadSQLOperation : public DBError
{
public:
    BadSQLOperation(const std::string &msg);
};

class BadOperationInMode : public DBError
{
public:
    BadOperationInMode(const std::string &msg);
};

class SqlDialectError : public DBError
{
public:
    SqlDialectError(const std::string &msg);
};

class SqlDriverError : public DBError
{
public:
    SqlDriverError(const std::string &msg);
};

class SqlDialect
{
    std::string name_, dual_;
    bool has_sequences_;
    SqlDialect(const SqlDialect &);
    SqlDialect &operator=(const SqlDialect &);
public:
    SqlDialect(const std::string &name, const std::string &dual,
            bool has_sequences)
        : name_(name)
        , dual_(dual)
        , has_sequences_(has_sequences)
    {}
    virtual ~SqlDialect();
    const std::string &get_name() { return name_; }
    const std::string &dual_name() { return dual_; }
    bool has_sequences() { return has_sequences_; }
    virtual const std::string select_curr_value(
            const std::string &seq_name) = 0;
    virtual const std::string select_next_value(
            const std::string &seq_name) = 0;
    virtual const std::string sql_value(const Value &x) = 0;
};

SqlDialect *sql_dialect(const std::string &name);
bool register_sql_dialect(std::auto_ptr<SqlDialect> dialect);
const Strings list_sql_dialects();

typedef ValueMap Row;
typedef std::auto_ptr<Row> RowPtr;
typedef std::vector<Row> Rows;
typedef std::auto_ptr<Rows> RowsPtr;

class SqlConnectBackend
{
    SqlConnectBackend(const SqlConnectBackend &);
    SqlConnectBackend &operator=(const SqlConnectBackend &);
public:
    SqlConnectBackend() {}
    virtual ~SqlConnectBackend();
    virtual void open(SqlDialect *dialect, const std::string &db,
        const std::string &user, const std::string &passwd) = 0;
    virtual void close() = 0;
    virtual void commit() = 0;
    virtual void rollback() = 0;
    virtual void exec_direct(const std::string &sql) = 0;
    virtual RowPtr fetch_row() = 0;
    virtual void prepare(const std::string &sql) = 0;
    virtual void exec(const Values &params) = 0;
};

class SqlDriver
{
    std::string name_;
    SqlDriver(const SqlDriver &);
    SqlDriver &operator=(const SqlDriver &);
public:
    SqlDriver(const std::string &name)
        : name_(name)
    {}
    virtual ~SqlDriver();
    const std::string &get_name() { return name_; }
    virtual std::auto_ptr<SqlConnectBackend> create_backend() = 0;
};

SqlDriver *sql_driver(const std::string &name);
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
    std::string db_, user_;
    std::auto_ptr<SqlConnectBackend> backend_;
    bool activity_, echo_;
    SqlConnect(const SqlConnect &);
    SqlConnect &operator=(const SqlConnect &);
public:
    SqlConnect(const std::string &driver_name,
            const std::string &dialect_name, const std::string &db,
            const std::string &user = "", const std::string &passwd = "");
    ~SqlConnect();
    SqlDriver *get_driver() { return driver_; }
    SqlDialect *get_dialect() { return dialect_; }
    const std::string &get_db() { return db_; }
    const std::string &get_user() { return user_; }
    void set_echo(bool echo) { echo_ = echo; }
    SqlResultSet exec_direct(const std::string &sql);
    RowPtr fetch_row();
    RowsPtr fetch_rows(int max_rows = -1); // -1 = all
    void prepare(const std::string &sql);
    SqlResultSet exec(const Values &params);
    void commit();
    void rollback();
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__SQL_DRIVER__INCLUDED
