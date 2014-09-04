// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#define YBORM_SOURCE

#include "qtsql_driver.h"
#include "util/string_utils.h"

using namespace std;
using Yb::StrUtils::str_to_upper;

namespace Yb {

QtSqlCursorBackend::QtSqlCursorBackend(QSqlDatabase *conn)
    : conn_(conn)
{}

void
QtSqlCursorBackend::clear()
{
    bound_.clear();
    rec_.reset(NULL);
    if (stmt_.get())
        stmt_->clear();
    stmt_.reset(NULL);
}

void
QtSqlCursorBackend::exec_direct(const String &sql)
{
    clear();
    stmt_.reset(new QSqlQuery(*conn_));
    stmt_->setForwardOnly(true);
    if (!stmt_->exec(sql))
        throw DBError(stmt_->lastError().text());
}

void
QtSqlCursorBackend::prepare(const String &sql)
{
    clear();
    stmt_.reset(new QSqlQuery(*conn_));
    stmt_->setForwardOnly(true);
    if (!stmt_->prepare(sql))
        throw DBError(stmt_->lastError().text());
}

void
QtSqlCursorBackend::exec(const Values &params)
{
    if (bound_.empty())
        for (unsigned i = 0; i < params.size(); ++i)
            bound_.push_back(QVariant());
    for (unsigned i = 0; i < params.size(); ++i) {
        if (params[i].is_null())
            bound_[i] = QVariant();
        else if (params[i].get_type() == Value::DATETIME)
            bound_[i] = QVariant(params[i].as_date_time());
        else if (params[i].get_type() == Value::INTEGER)
            bound_[i] = QVariant(params[i].as_integer());
        else if (params[i].get_type() == Value::LONGINT)
            bound_[i] = QVariant(params[i].as_longint());
        else if (params[i].get_type() == Value::FLOAT)
            bound_[i] = QVariant(params[i].as_float());
        else
            bound_[i] = QVariant(params[i].as_string());
        stmt_->bindValue(i, bound_[i]);
    }
    if (!stmt_->exec())
        throw DBError(stmt_->lastError().text());
}

RowPtr
QtSqlCursorBackend::fetch_row()
{
    if (!stmt_->next())
        return RowPtr();
    if (!rec_.get())
        rec_.reset(new QSqlRecord(stmt_->record()));
    int col_count = rec_->count();
    RowPtr row(new Row);
    for (int i = 0; i < col_count; ++i) {
        String name = str_to_upper(rec_->fieldName(i));
        Value v;
        if (!stmt_->value(i).isNull()) {
            QVariant::Type t = rec_->field(i).type();
            if (t == QVariant::Bool || t == QVariant::Int ||
                    t == QVariant::UInt)
                v = Value(stmt_->value(i).toInt());
            else if (t == QVariant::LongLong || t == QVariant::ULongLong)
                v = Value(stmt_->value(i).toLongLong());
            else if (t == QVariant::DateTime || t == QVariant::Date)
                v = Value(stmt_->value(i).toDateTime());
            else if (t == QVariant::Double || (int)t == (int)QMetaType::Float)
                v = Value(stmt_->value(i).toDouble());
            else
                v = Value(stmt_->value(i).toString());
        }
        row->push_back(RowItem(name, v));
    }
    return row;
}

QtSqlConnectionBackend::QtSqlConnectionBackend(QtSqlDriver *drv)
    : drv_(drv)
{}

void
QtSqlConnectionBackend::open(SqlDialect *dialect, const SqlSource &source)
{
    {
        ScopedLock lock(drv_->conn_mux_);
        conn_name_ = dialect->get_name() + _T("_") + source.db()
            + _T("_") + to_string(drv_->seq_);
        ++drv_->seq_;
    }
    String driver = source.driver();
    bool eat_slash = false;
    if (driver == _T("QTSQL"))
    {
        if (dialect->get_name() == _T("MYSQL"))
            driver = _T("QMYSQL");
        else if (dialect->get_name() == _T("POSTGRES"))
            driver = _T("QPSQL");
        else if (dialect->get_name() == _T("ORACLE"))
            driver = _T("QOCI");
        else if (dialect->get_name() == _T("INTERBASE"))
            driver = _T("QIBASE");
        else if (dialect->get_name() == _T("SQLITE"))
            driver = _T("QSQLITE");
        if (dialect->native_driver_eats_slash()
                && !str_empty(source.db())
                && char_code(source.db()[0]) == '/')
            eat_slash = true;
    }
    conn_.reset(new QSqlDatabase(QSqlDatabase::addDatabase(driver, conn_name_)));
    if (eat_slash)
        conn_->setDatabaseName(str_substr(source.db(), 1));
    else
        conn_->setDatabaseName(source.db());
    conn_->setUserName(source.user());
    conn_->setPassword(source.passwd());
    if (source.port() > 0)
        conn_->setPort(source.port());
    if (!str_empty(source.host()))
        conn_->setHostName(source.host());
    String options;
    Strings keys = source.options();
    for (size_t i = 0; i < keys.size(); ++i) {
        if (!str_empty(options))
            options += _T(";");
        options += keys[i] + _T("=") + source[keys[i]];
    }
    if (!str_empty(options))
        conn_->setConnectOptions(options);
    if (!conn_->open())
        throw DBError(conn_->lastError().text());
}

auto_ptr<SqlCursorBackend>
QtSqlConnectionBackend::new_cursor()
{
    auto_ptr<SqlCursorBackend> p(
            (SqlCursorBackend *)new QtSqlCursorBackend(conn_.get()));
    return p;
}

void
QtSqlConnectionBackend::close()
{
    conn_->close();
    conn_.reset(NULL);
    if (!str_empty(conn_name_)) {
        QSqlDatabase::removeDatabase(conn_name_);
        conn_name_ = String();
    }
}

void
QtSqlConnectionBackend::begin_trans()
{
    if (!conn_->transaction())
        throw DBError(conn_->lastError().text());
}

void
QtSqlConnectionBackend::commit()
{
    if (!conn_->commit())
        throw DBError(conn_->lastError().text());
}

void
QtSqlConnectionBackend::rollback()
{
    if (!conn_->rollback())
        throw DBError(conn_->lastError().text());
}

QtSqlDriver::QtSqlDriver(bool use_qodbc)
    : SqlDriver(use_qodbc? _T("QODBC3"): _T("QTSQL"))
    , seq_(1000)
{}

auto_ptr<SqlConnectionBackend>
QtSqlDriver::create_backend()
{
    auto_ptr<SqlConnectionBackend> p(
            (SqlConnectionBackend *)new QtSqlConnectionBackend(this));
    return p;
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
