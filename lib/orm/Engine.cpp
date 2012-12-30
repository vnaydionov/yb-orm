#include <sstream>
#include <algorithm>
#include <util/str_utils.hpp>
#include <orm/Engine.h>

using namespace std;
using namespace Yb::StrUtils;

namespace Yb {

const String env_cfg(const String &entry, const String &def_val)
{
    String value = xgetenv(_T("YBORM_") + entry);
    if (str_empty(value))
        return def_val;
    return value;
}

EngineBase::~EngineBase()
{}

Engine::Engine(Mode work_mode, Engine *master, bool echo, ILogger *logger,
               SqlConnection *conn)
    : master_ptr_(master)
    , echo_(echo)
    , mode_(work_mode)
    , timeout_(0)
    , logger_ptr_(logger)
    , pool_ptr_(NULL)
    , conn_ptr_(conn)
    , dialect_(conn->get_dialect())
{
    conn_ptr_->set_echo(echo_);
    conn_ptr_->init_logger(logger_ptr_);
}

Engine::Engine(Mode work_mode, Engine *master, bool echo, ILogger *logger,
               SqlPool *pool, const String &source_id, int timeout)
    : master_ptr_(master)
    , echo_(echo)
    , mode_(work_mode)
    , source_id_(source_id)
    , timeout_(timeout)
    , logger_ptr_(logger)
    , pool_ptr_(pool)
    , conn_ptr_(pool->get(source_id, timeout))
    , dialect_(NULL)
{
    if (!conn_ptr_)
        throw GenericDBError(_T("Can't get connection"));
    dialect_ = conn_ptr_->get_dialect();
    conn_ptr_->set_echo(echo_);
    conn_ptr_->init_logger(logger_ptr_);
}

SqlSource Engine::sql_source_from_env(const String &id)
{
    SqlSource src;
    String url = env_cfg(_T("URL"));
    if (!str_empty(url))
        src = SqlSource(url);
    else
        src = SqlSource(_T(""),
            env_cfg(_T("DRIVER"), _T("DEFAULT")),
            env_cfg(_T("DBTYPE")), env_cfg(_T("DB")),
            env_cfg(_T("USER")), env_cfg(_T("PASSWD")));
    if (str_empty(id))
        src[_T("&id")] = src.format();
    else
        src[_T("&id")] = id;
    return src;
}

Engine::Engine(Mode work_mode)
    : master_ptr_(NULL)
    , echo_(false)
    , mode_(work_mode)
    , timeout_(0)
    , conn_(new SqlConnection(sql_source_from_env()))
    , logger_ptr_(NULL)
    , pool_ptr_(NULL)
    , conn_ptr_(NULL)
    , dialect_(conn_->get_dialect())
{}

Engine::Engine(Mode work_mode, auto_ptr<SqlConnection> conn)
    : master_ptr_(NULL)
    , echo_(false)
    , mode_(work_mode)
    , timeout_(0)
    , conn_(conn)
    , logger_ptr_(NULL)
    , pool_ptr_(NULL)
    , conn_ptr_(NULL)
    , dialect_(conn_->get_dialect())
{}

Engine::Engine(Mode work_mode, auto_ptr<SqlPool> pool,
               const String &source_id, int timeout)
    : master_ptr_(NULL)
    , echo_(false)
    , mode_(work_mode)
    , pool_(pool)
    , source_id_(source_id)
    , timeout_(timeout)
    , logger_ptr_(NULL)
    , pool_ptr_(NULL)
    , conn_ptr_(NULL)
    , dialect_(NULL)
{}

Engine::Engine(Mode work_mode, SqlDialect *dialect)
    : master_ptr_(NULL)
    , echo_(false)
    , mode_(work_mode)
    , timeout_(0)
    , logger_ptr_(NULL)
    , pool_ptr_(NULL)
    , conn_ptr_(NULL)
    , dialect_(dialect)
{}

Engine::~Engine()
{
    if (conn_ptr_) {
        SqlPool *pool = get_pool();
        if (pool)
            pool->put(conn_ptr_);
    }
}

std::auto_ptr<EngineBase>
Engine::clone()
{
    if (master_ptr_)
        throw GenericDBError(_T("Trying to clone a non master Engine"));
    if (pool_.get())
        return std::auto_ptr<EngineBase>(new Engine(mode_, this, echo_, logger_.get(),
            pool_.get(), source_id_, timeout_));
    return std::auto_ptr<EngineBase>(new Engine(mode_, this, echo_, logger_.get(),
        conn_.get()));
}

void
Engine::touch()
{
    if (dialect_->explicit_begin_trans() && !get_conn()->explicit_trans())
        get_conn()->begin_trans();
}

SqlPool *
Engine::get_pool()
{
    if (pool_ptr_)
        return pool_ptr_;
    if (pool_.get())
        return pool_.get();
    return NULL;
}

SqlConnection *
Engine::get_conn(bool strict)
{
    if (conn_ptr_)
        return conn_ptr_;
    if (conn_.get())
        return conn_.get();
    if (!strict)
        return NULL;
    SqlPool *pool = get_pool();
    if (!pool)
        throw GenericDBError(_T("Engine with no connection"));
    conn_ptr_ = pool->get(source_id_, timeout_);
    if (!conn_ptr_)
        throw GenericDBError(_T("Can't get connection"));
    dialect_ = conn_ptr_->get_dialect();
    return conn_ptr_;
}

void
Engine::set_echo(bool echo)
{
    echo_ = echo;
    SqlConnection *conn = get_conn(false);
    if (conn)
        conn->set_echo(echo_);
}

void
Engine::set_logger(ILogger::Ptr logger)
{
    logger_ = logger;
    SqlConnection *conn = get_conn(false);
    if (conn)
        conn->init_logger(logger_.get());
}

SqlResultSet
Engine::select_iter(const Expression &select_expr)
{
    Values params;
    String sql = select_expr.generate_sql(&params);
    auto_ptr<SqlCursor> cursor = get_conn()->new_cursor();
    cursor->prepare(sql);
    SqlResultSet rs = cursor->exec(params);
    rs.own(cursor);
    return rs;
}

RowsPtr
Engine::select(const Expression &what,
        const Expression &from, const Expression &where,
        const Expression &group_by, const Expression &having,
        const Expression &order_by, int max_rows, bool for_update)
{
    SqlResultSet rs = select_iter(SelectExpr(what).from_(from).
            where_(where).group_by_(group_by).having_(having).
            order_by_(order_by).for_update(for_update));
    RowsPtr rows(new Rows);
    copy_no_more_than_n(rs.begin(), rs.end(), max_rows,
            back_inserter(*rows));
    return rows;
}

const vector<LongInt>
Engine::insert(const Table &table, const RowsData &rows,
        bool collect_new_ids)
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode(
                _T("Using INSERT operation in read-only mode"));
    touch();
    return on_insert(table, rows, collect_new_ids);
}

