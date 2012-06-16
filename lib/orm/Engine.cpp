#include <sstream>
#include <algorithm>
#include <util/str_utils.hpp>
#include <orm/Engine.h>

using namespace std;
using namespace Yb::StrUtils;

namespace Yb {

static inline const String cfg(const String &entry)
{ return xgetenv(_T("YBORM_") + entry); }

EngineBase::~EngineBase()
{}

Engine::Engine(Mode work_mode, Engine *master, bool echo, ILogger *logger,
               SqlConnection *conn)
    : master_ptr_(master)
    , touched_(false)
    , echo_(echo)
    , mode_(work_mode)
    , timeout_(0)
    , logger_ptr_(logger)
    , pool_ptr_(NULL)
    , conn_ptr_(conn)
    , dialect_(conn->get_dialect())
{
    conn_ptr_->set_echo(echo_);
    conn_ptr_->set_logger(logger_ptr_);
}

Engine::Engine(Mode work_mode, Engine *master, bool echo, ILogger *logger,
               SqlPool *pool, const String &source_id, int timeout)
    : master_ptr_(master)
    , touched_(false)
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
    conn_ptr_->set_logger(logger_ptr_);
}

Engine::Engine(Mode work_mode)
    : master_ptr_(NULL)
    , touched_(false)
    , echo_(false)
    , mode_(work_mode)
    , timeout_(0)
    , conn_(new SqlConnection(_T("DEFAULT"), cfg(_T("DBTYPE")),
                cfg(_T("DB")), cfg(_T("USER")), cfg(_T("PASSWD"))))
    , logger_ptr_(NULL)
    , pool_ptr_(NULL)
    , conn_ptr_(NULL)
    , dialect_(conn_->get_dialect())
{}

Engine::Engine(Mode work_mode, auto_ptr<SqlConnection> conn)
    : master_ptr_(NULL)
    , touched_(false)
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
    , touched_(false)
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
    , touched_(false)
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
        conn->set_logger(logger_.get());
}

SqlResultSet
Engine::select_iter(const StrList &what,
        const StrList &from, const Filter &where,
        const StrList &group_by, const Filter &having,
        const StrList &order_by, int max_rows, bool for_update)
{
    bool select_mode = (mode_ == FORCE_SELECT_UPDATE) ? true : for_update;
    if ((mode_ == READ_ONLY) && select_mode)
        throw BadOperationInMode(
                _T("Using SELECT FOR UPDATE in read-only mode"));
    String sql;
    Values params;
    do_gen_sql_select(sql, params,
            what, from, where, group_by, having, order_by, for_update);
    auto_ptr<SqlCursor> cursor = get_conn()->new_cursor();
    cursor->prepare(sql);
    SqlResultSet rs = cursor->exec(params);
    rs.own(cursor);
    if (select_mode)
        touched_ = true;
    return rs;
}

RowsPtr
Engine::select(const StrList &what,
        const StrList &from, const Filter &where,
        const StrList &group_by, const Filter &having,
        const StrList &order_by, int max_rows, bool for_update)
{
    bool select_mode = (mode_ == FORCE_SELECT_UPDATE) ? true : for_update;
    if ((mode_ == READ_ONLY) && select_mode)
        throw BadOperationInMode(
                _T("Using SELECT FOR UPDATE in read-only mode"));
    RowsPtr rows = on_select(what, from, where,
                group_by, having, order_by, max_rows, select_mode);
    if (select_mode)
        touched_ = true;
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
    touched_ = true;
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
    on_update(table_name, rows, key_fields, exclude_fields, where);
    touched_ = true;
}

void
Engine::delete_from(const String &table_name, const Filter &where)
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode(
                _T("Using DELETE operation in read-only mode"));
    on_delete(table_name, where);
    touched_ = true;
}

void
Engine::delete_from(const String &table_name, const Keys &keys)
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode(
                _T("Using DELETE operation in read-only mode"));
    Keys::const_iterator i = keys.begin(), iend = keys.end();
    for (; i != iend; ++i)
        on_delete(table_name, KeyFilter(*i));
    touched_ = true;
}

