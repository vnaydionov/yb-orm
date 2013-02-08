#include <orm/SOCIDriver.h>
#include <util/str_utils.hpp>
#include <ctime>
#include <cstring>

using namespace std;
using Yb::StrUtils::str_to_upper;

//#define YB_SOCI_DEBUG

namespace Yb {

SOCICursorBackend::SOCICursorBackend(SOCIDatabase *conn)
    :conn_(conn), stmt_(NULL), exec_count_(0)
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
            exec_count_ = 0;
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
    vector<int> pos_list;
    String first_word;
    if (!find_subst_signs(sql, pos_list, first_word))
        throw DBError(_T("SQL syntax error"));
    vector<String> parts;
    split_by_subst_sign(sql, pos_list, parts);
    String sql2 = parts[0];
    for (int i = 1; i < parts.size(); ++i) {
        sql2 += _T(":") + to_string(i);
        sql2 += parts[i];
    }
    sql_ = NARROW(sql2);
    is_select_ = str_to_upper(first_word) == _T("SELECT");
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
SOCICursorBackend::exec(const Values &params)
{
    try {
        if (!exec_count_) {
            in_params_.resize(params.size());
            in_flags_.resize(params.size());
        }
        else {
            delete stmt_;
            stmt_ = new soci::statement(*conn_);
            stmt_->alloc();
            stmt_->prepare(sql_);
        }
        ++exec_count_;
        for (size_t i = 0; i < params.size(); ++i) {
            const Value &param = params[i];
            in_flags_[i] = soci::i_ok;
            if (param.get_type() == Value::INVALID) {
                if (in_params_[i].size() < sizeof(int))
                    in_params_[i].resize(sizeof(int));
                strcpy(&(in_params_[i][0]), "");
                in_flags_[i] = soci::i_null;
                stmt_->exchange(soci::use(in_params_[i], in_flags_[i]));
            }
            else if (param.get_type() == Value::INTEGER) {
                if (in_params_[i].size() < sizeof(int))
                    in_params_[i].resize(sizeof(int));
                int &x = *(int *)&(in_params_[i][0]);
                x = param.as_integer();
                stmt_->exchange(soci::use(x));
            }
            else if (param.get_type() == Value::LONGINT) {
                if (in_params_[i].size() < sizeof(LongInt))
                    in_params_[i].resize(sizeof(LongInt));
                LongInt &x = *(LongInt *)&(in_params_[i][0]);
                x = param.as_longint();
                stmt_->exchange(soci::use(x));
            }
            else if (param.get_type() == Value::DATETIME) {
                if (in_params_[i].size() < sizeof(std::tm))
                    in_params_[i].resize(sizeof(std::tm));
                std::tm &when = *(std::tm *)&(in_params_[i][0]);
                memset(&when, 0, sizeof(when));
                DateTime d = param.as_date_time();
                when.tm_year = dt_year(d) - 1900;
                when.tm_mon = dt_month(d) - 1;
                when.tm_mday = dt_day(d);
                when.tm_hour = dt_hour(d);
                when.tm_min = dt_minute(d);
                when.tm_sec = dt_second(d);
                stmt_->exchange(soci::use(when));
            }
            else {
                string s = NARROW(param.as_string());
                if (in_params_[i].size() < s.size())
                    in_params_[i].resize(s.size());
                strcpy(&(in_params_[i][0]), s.c_str());
                stmt_->exchange(soci::use(in_params_[i]));
            }
        }
        if (is_select_)
            stmt_->exchange(soci::into(row_));
        stmt_->define_and_bind();
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
                    v = Value(Decimal(row_.get<double>(i)));
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
    : conn_(NULL), drv_(drv)
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
    try {
        conn_ = new SOCIDatabase(soci_convert_dialect(dialect->get_name()),
                NARROW(source.db()));
#ifdef YB_SOCI_DEBUG
        conn_->set_log_stream(&cerr);
#endif
    }
    catch (const soci::soci_error &e) {
        throw DBError(WIDEN(e.what()));
    }
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
        if (conn_) {
            delete conn_;
            conn_ = NULL;
        }
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

SOCIDriver::SOCIDriver():
    SqlDriver(_T("SOCI"))
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

} //namespace Yb

// vim:ts=4:sts=4:sw=4:et:
