#ifndef YB__ORM__SOCI_DRIVER__INCLUDED
#define YB__ORM__SOCI_DRIVER__INCLUDED

#include <memory>
#include <vector>
#include <util/Thread.h>
#include <orm/SqlDriver.h>
#include <soci.h>

namespace Yb {

typedef soci::session SOCIDatabase;
typedef soci::statement SOCIQuery;

class SOCICursorBackend: public SqlCursorBackend
{
    SOCIDatabase *conn_;
    SOCIQuery *stmt_;
    int exec_count_;
    std::string sql_;
    bool is_select_;
    soci::row row_;
    std::vector<std::string> in_params_;
    std::vector<soci::indicator> in_flags_;
public:
    SOCICursorBackend(SOCIDatabase *conn);
    ~SOCICursorBackend();
    void close();
    void exec_direct(const String &sql);
    void prepare(const String &sql);
    void exec(const Values &params);
    RowPtr fetch_row();
};

class SOCIDriver;

class SOCIConnectionBackend: public SqlConnectionBackend
{
    SOCIDatabase *conn_;
    SOCIDriver *drv_;
public:
    SOCIConnectionBackend(SOCIDriver *drv);
    ~SOCIConnectionBackend();
    void open(SqlDialect *dialect, const SqlSource &source);
    std::auto_ptr<SqlCursorBackend> new_cursor();
    void close();
    void commit();
    void rollback();
};

class SOCIDriver: public SqlDriver
{
    friend class SOCIConnectionBackend;
    Mutex conn_mux_;
public:
    SOCIDriver();
    std::auto_ptr<SqlConnectionBackend> create_backend();
    void parse_url_tail(const String &dialect_name,
            const String &url_tail, StringDict &source);
};

} //namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__SOCI_DRIVER__INCLUDED