void
Engine::update(const Table &table, const RowsData &rows)
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode(
                _T("Using UPDATE operation in read-only mode"));
    touch();
    on_update(table, rows);
}

void
Engine::delete_from(const Table &table, const Keys &keys)
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode(
                _T("Using DELETE operation in read-only mode"));
    touch();
    on_delete(table, keys);
}

void
Engine::exec_proc(const String &proc_code)
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode(
                _T("Trying to invoke a PROCEDURE in read-only mode"));
    touch();
    on_exec_proc(proc_code);
}

void
Engine::commit()
{
    on_commit();
}

void
Engine::rollback()
{
    on_rollback();
}

RowPtr
Engine::select_row(const Expression &what,
        const Expression &from, const Expression &where)
{
    RowsPtr rows = select(what, from, where);
    if (rows->size() != 1)
        throw NoDataFound(_T("Unable to fetch exactly one row!"));
    RowPtr row(new Row((*rows)[0]));
    return row;
}

const Value
Engine::select1(const Expression &what,
        const Expression &from, const Expression &where)
{
    RowPtr row = select_row(what, from, where);
    if (row->size() != 1)
        throw BadSQLOperation(_T("Unable to fetch exactly one column!"));
    return row->begin()->second;
}

LongInt
Engine::get_curr_value(const String &seq_name)
{
    return select1(Expression(dialect_->select_curr_value(seq_name)),
            Expression(dialect_->dual_name()), Expression()).as_longint();
}

LongInt
Engine::get_next_value(const String &seq_name)
{
    return select1(Expression(dialect_->select_next_value(seq_name)),
            Expression(dialect_->dual_name()), Expression()).as_longint();
}

const vector<LongInt>
Engine::on_insert(const Table &table, const RowsData &rows,
        bool collect_new_ids)
{
    vector<LongInt> ids;
    if (!rows.size())
        return ids;
    String sql;
    Values params;
    ParamNums param_nums;
    do_gen_sql_insert(sql, params, param_nums, table,
            !collect_new_ids);
    if (!collect_new_ids) {
        auto_ptr<SqlCursor> cursor = get_conn()->new_cursor();
        cursor->prepare(sql);
        RowsData::const_iterator r = rows.begin(), rend = rows.end();
        for (; r != rend; ++r) {
            ParamNums::const_iterator f = param_nums.begin(),
                fend = param_nums.end();
            for (; f != fend; ++f)
                params[f->second] = (*r)[table.idx_by_name(f->first)];
            cursor->exec(params);
        }
    }
    else {
        RowsData::const_iterator r = rows.begin(), rend = rows.end();
        for (; r != rend; ++r) {
            auto_ptr<SqlCursor> cursor = get_conn()->new_cursor();
            cursor->prepare(sql);
            ParamNums::const_iterator f = param_nums.begin(),
                fend = param_nums.end();
            for (; f != fend; ++f)
                params[f->second] = (*r)[table.idx_by_name(f->first)];
            cursor->exec(params);
            cursor.reset(NULL);
            auto_ptr<SqlCursor> cursor2 = get_conn()->new_cursor();
            cursor2->prepare(
                    dialect_->select_last_inserted_id(table.name()));
            cursor2->exec(Values());
            RowsPtr id_rows = cursor2->fetch_rows();
            ids.push_back((*id_rows)[0][0].second.as_longint());
        }
    }
    return ids;
}

