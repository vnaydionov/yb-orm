#ifndef YB__ORM__SQLITE_DRIVER__INCLUDED
#define YB__ORM__SQLITE_DRIVER__INCLUDED

#include <memory>
#include <vector>
#include <util/Thread.h>
#include <orm/SqlDriver.h>
#include <sqlite3.h>

typedef sqlite3 SQLiteDatabase;
typedef sqlite3_stmt SQLiteQuery;

namespace Yb {

class SQLiteCursorBackend: public SqlCursorBackend
{
    SQLiteDatabase *conn_;
    std::auto_ptr<SQLiteQuery> stmt_;
//  SQLiteQuery *stmt_;
public:
    SQLiteCursorBackend(SQLiteDatabase *conn);
    void exec_direct(const String &sql);
    void prepare(const String &sql);
    void exec(const Values &params);
    RowPtr fetch_row(); 
};

class SQLiteDriver;

class SQLiteConnectionBackend: public SqlConnectionBackend
{
    //std::auto_ptr<SQLiteDatabase> conn_;
    SQLiteDatabase *conn_;
    SQLiteDriver *drv_;
public:
    SQLiteConnectionBackend(SQLiteDriver *drv);
    ~SQLiteConnectionBackend();
    void open(SqlDialect *dialect, const SqlSource &source);
    std::auto_ptr<SqlCursorBackend> new_cursor();
    void close();
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
};

} //namespace Yb 

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__SQLITE_DRIVER__INCLUDED
