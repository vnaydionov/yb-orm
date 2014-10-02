// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#define YBORM_SOURCE

#include "soci_driver.h"
#include "util/string_utils.h"
#include <ctime>
#include <cstring>

using namespace std;
using namespace Yb::StrUtils;

//#define YB_SOCI_DEBUG

namespace Yb {

SOCICursorBackend::SOCICursorBackend(soci::session *conn)
    : conn_(conn), stmt_(NULL), is_select_(false)
    , bound_first_(false), executed_(false)
{}

SOCICursorBackend::~SOCICursorBackend()
{
    close();
}

void
SOCICursorBackend::close()
{
    try {
        if (stmt_) {
            delete stmt_;
            stmt_ = NULL;
            is_select_ = false;
            bound_first_ = false;
            executed_ = false;
            sql_.empty();
        }
    }
    catch (const soci::soci_error &e) {
        throw DBError(WIDEN(e.what()));
    }
}

void
SOCICursorBackend::exec_direct(const String &sql)
{
    try {
        conn_->once << NARROW(sql);
    }
    catch (const soci::soci_error &e) {
        throw DBError(WIDEN(e.what()));
    }
}

void
SOCICursorBackend::prepare(const String &sql)
{
    close();
    sql_ = NARROW(sql);
    is_select_ = starts_with(
            str_to_upper(trim_trailing_space(sql)), _T("SELECT"));
    try {
        stmt_ = new soci::statement(*conn_);
        stmt_->alloc();
        stmt_->prepare(sql_);
    }
    catch (const soci::soci_error &e) {
        throw DBError(WIDEN(e.what()));
    }
}

void
SOCICursorBackend::bind_params(const TypeCodes &types)
{
    param_types_ = types;
    if (in_params_.size())
        throw DBError(_T("bind_params: already bound!"));
    in_params_.resize(types.size());
    in_flags_.resize(types.size());
    for (size_t i = 0; i < types.size(); ++i) {
        in_flags_[i] = soci::i_null;
        switch (types[i]) {
            case Value::INTEGER: {
                if (in_params_[i].size() < sizeof(int))
                    in_params_[i].resize(sizeof(int));
                int &x = *(int *)&(in_params_[i][0]);
                x = 0;
                stmt_->exchange(soci::use(x, in_flags_[i]));
                break;
            }
            case Value::LONGINT: {
                if (in_params_[i].size() < sizeof(LongInt))
                    in_params_[i].resize(sizeof(LongInt));
                LongInt &x = *(LongInt *)&(in_params_[i][0]);
                x = 0;
                stmt_->exchange(soci::use(x, in_flags_[i]));
                break;
            }
            case Value::FLOAT: {
                if (in_params_[i].size() < sizeof(double))
                    in_params_[i].resize(sizeof(double));
                double &x = *(double *)&(in_params_[i][0]);
                x = 0;
                stmt_->exchange(soci::use(x, in_flags_[i]));
                break;
            }
            case Value::DATETIME: {
                if (in_params_[i].size() < sizeof(std::tm))
                    in_params_[i].resize(sizeof(std::tm));
                std::tm &x = *(std::tm *)&(in_params_[i][0]);
                memset(&x, 0, sizeof(x));
                stmt_->exchange(soci::use(x, in_flags_[i]));
                break;
            }
            default: {
                stmt_->exchange(soci::use(in_params_[i], in_flags_[i]));
            }
        }
    }
    if (is_select_)
        stmt_->exchange(soci::into(row_));
    stmt_->define_and_bind();
}

void
SOCICursorBackend::exec(const Values &params)
{
    try {
        if (!executed_ && in_params_.size())
            bound_first_ = true;
        if (!bound_first_) {
            if (executed_) {
                delete stmt_;
                stmt_ = new soci::statement(*conn_);
                stmt_->alloc();
                stmt_->prepare(sql_);
                in_params_.clear();
                in_flags_.clear();
            }
            TypeCodes types(params.size());
            for (size_t i = 0; i < params.size(); ++i)
                types[i] = params[i].get_type();
            bind_params(types);
        }
        executed_ = true;
        for (size_t i = 0; i < params.size(); ++i) {
            const Value &param = params[i];
            if (param.is_null()) {
                in_flags_[i] = soci::i_null;
                continue;
            }
            in_flags_[i] = soci::i_ok;
            switch (param_types_[i]) {
                case Value::INTEGER: {
                    int &x = *(int *)&(in_params_[i][0]);
                    x = param.as_integer();
                    break;
                }
                case Value::LONGINT: {
                    LongInt &x = *(LongInt *)&(in_params_[i][0]);
                    x = param.as_longint();
                    break;
                }
                case Value::FLOAT: {
                    double &x = *(double *)&(in_params_[i][0]);
                    x = param.as_float();
                    break;
                }
                case Value::DATETIME: {
                    std::tm &x = *(std::tm *)&(in_params_[i][0]);
                    memset(&x, 0, sizeof(x));
                    const DateTime &d = param.as_date_time();
                    x.tm_year = dt_year(d) - 1900;
                    x.tm_mon = dt_month(d) - 1;
                    x.tm_mday = dt_day(d);
                    x.tm_hour = dt_hour(d);
                    x.tm_min = dt_minute(d);
                    x.tm_sec = dt_second(d);
                    break;
                }
                default: {
                    in_params_[i] = NARROW(param.as_string());
                }
            }
        }
        stmt_->execute(!is_select_);
    }
    catch (const soci::soci_error &e) {
        throw DBError(WIDEN(e.what()));
    }
}

RowPtr SOCICursorBackend::fetch_row()
{
    try {
        if (!stmt_->fetch()) {
#ifdef YB_SOCI_DEBUG
            cerr << "fetch(): false" << endl;
#endif
            return RowPtr();
        }
        RowPtr result(new Row);
        int col_count = row_.size();
#ifdef YB_SOCI_DEBUG
        cerr << "fetch(): col_count=" << col_count << endl;
#endif
        for (int i = 0; i < col_count; ++i) {
            const soci::column_properties &props = row_.get_properties(i);
            String name = str_to_upper(WIDEN(props.get_name()));
            Value v;
            if (row_.get_indicator(i) != soci::i_null) {
                std::tm when;
                unsigned long long x;
                switch (props.get_data_type()) {
                case soci::dt_string:
                    v = Value(WIDEN(row_.get<string>(i)));
                    break;
                case soci::dt_double:
                    v = Value(row_.get<double>(i));
                    break;
                case soci::dt_integer:
                    v = Value(row_.get<int>(i));
                    break;
#if 0
                case soci::dt_unsigned_long:
                    x = row_.get<unsigned long>(i);
                    v = Value((LongInt)x);
                    break;
#endif
                case soci::dt_long_long:
                    x = row_.get<long long>(i);
                    v = Value((LongInt)x);
                    break;
                case soci::dt_unsigned_long_long:
                    x = row_.get<unsigned long long>(i);
                    v = Value((LongInt)x);
                    break;
                case soci::dt_date:
                    when = row_.get<std::tm>(i);
                    v = Value(dt_make(when.tm_year + 1900, when.tm_mon + 1,
                                when.tm_mday, when.tm_hour,
                                when.tm_min, when.tm_sec));
                    break;
                }
            }
#ifdef YB_SOCI_DEBUG
            cerr << "fetch(): col[" << i << "]: name=" << props.get_name()
                << " type=" << props.get_data_type()
                << " value=" << NARROW(v.sql_str()) << endl;
#endif
            result->push_back(make_pair(name, v));
        }
        return result;
    }
    catch (const soci::soci_error &e) {
        throw DBError(WIDEN(e.what()));
    }
}

SOCIConnectionBackend::SOCIConnectionBackend(SOCIDriver *drv)
    : conn_(NULL), drv_(drv), own_handle_(false)
{}

SOCIConnectionBackend::~SOCIConnectionBackend()
{
    close();
}

static const string soci_convert_dialect(const String &dialect_name)
{
    if (dialect_name == _T("POSTGRES"))
        return "postgresql";
    if (dialect_name == _T("MYSQL"))
        return "mysql";
    if (dialect_name == _T("ORACLE"))
        return "oracle";
    if (dialect_name == _T("SQLITE"))
        return "sqlite3";
    if (dialect_name == _T("INTERBASE"))
        return "firebird";
    throw SqlDialectError(_T("Unsupported dialect: ") + dialect_name);
}

void
SOCIConnectionBackend::open(SqlDialect *dialect, const SqlSource &source)
{
    close();
    ScopedLock lock(drv_->conn_mux_);
    own_handle_ = true;
    try {
        String driver = source.driver();
        std::string soci_backend = "odbc";
        if (driver == _T("SOCI"))
            soci_backend = soci_convert_dialect(dialect->get_name());
        conn_ = new soci::session(soci_backend, NARROW(source.db()));
#ifdef YB_SOCI_DEBUG
        conn_->set_log_stream(&cerr);
#endif
    }
    catch (const soci::soci_error &e) {
        throw DBError(WIDEN(e.what()));
    }
}

void
SOCIConnectionBackend::use_raw(SqlDialect *dialect, void *raw_connection)
{
    close();
    ScopedLock lock(drv_->conn_mux_);
    conn_ = (soci::session *)raw_connection;
}

void *
SOCIConnectionBackend::get_raw()
{
    return (void *)conn_;
}

auto_ptr<SqlCursorBackend>
SOCIConnectionBackend::new_cursor()
{
    auto_ptr<SqlCursorBackend> p(
            (SqlCursorBackend *)new SOCICursorBackend(conn_));
    return p;
}

void SOCIConnectionBackend::close()
{
    ScopedLock lock(drv_->conn_mux_);
    try {
        if (own_handle_) {
            if (conn_)
                delete conn_;
            own_handle_ = false;
        }
        conn_ = NULL;
    }
    catch (const soci::soci_error &e) {
        throw DBError(WIDEN(e.what()));
    }
}

void
SOCIConnectionBackend::begin_trans()
{
    try {
        conn_->begin();
    }
    catch (const soci::soci_error &e) {
        throw DBError(WIDEN(e.what()));
    }
}

void
SOCIConnectionBackend::commit()
{
    try {
        conn_->commit();
    }
    catch (const soci::soci_error &e) {
        throw DBError(WIDEN(e.what()));
    }
}

void
SOCIConnectionBackend::rollback()
{
    try {
        conn_->rollback();
    }
    catch (const soci::soci_error &e) {
        throw DBError(WIDEN(e.what()));
    }
}

SOCIDriver::SOCIDriver(bool use_odbc):
    SqlDriver(use_odbc? _T("SOCI_ODBC"): _T("SOCI"))
{}

auto_ptr<SqlConnectionBackend>
SOCIDriver::create_backend()
{
    auto_ptr<SqlConnectionBackend> p(
            (SqlConnectionBackend *)new SOCIConnectionBackend(this));
    return p;
}

void
SOCIDriver::parse_url_tail(const String &dialect_name,
        const String &url_tail, StringDict &source)
{
    source.set(_T("&db"), url_tail);
}

bool
SOCIDriver::numbered_params()
{
    return true;
}

} //namespace Yb

// vim:ts=4:sts=4:sw=4:et:
