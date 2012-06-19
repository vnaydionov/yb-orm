#include <orm/QtSqlDriver.h>
#include <util/str_utils.hpp>

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
            else if (t == QVariant::Double) {
                try {
                    v = Value(Decimal(stmt_->value(i).toDouble()));
                }
                catch (const decimal::exception &) {
                    v = Value(stmt_->value(i).toString());
                }
            }
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
        conn_name_ = dialect->get_name() + _T("_") + source.get_db()
            + _T("_") + to_string(drv_->seq_);
        ++drv_->seq_;
    }
    String driver = source.get_lowlevel_driver();
    if (str_empty(driver))
    {
        if (dialect->get_name() == _T("MYSQL"))
            driver = _T("QMYSQL");
        else if (dialect->get_name() == _T("POSTGRES"))
            driver = _T("QPSQL");
        else if (dialect->get_name() == _T("ORACLE"))
            driver = _T("QOCI");
        else if (dialect->get_name() == _T("INTERBASE"))
            driver = _T("QIBASE");
    }
    conn_.reset(new QSqlDatabase(QSqlDatabase::addDatabase(driver, conn_name_)));
    conn_->setDatabaseName(source.get_db());
    conn_->setUserName(source.get_user());
    conn_->setPassword(source.get_passwd());
    if (source.get_port() > 0)
        conn_->setPort(source.get_port());
    if (!str_empty(source.get_host()))
        conn_->setHostName(source.get_host());
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

QtSqlDriver::QtSqlDriver()
    : SqlDriver(_T("QTSQL"))
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