void Engine::on_update(const Table &table, const RowsData &rows)
{
    if (!rows.size())
        return;
    String sql;
    Values params;
    ParamNums param_nums;
    do_gen_sql_update(sql, params, param_nums, table);
    auto_ptr<SqlCursor> cursor = get_conn()->new_cursor();
    cursor->prepare(sql);
    RowsData::const_iterator r = rows.begin(), rend = rows.end();
    for (; r != rend; ++r) {
        ParamNums::const_iterator f = param_nums.begin(),
            fend = param_nums.end();
        for (; f != fend; ++f)
            params[f->second] = (*r)[table.idx_by_name(f->first)];
        cursor->exec(params);
    }
}

void
Engine::on_delete(const Table &table, const Keys &keys)
{
    if (!keys.size())
        return;
    String sql;
    Values params;
    do_gen_sql_delete(sql, params, table);
    auto_ptr<SqlCursor> cursor = get_conn()->new_cursor();
    cursor->prepare(sql);
    Keys::const_iterator k = keys.begin(), kend = keys.end();
    for (; k != kend; ++k) {
        for (size_t i = 0; i < k->second.size(); ++i)
            params[i] = k->second[i];
        cursor->exec(params);
    }
}

void
Engine::on_exec_proc(const String &proc_code)
{
    auto_ptr<SqlCursor> cursor = get_conn()->new_cursor();
    cursor->exec_direct(proc_code);
}

void
Engine::on_commit()
{
    get_conn()->commit();
}

void
Engine::on_rollback()
{
    get_conn()->rollback();
}

void
Engine::do_gen_sql_insert(String &sql, Values &params_out,
        ParamNums &param_nums_out, const Table &table,
        bool include_pk) const
{
    String sql_query;
    sql_query = _T("INSERT INTO ") + table.name() + _T(" (");
    Values params;
    ParamNums param_nums;
    Strings names, pholders;
    size_t i;
    for (i = 0; i < table.size(); ++i) {
        const Column &col = table[i];
        if ((!col.is_ro() || col.is_pk()) &&
                (!col.is_pk() || include_pk))
        {
            param_nums[col.name()] = params.size();
            params.push_back(Value(-1));
            names.push_back(col.name());
            pholders.push_back(_T("?"));
        }
    }
    sql_query += ExpressionList(names).get_sql() + _T(") VALUES (") + 
        ExpressionList(pholders).get_sql() + _T(")");
    sql = sql_query;
    params_out.swap(params);
    param_nums_out.swap(param_nums);
}

void
Engine::do_gen_sql_update(String &sql, Values &params_out,
        ParamNums &param_nums_out, const Table &table) const
{
    if (!table.pk_fields().size())
        throw BadSQLOperation(_T("cannot build update statement: no key in table"));
    String sql_query = _T("UPDATE ") + table.name() + _T(" SET ");
    Values params;
    params.reserve(table.size());
    ParamNums param_nums;
    size_t i;
    for (i = 0; i < table.size(); ++i) {
        const Column &col = table[i];
        if (!col.is_pk() && !col.is_ro()) {
            if (!params.empty())
                sql_query += _T(", ");
            sql_query += col.name() + _T(" = ?");
            param_nums[col.name()] = params.size();
            params.push_back(Value());
        }
    }
    for (i = 0; i < table.pk_fields().size(); ++i) {
        const String &col_name = table.pk_fields()[i];
        param_nums[col_name] = params.size();
        params.push_back(Value());
    }
    Key key;
    Values fake_params(table.size(), Value(-1));
    table.mk_key(fake_params, key);
    sql_query += _T(" WHERE ") + KeyFilter(key).generate_sql(&fake_params);
    sql = sql_query;
    params_out.swap(params);
    param_nums_out.swap(param_nums);
}

void
Engine::do_gen_sql_delete(String &sql, Values &params, const Table &table) const
{
    if (!table.pk_fields().size())
        throw BadSQLOperation(_T("cannot build update statement: no key in table"));
    String sql_query = _T("DELETE FROM ") + table.name();
    Key key;
    Values fake_params(table.size(), Value(-1));
    table.mk_key(fake_params, key);
    sql_query += _T(" WHERE ") + KeyFilter(key).generate_sql(&params);
    sql = sql_query;
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
