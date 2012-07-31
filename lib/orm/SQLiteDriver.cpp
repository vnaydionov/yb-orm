#include <orm/SQLiteDriver.h>
#include <util/str_utils.hpp>

using namespace std;

namespace Yb {

SQLiteCursorBackend::SQLiteCursorBackend(SQLiteDatabase *conn)
    :conn_(conn), stmt_(NULL)
{}

SQLiteCursorBackend::~SQLiteCursorBackend()
{
    close();
}

void
SQLiteCursorBackend::close()
{
    if (stmt_) {
        sqlite3_finalize(stmt_);
        stmt_ = NULL;
    }
}

/*static int
exec_callback(void *NotUsed, int argc, char **argv, char **azColName)
{
    int i;
    for(i = 0; i < argc; i++) {
        cout << azColName[i] <<" = " << (argv[i] ? argv[i] : "NULL") << "\n";
    }
    cout << endl;
    return 0;
}*/

void
SQLiteCursorBackend::exec_direct(const String &sql)
{
    int rs = sqlite3_exec(conn_, NARROW(sql).c_str(), 0, 0, 0);
    if (SQLITE_OK != sqlite3_errcode(conn_)) {
        const char *err = sqlite3_errmsg(conn_);
        throw DBError(WIDEN(err));
    }
}

void
SQLiteCursorBackend::prepare(const String &sql)
{
    close();
    sqlite3_prepare_v2(conn_, NARROW(sql).c_str(), -1, &stmt_, 0);
    if (SQLITE_OK != sqlite3_errcode(conn_)) {
        const char *err = sqlite3_errmsg(conn_);
        throw DBError(WIDEN(err));
    }
}

void
SQLiteCursorBackend::exec(const Values &params)
{
    for (size_t i = 0; i < params.size(); ++i) {
        if (params[i].get_type() == Value::DATETIME) {
            sqlite3_bind_text(stmt_, i + 1, params[i].as_string().c_str(), (params[i].as_string()).length(), SQLITE_STATIC);
            if (SQLITE_OK != sqlite3_errcode(conn_)) {
                const char *err = sqlite3_errmsg(conn_);
                throw DBError(WIDEN(err));
            }
        } else if (params[i].get_type() == Value::INTEGER) {
            sqlite3_bind_int(stmt_, i + 1, params[i].as_integer());
            if (SQLITE_OK != sqlite3_errcode(conn_)) {
                const char *err = sqlite3_errmsg(conn_);
                throw DBError(WIDEN(err));
            }
        } else {
            sqlite3_bind_text(stmt_, i + 1, params[i].as_string().c_str(), (params[i].as_string()).length(), SQLITE_STATIC);
            if (SQLITE_OK != sqlite3_errcode(conn_)) {
                const char *err = sqlite3_errmsg(conn_);
                throw DBError(WIDEN(err));
            }
        }
    }
}

RowPtr SQLiteCursorBackend::fetch_row()
{
    RowPtr q;
    sqlite3_step(stmt_);
    // TODO: analyze the result and fill the row
    if (SQLITE_OK != sqlite3_errcode(conn_)) {
        const char *err = sqlite3_errmsg(conn_);
        throw DBError(WIDEN(err));
    }
    // ...
    return q;
}

SQLiteConnectionBackend::SQLiteConnectionBackend(SQLiteDriver *drv)
    : conn_(NULL), drv_(drv)
{}

SQLiteConnectionBackend::~SQLiteConnectionBackend()
{
    close();
}

void
SQLiteConnectionBackend::open(SqlDialect *dialect, const SqlSource &source)
{
    close();
    ScopedLock lock(drv_->conn_mux_);
    sqlite3_open_v2(NARROW(source.db()).c_str(), &conn_,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (SQLITE_OK != sqlite3_errcode(conn_)) {
        conn_ = NULL;
        const char *err = sqlite3_errmsg(conn_);
        throw DBError(WIDEN(err));
    }
}

auto_ptr<SqlCursorBackend>
SQLiteConnectionBackend::new_cursor()
{
    auto_ptr<SqlCursorBackend> p(
            (SqlCursorBackend *)new SQLiteCursorBackend(conn_));
    return p;
}

void SQLiteConnectionBackend::close()
{
    ScopedLock lock(drv_->conn_mux_);
    if (conn_) {
        sqlite3_close(conn_);
        conn_ = NULL;
    }
}

void
SQLiteConnectionBackend::commit()
{
    sqlite3_exec(conn_, _T("COMMIT"), NULL, 0, 0);
    if (SQLITE_OK != sqlite3_errcode(conn_)) {
        const char *err = sqlite3_errmsg(conn_);
        throw DBError(WIDEN(err));
    }
}

void
SQLiteConnectionBackend::rollback()
{
    sqlite3_exec(conn_, _T("ROLLBACK"), NULL, 0, 0);
    if (SQLITE_OK != sqlite3_errcode(conn_)) {
        const char *err = sqlite3_errmsg(conn_);
        throw DBError(WIDEN(err));
    }
}

SQLiteDriver::SQLiteDriver():
    SqlDriver(_T("SQLITE"))
{}


auto_ptr<SqlConnectionBackend>
SQLiteDriver::create_backend()
{
    auto_ptr<SqlConnectionBackend> p(
            (SqlConnectionBackend *)new SQLiteConnectionBackend(this));
    return p;
}

} //namespace Yb

// vim:ts=4:sts=4:sw=4:et:
