#include <set>
#include <fstream>
#include <util/str_utils.hpp>
#include <util/xmlnode.h>
#include <dbpool/mypp.h>
#include <dbpool/db_pool.h>
#include <orm/DBPoolDriver.h>

using namespace std;
using Yb::StrUtils::str_to_upper;

namespace Yb {

DBPoolConfig::DBPoolConfig(const String &xml_file_name)
    : pool_size_(DBPOOL_DEFAULT_SIZE)
{
    if (!str_empty(xml_file_name))
        load_from_xml_file(xml_file_name);
}

void
DBPoolConfig::register_data_source(const String &ds_name,
        const DBPoolDataSource &ds)
{
    ds_map_[ds_name] = ds;
}

static void parse_dbpool_config(ElementTree::ElementPtr root,
        DBPoolConfig::Map &ds_map, int &pool_size)
{
    set<String> drivers_to_load;
    DBPoolConfig::Map empty_map;
    empty_map.swap(ds_map);
    pool_size = DBPOOL_DEFAULT_SIZE;
    ElementTree::ElementsPtr backends = root->find_all(_T("DbBackend"));
    ElementTree::Elements::const_iterator j = backends->begin();
    for (; j != backends->end(); ++j) {
        ElementTree::ElementPtr backend = *j;
        if (!backend->has_attr(_T("id")))
            throw DBPoolConfigParseError(_T("DbBackend w/o @id"));
        String key = backend->get_attr(_T("id"));
        DBPoolDataSource ds;
        ds.driver = backend->find_first(_T("Driver"))->get_text();
        drivers_to_load.insert(ds.driver);
        ds.db = backend->find_first(_T("Name"))->get_text();
        ds.user = backend->find_first(_T("User"))->get_text();
        ds.pass = backend->find_first(_T("Pass"))->get_text();
        try {
            ds.host = backend->find_first(_T("Host"))->get_text();
        }
        catch (const ElementTree::ElementNotFound &) {}
        try {
            from_string(backend->find_first(_T("Port"))->get_text(), ds.port);
        }
        catch (const ElementTree::ElementNotFound &) {}
        try {
            from_string(backend->find_first(_T("Timeout"))->get_text(), ds.timeout);
        }
        catch (const ElementTree::ElementNotFound &) {}
        ds_map[key] = ds;
    }
    try {
        from_string(root->find_first(_T("DbPoolSize"))->get_text(), pool_size);
    }
    catch (const ElementTree::ElementNotFound &) {}
    // preload drivers
    set<String> ::const_iterator i = drivers_to_load.begin(),
        iend = drivers_to_load.end();
    for (; i != iend; ++i)
        mypp::Driver::load_driver(NARROW(*i));
}

void
DBPoolConfig::parse_from_xml_string(const std::string &xml_data)
{
    ElementTree::ElementPtr root = ElementTree::parse(xml_data);
    parse_dbpool_config(root, ds_map_, pool_size_);
}

void
DBPoolConfig::load_from_xml_file(const String &xml_file_name)
{
    std::ifstream f(NARROW(xml_file_name).c_str());
    if (!f.good())
        throw DBPoolConfigParseError(_T("file open error: ") + xml_file_name);
    ElementTree::ElementPtr root = ElementTree::parse(f);
    parse_dbpool_config(root, ds_map_, pool_size_);
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
        Char c = sql[i];
        switch (st) {
        case NORMAL:
            switch (char_code(c)) {
            case '-': st = MINUS_FOUND; break;
            case '/': st = SLASH_FOUND; break;
            case '"': st = IN_DQUOT; break;
            case '\'': st = IN_QUOT; break;
            case '?': pos_list.push_back(i); break;
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
        parts.push_back(str_substr(sql, prev_pos + 1, cur_pos - prev_pos - 1));
        prev_pos = cur_pos;
    }
    parts.push_back(str_substr(sql, prev_pos + 1, sql.size() - prev_pos - 1));
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

class DBPoolConnectionBackend;

class DBPoolCursorBackend: public SqlCursorBackend
{
    DBPoolConnectionBackend *conn_;
    String saved_sql_;
    vector<String> sql_parts_;
public:
    DBPoolCursorBackend(DBPoolConnectionBackend *conn)
        : conn_(conn)
    {}

    void exec_direct(const String &sql);
    void prepare(const String &sql);
    void exec(const Values &params);
    RowPtr fetch_row();
};

class DBPoolConnectionBackend: public SqlConnectionBackend
{
    friend class DBPoolCursorBackend;
    const DBPoolConfig &conf_;
    mypp::DBPool &pool_;
    SqlDialect *dialect_;
    mypp::Handle *handle_;
    auto_ptr<mypp::Statement> stmt_;
public:
    DBPoolConnectionBackend(const DBPoolConfig &conf, mypp::DBPool &pool)
        : conf_(conf)
        , pool_(pool)
        , dialect_(NULL)
        , handle_(NULL)
    {}

    ~DBPoolConnectionBackend()
    {
        close();
    }

    void open(SqlDialect *dialect, const SqlSource &source)
    {
        close();
        DBPoolDataSource ds(conf_.get_data_source(source.db()));
        mypp::DSN mypp_dsn(NARROW(ds.driver), NARROW(ds.host), ds.port,
                NARROW(ds.user), NARROW(ds.pass), NARROW(ds.db));
        handle_ = pool_.get(mypp_dsn, ds.timeout);
        if (!handle_)
            throw DBError(_T("Can't get connection from pool"));
        handle_->Clear();
        dialect_ = dialect;
    }

    auto_ptr<SqlCursorBackend> new_cursor()
    {
        auto_ptr<SqlCursorBackend> p(
                (SqlCursorBackend *)new DBPoolCursorBackend(this));
        return p;
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
        DBPoolCursorBackend cur(this);
        cur.exec_direct(_T("COMMIT"));
    }

    void rollback()
    {
        DBPoolCursorBackend cur(this);
        cur.exec_direct(_T("ROLLBACK"));
    }
};

void DBPoolCursorBackend::exec_direct(const String &sql)
{
    try {
        saved_sql_ = sql;
        conn_->stmt_.reset(NULL);
        conn_->stmt_.reset(new mypp::Statement(conn_->handle_, NARROW(sql)));
        conn_->stmt_->Execute();
    }
    catch (const mypp::Exception &e) {
        throw GenericDBError(WIDEN(e.error()));
    }
}

void DBPoolCursorBackend::prepare(const String &sql)
{
    saved_sql_ = sql;
    vector<int> pos_list;
    find_subst_signs(sql, pos_list);
    vector<String> empty;
    sql_parts_.swap(empty);
    split_by_subst_sign(sql, pos_list, sql_parts_);
}

void DBPoolCursorBackend::exec(const Values &params)
{
    exec_direct(format_sql(sql_parts_, params, conn_->dialect_));
}

RowPtr DBPoolCursorBackend::fetch_row()
{
    if (!conn_->stmt_.get())
        throw GenericDBError(
                _T("fetch_rows(): no statement. context: ") + saved_sql_);
    RowPtr orm_row;
    try {
    	mypp::Row row;
        if (!conn_->stmt_->Fetch(row))
            return RowPtr();
        orm_row.reset(new Row);
        for (int i = 0; i < row.size(); ++i) {
            const String name = str_to_upper(WIDEN(row[i].getName()));
            if (row[i].isNull())
                orm_row->push_back(RowItem(name, Value()));
            else
                orm_row->push_back(RowItem(name, Value(WIDEN(row[i].asString()))));
        }
    }
    catch (const mypp::Exception &e) {
        throw GenericDBError(WIDEN(e.error()));
    }
    return orm_row;
}

class DBPoolDriverImpl
{
    auto_ptr<DBPoolConfig> conf_;
    mypp::DBPool pool_;
public:
    DBPoolDriverImpl(auto_ptr<DBPoolConfig> conf)
        : conf_(conf)
        , pool_(conf_->get_pool_size())
    {}

    auto_ptr<SqlConnectionBackend> create_backend()
    {
        auto_ptr<SqlConnectionBackend> p(
                (SqlConnectionBackend *)new DBPoolConnectionBackend(
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

auto_ptr<SqlConnectionBackend>
DBPoolDriver::create_backend()
{
    return pimpl_->create_backend();
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
