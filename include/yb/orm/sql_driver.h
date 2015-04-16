// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__ORM__SQL_DRIVER__INCLUDED
#define YB__ORM__SQL_DRIVER__INCLUDED

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <iterator>
#include "util/utility.h"
#include "util/thread.h"
#include "util/result_set.h"
#include "util/item_registry.h"
#include "util/nlogger.h"
#include "util/value_type.h"
#include "orm_config.h"

namespace Yb {

class YBORM_DECL DBError: public RunTimeError
{
public:
    DBError(const String &msg);
};

class YBORM_DECL NoDataFound: public DBError
{
public:
    NoDataFound(const String &msg = _T(""));
};

class YBORM_DECL BadSQLOperation: public DBError
{
public:
    BadSQLOperation(const String &msg);
};

class YBORM_DECL BadOperationInMode: public DBError
{
public:
    BadOperationInMode(const String &msg);
};

class YBORM_DECL SqlDialectError: public DBError
{
public:
    SqlDialectError(const String &msg);
};

class YBORM_DECL SqlDriverError: public DBError
{
public:
    SqlDriverError(const String &msg);
};

class SqlCursor;
class SqlConnection;
class SqlPool;

struct ColumnInfo
{
    String name;
    String type;
    int size;
    bool notnull;
    String default_value;
    bool pk;
    String fk_table;
    String fk_table_key;
    ColumnInfo()
        : size(0)
        , notnull(false)
        , pk(false)
    {}
};

typedef std::vector<ColumnInfo> ColumnsInfo;

class YBORM_DECL SqlDialect: NonCopyable
{
    String name_, dual_;
    bool has_sequences_;
public:
    SqlDialect(const String &name, const String &dual,
            bool has_sequences)
        : name_(name)
        , dual_(dual)
        , has_sequences_(has_sequences)
    {}
    virtual ~SqlDialect();
    virtual bool parse_url_tail(const String &url_tail, StringDict &source);
    const String &get_name() { return name_; }
    const String &dual_name() { return dual_; }
    bool has_sequences() { return has_sequences_; }
    virtual bool native_driver_eats_slash();
    virtual const String select_curr_value(
            const String &seq_name) = 0;
    virtual const String select_next_value(
            const String &seq_name) = 0;
    virtual const String select_last_inserted_id(
            const String &table_name);
    virtual const String sql_value(const Value &x) = 0;
    virtual bool fk_internal();
    virtual bool commit_ddl();
    virtual bool has_for_update();
    virtual const String type2sql(int t) = 0;
    virtual const String create_sequence(const String &seq_name) = 0;
    virtual const String drop_sequence(const String &seq_name) = 0;
    virtual const String suffix_create_table();
    virtual const String primary_key_flag();
    virtual const String autoinc_flag();
    virtual const String sysdate_func();
    virtual bool explicit_null();
    virtual const String not_null_default(const String &not_null_clause,
            const String &default_value);
    virtual int pager_model();
    virtual const String grant_insert_id_statement(const String &table_name, bool on);
    // schema introspection
    virtual bool table_exists(SqlConnection &conn, const String &table) = 0;
    virtual bool view_exists(SqlConnection &conn, const String &table) = 0;
    virtual Strings get_tables(SqlConnection &conn) = 0;
    virtual Strings get_views(SqlConnection &conn) = 0;
    virtual ColumnsInfo get_columns(SqlConnection &connection,
            const String &table) = 0;
};

YBORM_DECL SqlDialect *sql_dialect(const String &name);
YBORM_DECL bool register_sql_dialect(std::auto_ptr<SqlDialect> dialect);
YBORM_DECL const Strings list_sql_dialects();

typedef std::pair<String, Value> RowItem;
typedef std::vector<RowItem> Row;
typedef std::auto_ptr<Row> RowPtr;
typedef std::vector<Row> Rows;
typedef std::auto_ptr<Rows> RowsPtr;
typedef std::vector<int> TypeCodes;

class YBORM_DECL SqlCursorBackend: NonCopyable
{
public:
    SqlCursorBackend() {}
    virtual ~SqlCursorBackend();
    virtual void exec_direct(const String &sql) = 0;
    virtual void prepare(const String &sql) = 0;
    virtual void bind_params(const TypeCodes &types);
    virtual void exec(const Values &params) = 0;
    virtual RowPtr fetch_row() = 0;
};

class YBORM_DECL SqlSource: public StringDict
{
public:
    SqlSource();
    explicit SqlSource(const String &url);
    SqlSource(const String &id, const String &driver, const String &dialect,
            const String &db, const String &user = _T(""),
            const String &passwd = _T(""));
    const String &id() const { return get(_T("&id")); }
    const String &driver() const { return get(_T("&driver")); }
    const String &dialect() const { return get(_T("&dialect")); }
    const String &db() const { return get(_T("&db")); }
    const String &user() const { return get(_T("&user")); }
    const String &passwd() const { return get(_T("&passwd")); }
    const String &host() const { return get(_T("&host")); }
    int port() const { return get_as<int>(String(_T("&port")), 0); }
    const Strings options() const;
    const String format(bool hide_passwd = true) const;

