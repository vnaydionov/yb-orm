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
Engine::select_iter(const Expression &what,
        const Expression &from, const Expression &where,
        const Expression &group_by, const Expression &having,
        const Expression &order_by, int max_rows, bool for_update)
{
    bool select_mode = (mode_ == FORCE_SELECT_UPDATE) ? true : for_update;
    if ((mode_ == READ_ONLY) && select_mode)
        throw BadOperationInMode(
                _T("Using SELECT FOR UPDATE in read-only mode"));
    String sql;
    Values params;
    do_gen_sql_select(sql, params,
            what, from, where, group_by, having, order_by, for_update);
    if (select_mode)
        touch();
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
    bool select_mode = (mode_ == FORCE_SELECT_UPDATE) ? true : for_update;
    if ((mode_ == READ_ONLY) && select_mode)
        throw BadOperationInMode(
                _T("Using SELECT FOR UPDATE in read-only mode"));
    if (select_mode)
        touch();
    RowsPtr rows = on_select(what, from, where,
                group_by, having, order_by, max_rows, select_mode);
    return rows;
}

const vector<LongInt>
Engine::insert(const String &table_name,
        const Rows &rows, const StringSet &exclude_fields,
        bool collect_new_ids)
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode(
                _T("Using INSERT operation in read-only mode"));
    touch();
    return on_insert(table_name, rows, exclude_fields, collect_new_ids);
}

void
Engine::update(const String &table_name,
        const Rows &rows, const Strings &key_fields,
        const StringSet &exclude_fields, const Filter &where)
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode(
                _T("Using UPDATE operation in read-only mode"));
    touch();
    on_update(table_name, rows, key_fields, exclude_fields, where);
}

void
Engine::delete_from(const String &table_name, const Filter &where)
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode(
                _T("Using DELETE operation in read-only mode"));
    touch();
    on_delete(table_name, where);
}

void
Engine::delete_from(const String &table_name, const Keys &keys)
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode(
                _T("Using DELETE operation in read-only mode"));
    touch();
    on_delete(table_name, keys);
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

RowsPtr
Engine::on_select(const Expression &what,
        const Expression &from, const Expression &where,
        const Expression &group_by, const Expression &having,
        const Expression &order_by, int max_rows,
        bool for_update)
{
    String sql;
    Values params;
    do_gen_sql_select(sql, params,
            what, from, where, group_by, having, order_by, for_update);
    auto_ptr<SqlCursor> cursor = get_conn()->new_cursor();
    cursor->prepare(sql);
    cursor->exec(params);
    return cursor->fetch_rows(max_rows);
}

const vector<LongInt>
Engine::on_insert(const String &table_name,
        const Rows &rows, const StringSet &exclude_fields,
        bool collect_new_ids)
{
    vector<LongInt> ids;
    if (!rows.size())
        return ids;
    String sql;
    Values params;
    ParamNums param_nums;
    do_gen_sql_insert(sql, params, param_nums, table_name,
            rows[0], exclude_fields);
    if (!collect_new_ids) {
        auto_ptr<SqlCursor> cursor = get_conn()->new_cursor();
        cursor->prepare(sql);
        Rows::const_iterator it = rows.begin(), end = rows.end();
        for (; it != end; ++it) {
            Row::const_iterator f = it->begin(), fend = it->end();
            for (; f != fend; ++f) {
                StringSet::const_iterator x = exclude_fields.find(f->first);
                if (x == exclude_fields.end())
                    params[param_nums[f->first]] = f->second;
            }
            cursor->exec(params);
        }
    }
    else {
        Rows::const_iterator it = rows.begin(), end = rows.end();
        for (; it != end; ++it) {
            {
                auto_ptr<SqlCursor> cursor = get_conn()->new_cursor();
                cursor->prepare(sql);
                Row::const_iterator f = it->begin(), fend = it->end();
                for (; f != fend; ++f) {
                    StringSet::const_iterator x = exclude_fields.find(f->first);
                    if (x == exclude_fields.end())
                        params[param_nums[f->first]] = f->second;
                }
                cursor->exec(params);
            }
            auto_ptr<SqlCursor> cursor = get_conn()->new_cursor();
            cursor->prepare(dialect_->select_last_inserted_id(table_name));
            cursor->exec(Values());
            RowsPtr id_rows = cursor->fetch_rows();
            ids.push_back((*id_rows)[0][0].second.as_longint());
        }
    }
    return ids;
}

void
Engine::on_update(const String &table_name,
        const Rows &rows, const Strings &key_fields,
        const StringSet &exclude_fields, const Filter &where)
{
    if (!rows.size())
        return;
    String sql;
    Values params;
    ParamNums param_nums;
    do_gen_sql_update(sql, params, param_nums, table_name,
            rows[0], key_fields, exclude_fields, where);
    auto_ptr<SqlCursor> cursor = get_conn()->new_cursor();
    cursor->prepare(sql);
    Rows::const_iterator it = rows.begin(), end = rows.end();
    for (; it != end; ++it) {
        Row::const_iterator f = it->begin(), fend = it->end();
        for (; f != fend; ++f) {
            StringSet::const_iterator x = exclude_fields.find(f->first);
            if (x == exclude_fields.end())
                params[param_nums[f->first]] = f->second;
        }
        cursor->exec(params);
    }
}

