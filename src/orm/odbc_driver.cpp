// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#define YBORM_SOURCE

#include "odbc_driver.h"
#include "util/string_utils.h"

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
    for (size_t i = 0; i < params.size(); ++i) {
        switch (params[i].get_type()) {
            case Value::INVALID: {
                stmt_->param(i + 1).set_as_null();
                break;
            }
            case Value::DATETIME: {
                const DateTime &t = params[i].read_as<DateTime>();
                TIMESTAMP_STRUCT ts;
                ts.year = dt_year(t);
                ts.month = dt_month(t);
                ts.day = dt_day(t);
                ts.hour = (SQLUSMALLINT)dt_hour(t);
                ts.minute = (SQLUSMALLINT)dt_minute(t);
                ts.second = (SQLUSMALLINT)dt_second(t);
                ts.fraction = dt_millisec(t) * 1000000;
                stmt_->param(i + 1).set_as_date_time(ts, false);
                break;
            }
            case Value::INTEGER: {
                const int &x = params[i].read_as<int>();
                stmt_->param(i + 1).set_as_long(x, false);
                break;
            }
            case Value::LONGINT: {
                const LongInt &x = params[i].read_as<LongInt>();
                stmt_->param(i + 1).set_as_long_long(x, false);
                break;
            }
            case Value::FLOAT: {
                const double &x = params[i].read_as<double>();
                stmt_->param(i + 1).set_as_double(x, false);
                break;
            }
            case Value::STRING: {
                const String &s = params[i].read_as<String>();
                stmt_->param(i + 1).set_as_string(s, false);
                break;
            }
            default: {
                String s = params[i].as_string();
                stmt_->param(i + 1).set_as_string(s, false);
            }
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
    row->reserve(col_count);
    for (int i = 0; i < col_count; ++i) {
        tiodbc::field_impl f = stmt_->field(i + 1);
        row->push_back(make_pair(str_to_upper(f.get_name()), Value()));
        Value &v = (*row)[row->size() - 1].second;
        switch (f.get_type()) {
            case SQL_DATE:
            case SQL_TIMESTAMP:
            case SQL_TYPE_DATE:
            case SQL_TYPE_TIME:
            case SQL_TYPE_TIMESTAMP: {
                TIMESTAMP_STRUCT ts = f.as_date_time();
                if (!f.is_null())
                    v = Value(dt_make(ts.year, ts.month, ts.day,
                                       ts.hour, ts.minute, ts.second,
                                       ts.fraction/1000000));
                break;
            }
            case SQL_INTEGER:
            case SQL_SMALLINT:
            case SQL_TINYINT: {
                int x = f.as_long();
                if (!f.is_null())
                    v = Value(x);
                break;
            }
            case SQL_BIGINT: {
                LongInt x = f.as_long_long();
                if (!f.is_null())
                    v = Value(x);
                break;
            }
            case SQL_REAL:
            case SQL_FLOAT:
            case SQL_DOUBLE: {
                double x = f.as_double();
                if (!f.is_null())
                    v = Value(x);
                break;
            }
            case SQL_DECIMAL:
            case SQL_NUMERIC: {
                String x = f.as_string();
                if (!f.is_null())
                    v = Value(Decimal(x));
                break;
            }
            default: {
                String x = f.as_string();
                if (!f.is_null())
                    v = Value(x);
            }
        }
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
                bool(source.get_as<int>(String(_T("autocommit"))), 0)))
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
OdbcConnectionBackend::begin_trans()
{
    if (!conn_->begin_trans())
        throw DBError(conn_->last_error_ex());
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

bool
OdbcDriver::explicit_begin_trans_required()
{
    return false;
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
