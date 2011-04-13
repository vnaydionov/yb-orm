#ifndef YB__ORM__ODBC_DRIVER__INCLUDED
#define YB__ORM__ODBC_DRIVER__INCLUDED

#include <memory>
#include <string>
#include "Value.h"
#include "EngineBase.h"

namespace Yb {

class OdbcDriverImpl;

class OdbcDriver
{
    OdbcDriverImpl *impl_;
public:
    OdbcDriver();
    ~OdbcDriver();
    void open(const std::string &db, const std::string &user, const std::string &passwd);
    void commit();
    void rollback();
    void exec_direct(const std::string &sql);
    RowsPtr fetch_rows(int max_rows = -1);
    void prepare(const std::string &sql);
    void exec(const Values &params);
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__ODBC_DRIVER__INCLUDED
