#include <set>
#include <boost/lexical_cast.hpp>
#include <util/str_utils.hpp>
#include <util/xmlnode.h>
#include <dbpool/mypp.h>
#include <dbpool/db_pool.h>
#include "DBPoolDriver.h"

using namespace std;
using Yb::StrUtils::str_to_upper;

namespace Yb {

DBPoolConfig::DBPoolConfig(const String &xml_file_name)
    : pool_size_(DBPOOL_DEFAULT_SIZE)
{
    if (!xml_file_name.empty())
        load_from_xml_file(xml_file_name);
}

void
DBPoolConfig::register_data_source(const String &ds_name,
        const DBPoolDataSource &ds)
{
    ds_map_[ds_name] = ds;
}

static void parse_dbpool_config(xmlNodePtr node,
        DBPoolConfig::Map &ds_map, int &pool_size)
{
    set<String> drivers_to_load;
    DBPoolConfig::Map empty_map;
    empty_map.swap(ds_map);
    pool_size = DBPOOL_DEFAULT_SIZE;
    xmlNodePtr backend = Xml::Child(node, _T("DbBackend"));
    for (; backend; backend = Xml::Sibling(backend, _T("DbBackend")))
    {
        if (!Xml::HasAttr(backend, _T("id")))
            throw DBPoolConfigParseError(_T("DbBackend w/o @id"));
        String key = Xml::GetAttr(backend, _T("id"));
        DBPoolDataSource ds;
        xmlNodePtr field;
        field = Xml::Child(backend, _T("Driver"));
        if (!field)
            throw DBPoolConfigParseError(
                    _T("DbBackend ") + key + _T(": missing Driver field"));
        ds.driver = Xml::GetContent(field);
        drivers_to_load.insert(ds.driver);
        field = Xml::Child(backend, _T("Name"));
        if (!field)
            throw DBPoolConfigParseError(
                    _T("DbBackend ") + key + _T(": missing Db field"));
        ds.db = Xml::GetContent(field);
        field = Xml::Child(backend, _T("User"));
        if (!field)
            throw DBPoolConfigParseError(
                    _T("DbBackend ") + key + _T(": missing User field"));
        ds.user = Xml::GetContent(field);
        field = Xml::Child(backend, _T("Pass"));
        if (!field)
            throw DBPoolConfigParseError(
                    _T("DbBackend ") + key + _T(": missing Pass field"));
        ds.pass = Xml::GetContent(field);
        field = Xml::Child(backend, _T("Host"));
        if (field)
            ds.host = Xml::GetContent(field);
        field = Xml::Child(backend, _T("Port"));
        if (field)
            ds.port = boost::lexical_cast<int>(Xml::GetContent(field));
        field = Xml::Child(backend, _T("Timeout"));
        if (field)
            ds.timeout = boost::lexical_cast<int>(Xml::GetContent(field));
        ds_map[key] = ds;
    }
    xmlNodePtr item = Xml::Child(node, _T("DbPoolSize"));
    if (item)
        pool_size = boost::lexical_cast<int>(Xml::GetContent(item));
    // preload drivers
    set<String> ::const_iterator i = drivers_to_load.begin(),
        iend = drivers_to_load.end();
    for (; i != iend; ++i)
        mypp::Driver::load_driver(NARROW(*i));
}

void
DBPoolConfig::parse_from_xml_string(const std::string &xml_data)
{
    Xml::Node node(Xml::Parse(xml_data));
    parse_dbpool_config(node.get(), ds_map_, pool_size_);
}

void
DBPoolConfig::load_from_xml_file(const String &xml_file_name)
{
    Xml::Node node(Xml::ParseFile(xml_file_name));
    parse_dbpool_config(node.get(), ds_map_, pool_size_);
}

const DBPoolDataSource &
DBPoolConfig::get_data_source(
        const String &ds_name) const
{
    Map::const_iterator i = ds_map_.find(ds_name);
    if (ds_map_.end() == i)
        throw InvalidDBPoolDataSource(ds_name);
    return i->second;
}

const Strings
DBPoolConfig::list_data_sources() const
{
    Strings names;
    Map::const_iterator i = ds_map_.begin(), iend = ds_map_.end();
    for (; i != iend; ++i)
        names.push_back(i->first);
    return names;
}

bool find_subst_signs(const String &sql, vector<int> &pos_list)
{
    enum { NORMAL, MINUS_FOUND, LINE_COMMENT, SLASH_FOUND, COMMENT,
        COMMENT_ASTER_FOUND, IN_QUOT, IN_QUOT_QFOUND, IN_DQUOT } st;
    st = NORMAL;
    for (int i = 0; i < sql.size();) {
        char c = sql[i];
        switch (st) {
        case NORMAL:
            switch (c) {
            case '-': st = MINUS_FOUND; break;
            case '/': st = SLASH_FOUND; break;
            case '"': st = IN_DQUOT; break;
            case '\'': st = IN_QUOT; break;
            case '?': pos_list.push_back(i);
            }
            ++i;
            break;
        case MINUS_FOUND:
            if (c == '-') {
                st = LINE_COMMENT;
                ++i;
            }
            else
                st = NORMAL;
            break;
        case LINE_COMMENT:
            if (c == '\n')
                st = NORMAL;
            ++i;
            break;
        case SLASH_FOUND:
            if (c == '*') {
                st = COMMENT;
                ++i;
            }
            else
                st = NORMAL;
            break;
        case COMMENT:
            if (c == '*')
                st = COMMENT_ASTER_FOUND;
            ++i;
            break;
        case COMMENT_ASTER_FOUND:
            if (c == '/') {
                st = NORMAL;
                ++i;
            }
            else
                st = COMMENT;
            break;
        case IN_QUOT:
            if (c == '\'')
                st = IN_QUOT_QFOUND;
            ++i;
            break;
        case IN_QUOT_QFOUND:
            if (c == '\'') {
                st = IN_QUOT;
                ++i;
            }
            else
                st = NORMAL;
            break;
        case IN_DQUOT:
            if (c == '"')
                st = NORMAL;
            ++i;
            break;
        }
    }
    return st == NORMAL || st == IN_QUOT_QFOUND \
               || st == LINE_COMMENT || st == SLASH_FOUND;
}

void split_by_subst_sign(const String &sql,
        const vector<int> &pos_list, vector<String> &parts)
{
    int prev_pos = -1;
    for (int i = 0; i < pos_list.size(); ++i) {
        int cur_pos = pos_list[i];
        parts.push_back(sql.substr(prev_pos + 1, cur_pos - prev_pos - 1));
        prev_pos = cur_pos;
    }
    parts.push_back(sql.substr(prev_pos + 1, sql.size() - prev_pos - 1));
}

const String format_sql(const vector<String> &parts,
        const Values &values, SqlDialect *dialect)
{
    String sql;
    sql += parts[0];
    for (int i = 1; i < parts.size(); ++i) {
        sql += dialect->sql_value(values[i - 1]);
        sql += parts[i];
    }
    return sql;
}

class DBPoolConnectBackend: public SqlConnectBackend
{
    const DBPoolConfig &conf_;
    mypp::DBPool &pool_;
    SqlDialect *dialect_;
    mypp::Handle *handle_;
    auto_ptr<mypp::Statement> stmt_;
    String saved_sql_;
    vector<String> sql_parts_;
public:
    DBPoolConnectBackend(const DBPoolConfig &conf, mypp::DBPool &pool)
        : conf_(conf)
        , pool_(pool)
        , dialect_(NULL)
        , handle_(NULL)
    {}