void
Engine::on_delete(const String &table_name, const Filter &where)
{
    String sql;
    Values params;
    do_gen_sql_delete(sql, params, table_name, where);
    auto_ptr<SqlCursor> cursor = get_conn()->new_cursor();
    cursor->prepare(sql);
    cursor->exec(params);
}

void
Engine::on_delete(const String &table_name, const Keys &keys)
{
    if (keys.size()) {
        String sql;
        Values params;
        do_gen_sql_delete(sql, params, table_name, KeyFilter(*keys.begin()));
        auto_ptr<SqlCursor> cursor = get_conn()->new_cursor();
        cursor->prepare(sql);
        Keys::const_iterator i = keys.begin(), iend = keys.end();
        for (; i != iend; ++i) {
            ValueMap::const_iterator j = i->second.begin(),
                jend = i->second.end();
            for (int pos = 0; j != jend; ++j, ++pos)
                params[pos] = j->second;
            cursor->exec(params);
        }
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
Engine::do_gen_sql_select(String &sql, Values &params,
        const Expression &what, const Expression &from, const Expression &where,
        const Expression &group_by, const Expression &having,
        const Expression &order_by, bool for_update) const
{
    SelectExpr q(what);
    q.from_(from).where_(where)
        .group_by_(group_by).having_(having).order_by_(order_by);
    sql = q.generate_sql(&params);
    if (for_update)
        sql += _T(" FOR UPDATE");
}

void
Engine::do_gen_sql_insert(String &sql, Values &params,
        ParamNums &param_nums, const String &table_name,
        const Row &row, const StringSet &exclude_fields) const
{
    if (row.empty())
        throw BadSQLOperation(_T("Can't do INSERT with empty row"));
    std::ostringstream sql_query;
    sql_query << "INSERT INTO " << NARROW(table_name) << " (";
    Strings names, pholders;
    Row::const_iterator it = row.begin(), end = row.end();
    for (; it != end; ++it) {
        StringSet::const_iterator x = exclude_fields.find(it->first);
        if (x == exclude_fields.end()) {
            param_nums[it->first] = params.size();
            params.push_back(it->second);
            names.push_back(it->first);
            pholders.push_back(_T("?"));
        }
    }
    sql_query << NARROW(ExpressionList(names).get_sql())
        << ") VALUES ("
        << NARROW(ExpressionList(pholders).get_sql())
        << ")";
    sql = WIDEN(sql_query.str());
}

void
Engine::do_gen_sql_update(String &sql, Values &params,
        ParamNums &param_nums, const String &table_name,
        const Row &row, const Strings &key_fields,
        const StringSet &exclude_fields, const Filter &where) const
{
    if (key_fields.empty() && (where.get_sql() == Filter().get_sql()))
        throw BadSQLOperation(
                _T("Can't do UPDATE without where clause and empty key_fields"));
    if (row.empty())
        throw BadSQLOperation(_T("Can't UPDATE with empty row set"));

    Row excluded_row;
    std::ostringstream sql_query;
    sql_query << "UPDATE " << NARROW(table_name) << " SET ";
    Row::const_iterator it = row.begin(), end = row.end();
    for (; it != end; ++it) {
        if (exclude_fields.find(it->first)
                == const_cast<StringSet &>(exclude_fields).end() &&
            std::find(key_fields.begin(), key_fields.end(), it->first) 
                == key_fields.end())
        {
            excluded_row.push_back(make_pair(it->first, it->second));
        }
    }

    Row::const_iterator last = excluded_row.end();
    if (!excluded_row.empty())
        --last;
    end = excluded_row.end();
    for (it = excluded_row.begin(); it != excluded_row.end(); ++it)
    {
        param_nums[it->first] = params.size();
        params.push_back(it->second);
        sql_query << NARROW(it->first) << " = ?";
        if (it != last)
            sql_query << ", ";
    }

    sql_query << " WHERE ";
    std::ostringstream where_sql;
    String s = where.generate_sql(&params);
    if (s != Filter().get_sql())
    {
        where_sql << "(" << NARROW(s) << ")";
        if (!key_fields.empty())
            where_sql << " AND ";
    }
    Strings::const_iterator it_where = key_fields.begin(), end_where = key_fields.end();
    Strings::const_iterator it_last = key_fields.end();
    if (!key_fields.empty())
        --it_last;
    for (; it_where != end_where; ++it_where) {
        Row::const_iterator it_find = find_in_row(row, *it_where);
        if (it_find != row.end()) {
            param_nums[it_find->first] = params.size();
            params.push_back(it_find->second);
            where_sql << "(" << NARROW(it_find->first) << " = ?)";
            if (it_where != it_last)
                where_sql << " AND ";
        }
    }
    if (str_empty(WIDEN(where_sql.str())))
        throw BadSQLOperation(_T("Can't do UPDATE with empty where clause"));
    sql_query << where_sql.str();
    sql = WIDEN(sql_query.str());
}

void
Engine::do_gen_sql_delete(String &sql, Values &params,
        const String &table, const Filter &where) const
{
    if (where.get_sql() == Filter().get_sql())
        throw BadSQLOperation(_T("Can't DELETE without where clause"));
    std::ostringstream sql_query;
    sql_query << "DELETE FROM " << NARROW(table);
    String s = where.generate_sql(&params);
    if (s != Filter().get_sql())
        sql_query << " WHERE " << NARROW(s);
    sql = WIDEN(sql_query.str());
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
