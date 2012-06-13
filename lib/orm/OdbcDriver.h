#ifndef YB__ORM__ODBC_DRIVER__INCLUDED
#define YB__ORM__ODBC_DRIVER__INCLUDED

#include <string>
#include <vector>
#include <map>
#include <iterator>
#include <util/Utility.h>
#include <util/Thread.h>
#include <util/ResultSet.h>
#include <util/nlogger.h>
#include <orm/Value.h>

#include <memory>
#include <orm/SqlDriver.h>
#include <orm/tiodbc.hpp>

namespace Yb {

class OdbcDriver;

class OdbcConnectBackend: public SqlConnectBackend
{
    std::auto_ptr<tiodbc::connection> conn_;
    std::auto_ptr<tiodbc::statement> stmt_;
    OdbcDriver *drv_;
public:
    OdbcConnectBackend(OdbcDriver *drv);
    void open(SqlDialect *dialect, const String &dsn,
            const String &user, const String &passwd);
    void clear_statement();
    void close();
    void commit();
    void rollback();
    void exec_direct(const String &sql);
    RowPtr fetch_row();
    void prepare(const String &sql);
    void exec(const Values &params);
};

class OdbcDriver: public SqlDriver
{
    friend class OdbcConnectBackend;
    Mutex conn_mux_;
public:
    OdbcDriver();
    std::auto_ptr<SqlConnectBackend> create_backend();
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__ODBC_DRIVER__INCLUDED