void
Engine::exec_proc(const String &proc_code)
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode(
                _T("Trying to invoke a PROCEDURE in read-only mode"));
    on_exec_proc(proc_code);
    touched_ = true;
}

void
Engine::commit()
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode(
                _T("Using COMMIT operation in read-only mode"));
    on_commit();
    touched_ = false;
}

void
Engine::rollback()
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode(
                _T("Using ROLLBACK operation in read-only mode"));
    on_rollback();
    touched_ = false;
}

RowPtr
Engine::select_row(const StrList &what,
        const StrList &from, const Filter &where)
{
    RowsPtr rows = select(what, from, where);
    if (rows->size() != 1)
        throw NoDataFound(_T("Unable to fetch exactly one row!"));
    RowPtr row(new Row((*rows)[0]));
    return row;
}

RowPtr
Engine::select_row(const StrList &from, const Filter &where)
{
    return select_row(_T("*"), from, where);
}

const Value
Engine::select1(const String &what, const String &from, const Filter &where)
{
    RowPtr row = select_row(what, from, where);
    if (row->size() != 1)
        throw BadSQLOperation(_T("Unable to fetch exactly one column!"));
    return row->begin()->second;
}

LongInt
Engine::get_curr_value(const String &seq_name)
{
    return select1(dialect_->select_curr_value(seq_name),
            dialect_->dual_name(), Filter()).as_longint();
}

LongInt
Engine::get_next_value(const String &seq_name)
{
    return select1(dialect_->select_next_value(seq_name),
            dialect_->dual_name(), Filter()).as_longint();
}

RowsPtr
Engine::on_select(const StrList &what,
        const StrList &from, const Filter &where,
        const StrList &group_by, const Filter &having,
        const StrList &order_by, int max_rows,
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
            cursor->exec_direct(_T("SELECT LAST_INSERT_ID() LID"));
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
        const StrList &what, const StrList &from, const Filter &where,
        const StrList &group_by, const Filter &having,
        const StrList &order_by, bool for_update) const
{
    String s;
    std::ostringstream sql_query;
    sql_query << "SELECT " << NARROW(what.get_str());
    if (!str_empty(from.get_str()))
        sql_query << " FROM " << NARROW(from.get_str());

    s = where.collect_params_and_build_sql(params);
    if (s != Filter().get_sql())
        sql_query << " WHERE " << NARROW(s);
    if (!str_empty(group_by.get_str()))
        sql_query << " GROUP BY " << NARROW(group_by.get_str());
    s = having.collect_params_and_build_sql(params);
    if (s != Filter().get_sql()) {
        if (str_empty(group_by.get_str()))
            throw BadSQLOperation(
                    _T("Trying to use HAVING without GROUP BY clause"));
        sql_query << " HAVING " << NARROW(s);
    }
    if (!str_empty(order_by.get_str()))
        sql_query << " ORDER BY " << NARROW(order_by.get_str());
    if (for_update)
        sql_query << " FOR UPDATE";
    sql = WIDEN(sql_query.str());
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
    typedef vector<String> vector_string;
    vector_string names;
    vector_string pholders;
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
    sql_query << NARROW(StrList(names).get_str())
        << ") VALUES ("
        << NARROW(StrList(pholders).get_str())
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
        if (exclude_fields.find(it->first) == exclude_fields.end() &&
            std::find(key_fields.begin(), key_fields.end(), it->first) == key_fields.end())
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
    String s = where.collect_params_and_build_sql(params);
    if (s != Filter().get_sql())
    {
        where_sql << "(" << NARROW(s) << ")";
        if (!key_fields.empty())
            where_sql << " AND ";
    }
    Strings::const_iterator it_where = key_fields.begin(), end_where = key_fields.end();
    Strings::const_iterator it_last =
        (key_fields.empty()) ? key_fields.end() : --key_fields.end();
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
    String s = where.collect_params_and_build_sql(params);
    if (s != Filter().get_sql())
        sql_query << " WHERE " << NARROW(s);
    sql = WIDEN(sql_query.str());
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
