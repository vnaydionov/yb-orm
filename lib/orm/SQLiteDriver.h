#ifndef YB__ORM__SQLITE_DRIVER__INCLUDED
#define YB__ORM__SQLITE_DRIVER__INCLUDED

#include <memory>
#include <vector>
#include <util/Thread.h>
#include <orm/SqlDriver.h>
#include <sqlite3.h>

namespace Yb {

typedef ::sqlite3 SQLiteDatabase;
typedef ::sqlite3_stmt SQLiteQuery;

class SQLiteCursorBackend: public SqlCursorBackend
{
    SQLiteDatabase *conn_;
    SQLiteQuery *stmt_;
    int last_code_, exec_count_;
public:
    SQLiteCursorBackend(SQLiteDatabase *conn);
    ~SQLiteCursorBackend();
    void close();
    void exec_direct(const String &sql);
    void prepare(const String &sql);
    void exec(const Values &params);
    RowPtr fetch_row();
};

class SQLiteDriver;

class SQLiteConnectionBackend: public SqlConnectionBackend
{
    SQLiteDatabase *conn_;
    SQLiteDriver *drv_;
public:
    SQLiteConnectionBackend(SQLiteDriver *drv);
    ~SQLiteConnectionBackend();
    void open(SqlDialect *dialect, const SqlSource &source);
    std::auto_ptr<SqlCursorBackend> new_cursor();
    void close();
    void begin_trans();
    void commit();
    void rollback();
};

class SQLiteDriver: public SqlDriver
{
    friend class SQLiteConnectionBackend;
    Mutex conn_mux_;
public:
    SQLiteDriver();
    std::auto_ptr<SqlConnectionBackend> create_backend();
    void parse_url_tail(const String &dialect_name,
            const String &url_tail, StringDict &source);
};

} //namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__SQLITE_DRIVER__INCLUDED
