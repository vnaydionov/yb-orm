// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__ORM__DRIVER_QTSQL__INCLUDED
#define YB__ORM__DRIVER_QTSQL__INCLUDED

#include <memory>
#include <vector>
#include "util/thread.h"
#include "orm/sql_driver.h"
#include <QtSql>

namespace Yb {

class QtSqlCursorBackend: public SqlCursorBackend
{
    QSqlDatabase *conn_;
    std::auto_ptr<QSqlQuery> stmt_;
    std::auto_ptr<QSqlRecord> rec_;
    std::vector<QVariant> bound_;
    void clear();
public:
    QtSqlCursorBackend(QSqlDatabase *conn);
    void exec_direct(const String &sql);
    void prepare(const String &sql);
    void exec(const Values &params);
    RowPtr fetch_row();
};

class QtSqlDriver;

class QtSqlConnectionBackend: public SqlConnectionBackend
{
    QSqlDatabase *conn_;
    String conn_name_;
    QtSqlDriver *drv_;
    bool own_handle_;
public:
    QtSqlConnectionBackend(QtSqlDriver *drv);
    void open(SqlDialect *dialect, const SqlSource &source);
    void use_raw(SqlDialect *dialect, void *raw_connection);
    void *get_raw();
    std::auto_ptr<SqlCursorBackend> new_cursor();
    void close();
    void begin_trans();
    void commit();
    void rollback();
};

class QtSqlDriver: public SqlDriver
{
    friend class QtSqlConnectionBackend;
    int seq_;
    Mutex conn_mux_;
public:
    QtSqlDriver(bool use_qodbc);
    std::auto_ptr<SqlConnectionBackend> create_backend();
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__DRIVER_QTSQL__INCLUDED
