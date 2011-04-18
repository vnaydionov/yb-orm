#ifndef YB__ORM__DBPOOL_DRIVER__INCLUDED
#define YB__ORM__DBPOOL_DRIVER__INCLUDED

#include <memory>
#include <string>
#include <map>
#include "SqlDriver.h"

namespace Yb {

class InvalidDBPoolDataSource: public DBError
{
public:
    InvalidDBPoolDataSource(const std::string &ds_name):
        DBError("Invalid DBPool data source: " + ds_name)
    {}
};

class DBPoolConfigParseError: public DBError
{
public:
    DBPoolConfigParseError(const std::string &details):
        DBError("DBPool config parse error: " + details)
    {}
};

const int DBPOOL_DEFAULT_TIMEOUT = 60;
const int DBPOOL_DEFAULT_SIZE = 10;

struct DBPoolDataSource
{
    std::string driver, host, user, pass, db;
    int port, timeout;
    DBPoolDataSource()
        : port(0)
        , timeout(DBPOOL_DEFAULT_TIMEOUT)
    {}
};

class DBPoolConfig
{
public:
    typedef std::map<std::string, DBPoolDataSource> Map;
    DBPoolConfig(const std::string &xml_file_name = "");
    void register_data_source(const std::string &ds_name,
            const DBPoolDataSource &ds);
    void parse_from_xml_string(const std::string &xml_data);
    void load_from_xml_file(const std::string &xml_file_name);
    void set_pool_size(int pool_size) { pool_size_ = pool_size; }
    const DBPoolDataSource &get_data_source(
            const std::string &ds_name) const;
    const Names list_data_sources() const;
    int get_pool_size() const { return pool_size_; }
private:
    Map ds_map_;
    int pool_size_;
};

class DBPoolDriverImpl;

class DBPoolDriver: public SqlDriver
{
    DBPoolDriverImpl *pimpl_;
public:
    DBPoolDriver(std::auto_ptr<DBPoolConfig> conf,
            const std::string &name = "DBPOOL3");
    ~DBPoolDriver();
    std::auto_ptr<SqlConnectBackend> create_backend();
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__DBPOOL_DRIVER__INCLUDED
