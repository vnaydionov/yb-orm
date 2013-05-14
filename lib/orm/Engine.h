// -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#ifndef YB__ORM__ENGINE__INCLUDED
#define YB__ORM__ENGINE__INCLUDED

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <stdexcept>
#include <util/nlogger.h>
#include <util/Value.h>
#include <orm/Expression.h>
#include <orm/SqlDriver.h>
#include <orm/SqlPool.h>
#include <orm/MetaData.h>

class TestEngine;

namespace Yb {

class EngineBase
{
public:
    enum Mode { READ_ONLY = 0, READ_WRITE = 1, MANUAL = 1 }; 

    virtual ~EngineBase();
    virtual SqlConnection *get_conn() = 0;
    virtual SqlDialect *get_dialect() = 0;
    virtual ILogger *logger() = 0;
    virtual int get_mode() = 0;

    SqlResultSet select_iter(const Expression &select_expr);
    RowsPtr select(
        const Expression &what,
        const Expression &from,
        const Expression &where = Expression(),
        const Expression &group_by = Expression(),
        const Expression &having = Expression(),
        const Expression &order_by = Expression(),
        int max_rows = -1,
        bool for_update = false);
    const std::vector<LongInt> insert(const Table &table,
            const RowsData &rows, bool collect_new_ids);
    void update(const Table &table, const RowsData &rows);
    void delete_from(const Table &table, const Keys &keys);
    void exec_proc(const String &proc_code);
    RowPtr select_row(const Expression &what,
            const Expression &from, const Expression &where);
    const Value select1(const Expression &what,
            const Expression &from, const Expression &where);
    LongInt get_curr_value(const String &seq_name);
    LongInt get_next_value(const String &seq_name);
    void commit();
    void rollback();
    void touch();
    bool activity() { return get_conn()->activity(); }
    void create_schema(const Schema &schema, bool ignore_errors = false);
    void drop_schema(const Schema &schema, bool ignore_errors = false);

    static void gen_sql_insert(String &sql, TypeCodes &type_codes,
            ParamNums &param_nums, const Table &table,
            bool include_pk, bool numbered_params = false);
    static void gen_sql_update(String &sql, TypeCodes &type_codes,
            ParamNums &param_nums, const Table &table, 
            bool numbered_params = false);
    static void gen_sql_delete(String &sql, TypeCodes &type_codes,
            const Table &table, bool numbered_params = false);
};

class EngineSource
{
public:
    virtual ~EngineSource();
    virtual std::auto_ptr<EngineBase> clone() = 0;
};

class EngineCloned: public EngineBase
{
    int mode_;
    SqlConnection *conn_;
    SqlDialect *dialect_;
    ILogger *logger_;
    SqlPool *pool_;
public:
    EngineCloned(int mode, SqlConnection *conn,
            SqlDialect *dialect, ILogger *logger,
            SqlPool *pool = NULL)
        : mode_(mode)
        , conn_(conn)
        , dialect_(dialect)
        , logger_(logger)
        , pool_(pool)
    {}
    ~EngineCloned();
    int get_mode();
    SqlConnection *get_conn();
    SqlDialect *get_dialect();
    ILogger *logger();
};

const String env_cfg(const String &entry,
        const String &def_val = _T(""));

class Engine
    : public EngineBase, public EngineSource, private NonCopyable
{
public:
    friend class ::TestEngine;

    static SqlSource sql_source_from_env(const String &id = _T(""));
    Engine(int mode = READ_WRITE);
    Engine(int mode, std::auto_ptr<SqlConnection> conn);
    Engine(int mode, std::auto_ptr<SqlPool> pool,
           const String &source_id, int timeout = YB_POOL_WAIT_TIME);
    ~Engine();

    int get_mode();
    SqlConnection *get_conn();
    SqlDialect *get_dialect();
    ILogger *logger();
    std::auto_ptr<EngineBase> clone();
    void set_echo(bool echo) {
        echo_ = echo;
        if (conn_.get())
            conn_->set_echo(echo_);
        if (conn_ptr_)
            conn_ptr_->set_echo(echo_);
    }
    void set_logger(ILogger::Ptr logger) {
        logger_ = logger;
        if (conn_.get())
            conn_->init_logger(logger_.get());
        if (conn_ptr_)
            conn_ptr_->init_logger(logger_.get());
    }
private:
    SqlConnection *get_from_pool();

    bool echo_;
    int mode_;
    ILogger::Ptr logger_;
    std::auto_ptr<SqlPool> pool_;
    String source_id_;
    int timeout_;
    std::auto_ptr<SqlConnection> conn_;
    SqlDialect *dialect_;
    SqlConnection *conn_ptr_;
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__ENGINE__INCLUDED
