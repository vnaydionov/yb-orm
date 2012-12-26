#include <orm/OdbcDriver.h>
#include <util/str_utils.hpp>

using namespace std;
using Yb::StrUtils::str_to_upper;

namespace Yb {

OdbcCursorBackend::OdbcCursorBackend(tiodbc::connection *conn)
    : conn_(conn)
{}

void
OdbcCursorBackend::exec_direct(const String &sql)
{
    stmt_.reset(NULL);
    stmt_.reset(new tiodbc::statement());
    if (!stmt_->execute_direct(*conn_, sql))
        throw DBError(stmt_->last_error_ex());
}

void
OdbcCursorBackend::prepare(const String &sql)
{
    stmt_.reset(NULL);
    stmt_.reset(new tiodbc::statement());
    if (!stmt_->prepare(*conn_, sql))
        throw DBError(stmt_->last_error_ex());
}

void
OdbcCursorBackend::exec(const Values &params)
{
    for (unsigned i = 0; i < params.size(); ++i) {
        if (params[i].get_type() == Value::DATETIME) {
            TIMESTAMP_STRUCT ts;
            memset(&ts, 0, sizeof(ts));
            DateTime t = params[i].as_date_time();
            ts.year = dt_year(t);
            ts.month = dt_month(t);
            ts.day = dt_day(t);
            ts.hour = (SQLUSMALLINT)dt_hour(t);
            ts.minute = (SQLUSMALLINT)dt_minute(t);
            ts.second = (SQLUSMALLINT)dt_second(t);
            ts.fraction = 0; // TODO
            stmt_->param(i + 1).set_as_date_time(
                    ts,
                    params[i].is_null());
        }
        else if (params[i].get_type() == Value::INTEGER) {
            long x = params[i].as_integer();
            stmt_->param(i + 1).set_as_long(x, params[i].is_null());
        }
        else {
            String value;
            if (!params[i].is_null())
                value = params[i].as_string();
            stmt_->param(i + 1).set_as_string(value,
                                              params[i].is_null());
        }
    }
    if (!stmt_->execute())
        throw DBError(stmt_->last_error_ex());
}

RowPtr
OdbcCursorBackend::fetch_row()
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
                v = Value(dt_make(ts.year, ts.month, ts.day,
                            ts.hour, ts.minute, ts.second));
            }
        }
        else if (!f.is_null() && f.get_type() == SQL_INTEGER) {
            v = Value((int)f.as_long());
        }
        else if (!f.is_null() && f.get_type() == SQL_BIGINT) {
            LongInt x;
            v = Value(from_string(f.as_string(), x));
        }
        else {
            String val = f.as_string();
            if (f.is_null() != 1)
                v = Value(val);
        }
        row->push_back(make_pair(name, v));
    }
    return row;
}

OdbcConnectionBackend::OdbcConnectionBackend(OdbcDriver *drv)
    : drv_(drv)
{}

void
OdbcConnectionBackend::open(SqlDialect *dialect, const SqlSource &source)
{
    close();
    ScopedLock lock(drv_->conn_mux_);
    conn_.reset(new tiodbc::connection());
    if (!conn_->connect(source.db(), source.user(), source.passwd(),
                source.get_as<int>(String(_T("timeout")), 10),
                source.get_as<int>(String(_T("autocommit")), 0)))
        throw DBError(conn_->last_error_ex());
}

auto_ptr<SqlCursorBackend>
OdbcConnectionBackend::new_cursor()
{
    auto_ptr<SqlCursorBackend> p(
            (SqlCursorBackend *)new OdbcCursorBackend(conn_.get()));
    return p;
}

void
OdbcConnectionBackend::close()
{
    ScopedLock lock(drv_->conn_mux_);
    conn_.reset(NULL);
}

void
OdbcConnectionBackend::commit()
{
    if (!conn_->commit())
        throw DBError(conn_->last_error_ex());
}

void
OdbcConnectionBackend::rollback()
{
    if (!conn_->rollback())
        throw DBError(conn_->last_error_ex());
}

OdbcDriver::OdbcDriver():
    SqlDriver(_T("ODBC"))
{}

auto_ptr<SqlConnectionBackend>
OdbcDriver::create_backend()
{
    auto_ptr<SqlConnectionBackend> p(
            (SqlConnectionBackend *)new OdbcConnectionBackend(this));
    return p;
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
