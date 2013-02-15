#ifndef YB__ORM__ODBC_DRIVER__INCLUDED
#define YB__ORM__ODBC_DRIVER__INCLUDED

#include <memory>
#include <util/Thread.h>
#include <orm/SqlDriver.h>
#include <orm/tiodbc.hpp>

namespace Yb {

class OdbcCursorBackend: public SqlCursorBackend
{
    tiodbc::connection *conn_;
    std::auto_ptr<tiodbc::statement> stmt_;
public:
    OdbcCursorBackend(tiodbc::connection *conn);
    void exec_direct(const String &sql);
    void prepare(const String &sql);
    void exec(const Values &params);
    RowPtr fetch_row();
};

class OdbcDriver;

class OdbcConnectionBackend: public SqlConnectionBackend
{
    std::auto_ptr<tiodbc::connection> conn_;
    OdbcDriver *drv_;
public:
    OdbcConnectionBackend(OdbcDriver *drv);
    void open(SqlDialect *dialect, const SqlSource &source);
    std::auto_ptr<SqlCursorBackend> new_cursor();
    void close();
    void begin_trans();
    void commit();
    void rollback();
};

class OdbcDriver: public SqlDriver
{
    friend class OdbcConnectionBackend;
    Mutex conn_mux_;
public:
    OdbcDriver();
    std::auto_ptr<SqlConnectionBackend> create_backend();
    bool explicit_begin_trans_required();
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__ODBC_DRIVER__INCLUDED