    bool operator== (const SqlSource &obj) const { return id() == obj.id(); }
    bool operator< (const SqlSource &obj) const { return id() < obj.id(); }
};

class YBORM_DECL SqlConnectionBackend: NonCopyable
{
public:
    SqlConnectionBackend() {}
    virtual ~SqlConnectionBackend();
    virtual void open(SqlDialect *dialect, const SqlSource &source) = 0;
    virtual void use_raw(SqlDialect *dialect, void *raw_connection) = 0;
    virtual void *get_raw() = 0;
    virtual std::auto_ptr<SqlCursorBackend> new_cursor() = 0;
    virtual void close() = 0;
    virtual void begin_trans() = 0;
    virtual void commit() = 0;
    virtual void rollback() = 0;
};

class YBORM_DECL SqlDriver: NonCopyable
{
    String name_;
public:
    SqlDriver(const String &name)
        : name_(name)
    {}
    virtual ~SqlDriver();
    const String &get_name() { return name_; }
    virtual std::auto_ptr<SqlConnectionBackend> create_backend() = 0;
    virtual void parse_url_tail(const String &dialect_name,
            const String &url_tail, StringDict &source);
    virtual bool explicit_begin_trans_required();
    virtual bool numbered_params();
    static const String convert_to_numbered_params(
            const String &sql);
};

YBORM_DECL SqlDriver *sql_driver(const String &name);
YBORM_DECL bool register_sql_driver(std::auto_ptr<SqlDriver> driver);
YBORM_DECL const Strings list_sql_drivers();

class YBORM_DECL SqlResultSet: public ResultSetBase<Row>
{
    friend class SqlCursor;
    SqlCursor &cursor_;
    mutable std::auto_ptr<SqlCursor> owned_cursor_;
    bool fetch(Row &row);
    SqlResultSet(SqlCursor &cursor): cursor_(cursor) {}
public:
    SqlResultSet(const SqlResultSet &rs)
        : cursor_(rs.cursor_)
        , owned_cursor_(rs.owned_cursor_.release())
    {}
    void own(std::auto_ptr<SqlCursor> cursor);
};

class YBORM_DECL SqlCursor: NonCopyable
{
    friend class SqlConnection;
    SqlConnection &connection_;
    std::auto_ptr<SqlCursorBackend> backend_;
    bool echo_, conv_params_;
    ILogger *log_;
    void debug(const String &s) { if (log_) log_->debug(NARROW(s)); }
    SqlCursor(SqlConnection &connection);
public:
    void exec_direct(const String &sql);
    void prepare(const String &sql);
    void bind_params(const TypeCodes &types);
    SqlResultSet exec(const Values &params);
    RowPtr fetch_row();
    RowsPtr fetch_rows(int max_rows = -1); // -1 = all
};

class YBORM_DECL SqlConnection: NonCopyable
{
    friend class SqlPool;
    friend class SqlCursor;
    SqlSource source_;
    SqlDriver *driver_;
    SqlDialect *dialect_;
    std::auto_ptr<SqlConnectionBackend> backend_;
    std::auto_ptr<SqlCursor> cursor_;
    bool activity_, echo_, conv_params_, bad_, explicit_trans_started_;
    time_t free_since_;
    ILogger::Ptr log_;
    void debug(const String &s) { if (log_.get()) log_->debug(NARROW(s)); }
    void mark_bad(const std::exception &e);
public:
    SqlConnection(const String &driver_name,
            const String &dialect_name, const String &db,
            const String &user = _T(""), const String &passwd = _T(""));
    SqlConnection(const String &driver_name,
            const String &dialect_name, void *raw_connection);
    explicit SqlConnection(const SqlSource &source);
    explicit SqlConnection(const String &url);
    ~SqlConnection();
    const SqlSource &get_source() const { return source_; }
    SqlDriver *get_driver() const { return driver_; }
    SqlDialect *get_dialect() const { return dialect_; }
    const String &get_db() const { return source_.db(); }
    const String &get_user() const { return source_.user(); }
    void set_echo(bool echo) { echo_ = echo; }
    void set_convert_params(bool conv_params) { conv_params_ = conv_params; }
    void init_logger(ILogger *parent) {
        log_.reset(NULL);
        if (parent)
            log_.reset(parent->new_logger("sql").release());
    }
    std::auto_ptr<SqlCursor> new_cursor();
    bool bad() const { return bad_; }
    bool activity() const { return activity_; }
    bool explicit_trans_started() const { return explicit_trans_started_; }
    bool explicit_transaction_control() const;
    void begin_trans_if_necessary();
    void begin_trans();
    void commit();
    void rollback();
    void clear();
    void exec_direct(const String &sql);
    void prepare(const String &sql);
    SqlResultSet exec(const Values &params);
    RowPtr fetch_row();
    RowsPtr fetch_rows(int max_rows = -1); // -1 = all
    void grant_insert_id(const String &table_name, bool on, bool ignore_errors = false);
    // schema introspection
    bool table_exists(const String &table);
    bool view_exists(const String &table);
    Strings get_tables();
    Strings get_views();
    ColumnsInfo get_columns(const String &table);
};

YBORM_DECL bool find_subst_signs(const String &sql,
        std::vector<int> &pos_list, String &first_word);
YBORM_DECL void split_by_subst_sign(const String &sql,
        const std::vector<int> &pos_list, std::vector<String> &parts);

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__SQL_DRIVER__INCLUDED