    ~DBPoolConnectBackend()
    {
        close();
    }

    void open(SqlDialect *dialect, const String &dsn,
            const String & /* user */, const String & /* passwd */)
    {
        close();
        DBPoolDataSource ds(conf_.get_data_source(dsn));
        mypp::DSN mypp_dsn(NARROW(ds.driver), NARROW(ds.host), ds.port,
                NARROW(ds.user), NARROW(ds.pass), NARROW(ds.db));
        handle_ = pool_.get(mypp_dsn, ds.timeout);
        if (!handle_)
            throw DBError(_T("Can't get connection from pool"));
        handle_->Clear();
        dialect_ = dialect;
    }

    void close()
    {
        if (handle_) {
            pool_.put(handle_);
            handle_ = NULL;
            dialect_ = NULL;
        }
    }

    void commit()
    {
        exec_direct(_T("COMMIT"));
    }

    void rollback()
    {
        exec_direct(_T("ROLLBACK"));
    }

    void exec_direct(const String &sql)
    {
        try {
            saved_sql_ = sql;
            stmt_.reset(NULL);
            stmt_.reset(new mypp::Statement(handle_, NARROW(sql)));
            stmt_->Execute();
        }
        catch (const mypp::Exception &e) {
            throw GenericDBError(WIDEN(e.error()));
        }
    }

    RowPtr fetch_row()
    {
        if (!stmt_.get())
            throw GenericDBError(
                    _T("fetch_rows(): no statement. context: ") + saved_sql_);
        try {
            mypp::Row row;
            if (!stmt_->Fetch(row))
                return RowPtr();
            RowPtr orm_row(new Row);
            for (int i = 0; i < row.size(); ++i) {
                const String name = str_to_upper(WIDEN(row[i].getName()));
                if (row[i].isNull())
                    orm_row->push_back(RowItem(name, Value()));
                else
                    orm_row->push_back(RowItem(name, Value(WIDEN(row[i].asString()))));
            }
            return orm_row;
        }
        catch (const mypp::Exception &e) {
            throw GenericDBError(WIDEN(e.error()));
        }
    }

    void prepare(const String &sql)
    {
        saved_sql_ = sql;
        vector<int> pos_list;
        find_subst_signs(sql, pos_list);
        vector<String> empty;
        sql_parts_.swap(empty);
        split_by_subst_sign(sql, pos_list, sql_parts_);
    }

    void exec(const Values &params)
    {
        exec_direct(format_sql(sql_parts_, params, dialect_));
    }
};

class DBPoolDriverImpl
{
    auto_ptr<DBPoolConfig> conf_;
    mypp::DBPool pool_;
public:
    DBPoolDriverImpl(auto_ptr<DBPoolConfig> conf)
        : conf_(conf)
        , pool_(conf_->get_pool_size())
    {}

    auto_ptr<SqlConnectBackend> create_backend()
    {
        auto_ptr<SqlConnectBackend> p(
                (SqlConnectBackend *)new DBPoolConnectBackend(
                    *conf_, pool_));
        return p;
    }
};

DBPoolDriver::DBPoolDriver(auto_ptr<DBPoolConfig> conf,
        const String &name)
    : SqlDriver(name)
    , pimpl_(new DBPoolDriverImpl(conf))
{}

DBPoolDriver::~DBPoolDriver()
{
    delete pimpl_;
}

auto_ptr<SqlConnectBackend>
DBPoolDriver::create_backend()
{
    return pimpl_->create_backend();
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
