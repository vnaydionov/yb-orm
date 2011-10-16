#include <sstream>
#include <util/str_utils.hpp>
#include "Engine.h"

using namespace std;
using namespace Yb::StrUtils;

namespace Yb {

static inline const string cfg(const string &entry)
{ return xgetenv("YBORM_" + entry); }

EngineBase::~EngineBase()
{}

Engine::Engine(Mode work_mode)
    : touched_(false)
    , mode_(work_mode)
    , conn_(new SqlConnect("ODBC", cfg("DBTYPE"),
                cfg("DB"), cfg("USER"), cfg("PASSWD")))
    , dialect_(conn_->get_dialect())
{}

Engine::Engine(Mode work_mode, auto_ptr<SqlConnect> conn)
    : touched_(false)
    , mode_(work_mode)
    , conn_(conn)
    , dialect_(conn_->get_dialect())
{}

Engine::Engine(Mode work_mode, SqlDialect *dialect)
    : touched_(false)
    , mode_(work_mode)
    , dialect_(dialect)
{}

Engine::~Engine()
{}

SqlResultSet
Engine::select_iter(const StrList &what,
        const StrList &from, const Filter &where,
        const StrList &group_by, const Filter &having,
        const StrList &order_by, int max_rows, bool for_update)
{
    bool select_mode = (mode_ == FORCE_SELECT_UPDATE) ? true : for_update;
    if ((mode_ == READ_ONLY) && select_mode)
        throw BadOperationInMode(
                "Using SELECT FOR UPDATE in read-only mode");
    string sql;
    Values params;
    do_gen_sql_select(sql, params,
            what, from, where, group_by, having, order_by, for_update);
    conn_->prepare(sql);
    SqlResultSet rs = conn_->exec(params);
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
                "Using SELECT FOR UPDATE in read-only mode");
    RowsPtr rows = on_select(what, from, where,
                group_by, having, order_by, max_rows, select_mode);
    if (select_mode)
        touched_ = true;
    return rows;
}

const vector<LongInt>
Engine::insert(const string &table_name,
        const Rows &rows, const StringSet &exclude_fields,
        bool collect_new_ids)
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode(
                "Using INSERT operation in read-only mode");
    touched_ = true;
    return on_insert(table_name, rows, exclude_fields, collect_new_ids);
}

void
Engine::update(const string &table_name,
        const Rows &rows, const StringSet &key_fields,
        const StringSet &exclude_fields, const Filter &where)
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode(
                "Using UPDATE operation in read-only mode");
    on_update(table_name, rows, key_fields, exclude_fields, where);
    touched_ = true;
}

void
Engine::delete_from(const string &table_name, const Filter &where)
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode(
                "Using DELETE operation in read-only mode");
    on_delete(table_name, where);
    touched_ = true;
}

void
Engine::delete_from(const string &table_name, const Keys &keys)
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode(
                "Using DELETE operation in read-only mode");
    Keys::const_iterator i = keys.begin(), iend = keys.end();
    for (; i != iend; ++i)
        on_delete(table_name, KeyFilter(*i));
    touched_ = true;
}

void
Engine::exec_proc(const string &proc_code)
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode(
                "Trying to invoke a PROCEDURE in read-only mode");
    on_exec_proc(proc_code);
    touched_ = true;
}

void
Engine::commit()
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode(
                "Using COMMIT operation in read-only mode");
    on_commit();
    touched_ = false;
}

void
Engine::rollback()
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode(
                "Using ROLLBACK operation in read-only mode");
    on_rollback();
    touched_ = false;
}

RowPtr
Engine::select_row(const StrList &what,
        const StrList &from, const Filter &where)
{
    RowsPtr rows = select(what, from, where);
    if (rows->size() != 1)
        throw NoDataFound("Unable to fetch exactly one row!");
    RowPtr row(new Row((*rows)[0]));
    return row;
}

RowPtr
Engine::select_row(const StrList &from, const Filter &where)
{
    return select_row("*", from, where);
}

const Value
Engine::select1(const string &what, const string &from, const Filter &where)
{
    RowPtr row = select_row(what, from, where);
    if (row->size() != 1)
        throw BadSQLOperation("Unable to fetch exactly one column!");
    return row->begin()->second;
}

LongInt
Engine::get_curr_value(const string &seq_name)
{
    return select1(dialect_->select_curr_value(seq_name),
            dialect_->dual_name(), Filter()).as_longint();
}

LongInt
Engine::get_next_value(const string &seq_name)
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
    string sql;
    Values params;
    do_gen_sql_select(sql, params,
            what, from, where, group_by, having, order_by, for_update);
    conn_->prepare(sql);
    conn_->exec(params);
    return conn_->fetch_rows(max_rows);
}

const vector<LongInt>
Engine::on_insert(const string &table_name,
        const Rows &rows, const StringSet &exclude_fields,
        bool collect_new_ids)
{
    vector<LongInt> ids;
    if (!rows.size())
        return ids;
    string sql;
    Values params;
    ParamNums param_nums;
    do_gen_sql_insert(sql, params, param_nums, table_name,
            rows[0], exclude_fields);
    if (!collect_new_ids) {
        conn_->prepare(sql);
        Rows::const_iterator it = rows.begin(), end = rows.end();
        for (; it != end; ++it) {
            Row::const_iterator f = it->begin(), fend = it->end();
            for (; f != fend; ++f) {
                StringSet::const_iterator x = exclude_fields.find(f->first);
                if (x == exclude_fields.end())
                    params[param_nums[f->first]] = f->second;
            }
            conn_->exec(params);
        }
    }
    else {
        Rows::const_iterator it = rows.begin(), end = rows.end();
        for (; it != end; ++it) {
            conn_->prepare(sql);
            Row::const_iterator f = it->begin(), fend = it->end();
            for (; f != fend; ++f) {
                StringSet::const_iterator x = exclude_fields.find(f->first);
                if (x == exclude_fields.end())
                    params[param_nums[f->first]] = f->second;
            }
            conn_->exec(params);
            conn_->exec_direct("SELECT LAST_INSERT_ID() LID");
            RowsPtr id_rows = conn_->fetch_rows();
            ids.push_back((*id_rows)[0][0].second.as_longint());
        }
    }
    return ids;
}

void
Engine::on_update(const string &table_name,
        const Rows &rows, const StringSet &key_fields,
        const StringSet &exclude_fields, const Filter &where)
{
    if (!rows.size())
        return;
    string sql;
    Values params;
    ParamNums param_nums;
    do_gen_sql_update(sql, params, param_nums, table_name,
            rows[0], key_fields, exclude_fields, where);
    conn_->prepare(sql);
    Rows::const_iterator it = rows.begin(), end = rows.end();
    for (; it != end; ++it) {
        Row::const_iterator f = it->begin(), fend = it->end();
        for (; f != fend; ++f) {
            StringSet::const_iterator x = exclude_fields.find(f->first);
            if (x == exclude_fields.end())
                params[param_nums[f->first]] = f->second;
        }
        conn_->exec(params);
    }
}

void
Engine::on_delete(const string &table_name, const Filter &where)
{
    string sql;
    Values params;
    do_gen_sql_delete(sql, params, table_name, where);
    conn_->prepare(sql);
    conn_->exec(params);
}

void
Engine::on_exec_proc(const string &proc_code)
{
    conn_->exec_direct(proc_code);
}

void
Engine::on_commit()
{
    conn_->commit();
}

void
Engine::on_rollback()
{
    conn_->rollback();
}

void
Engine::do_gen_sql_select(string &sql, Values &params,
        const StrList &what, const StrList &from, const Filter &where,
        const StrList &group_by, const Filter &having,
        const StrList &order_by, bool for_update) const
{
    string s;
    ostringstream sql_query;
    sql_query << "SELECT " << what.get_str() << " FROM " << from.get_str();

    s = where.collect_params_and_build_sql(params);
    if (s != Filter().get_sql())
        sql_query << " WHERE " << s;
    if (!group_by.get_str().empty())
        sql_query << " GROUP BY " << group_by.get_str();
    s = having.collect_params_and_build_sql(params);
    if (s != Filter().get_sql()) {
        if (group_by.get_str().empty())
            throw BadSQLOperation(
                    "Trying to use HAVING without GROUP BY clause");
        sql_query << " HAVING " << s;
    }
    if (!order_by.get_str().empty())
        sql_query << " ORDER BY " << order_by.get_str();
    if (for_update)
        sql_query << " FOR UPDATE";
    sql = sql_query.str();
}

void
Engine::do_gen_sql_insert(string &sql, Values &params,
        ParamNums &param_nums, const string &table_name,
        const Row &row, const StringSet &exclude_fields) const
{
    if (row.empty())
        throw BadSQLOperation("Can't do INSERT with empty row");
    ostringstream sql_query;
    sql_query << "INSERT INTO " << table_name << " (";
    typedef vector<string> vector_string;
    vector_string names;
    vector_string pholders;
    Row::const_iterator it = row.begin(), end = row.end();
    for (; it != end; ++it) {
        StringSet::const_iterator x = exclude_fields.find(it->first);
        if (x == exclude_fields.end()) {
            param_nums[it->first] = params.size();
            params.push_back(it->second);
            names.push_back(it->first);
            pholders.push_back("?");
        }
    }
    sql_query << StrList(names).get_str()
        << ") VALUES ("
        << StrList(pholders).get_str()
        << ")";
    sql = sql_query.str();
}

void
Engine::do_gen_sql_update(string &sql, Values &params,
        ParamNums &param_nums, const string &table_name,
        const Row &row, const StringSet &key_fields,
        const StringSet &exclude_fields, const Filter &where) const
{
    if (key_fields.empty() && (where.get_sql() == Filter().get_sql()))
        throw BadSQLOperation(
                "Can't do UPDATE without where clause and empty key_fields");
    if (row.empty())
        throw BadSQLOperation("Can't UPDATE with empty row set");

    Row excluded_row;
    ostringstream sql_query;
    sql_query << "UPDATE " << table_name << " SET ";
    Row::const_iterator it = row.begin(), end = row.end();
    for (; it != end; ++it) {
        StringSet::const_iterator x = exclude_fields.find(it->first),
            y = key_fields.find(it->first);
        if ((x == exclude_fields.end()) && (y == key_fields.end()))
            excluded_row.push_back(make_pair(it->first, it->second));
    }

    Row::const_iterator last = (excluded_row.empty() ?
            excluded_row.end() : --excluded_row.end());
    end = excluded_row.end();
    for (it = excluded_row.begin(); it != excluded_row.end(); ++it)
    {
        param_nums[it->first] = params.size();
        params.push_back(it->second);
        sql_query << it->first << " = ?";
        if (it != last)
            sql_query << ", ";
    }

    sql_query << " WHERE ";
    ostringstream where_sql;
    string s = where.collect_params_and_build_sql(params);
    if (s != Filter().get_sql())
    {
        where_sql << "(" << s << ")";
        if (!key_fields.empty())
            where_sql << " AND ";
    }
    StringSet::const_iterator it_where = key_fields.begin(),
        end_where = key_fields.end();
    StringSet::const_iterator it_last =
        (key_fields.empty()) ? key_fields.end() : --key_fields.end();
    for (; it_where != end_where; ++it_where) {
        Row::const_iterator it_find = find_in_row(row, *it_where);
        if (it_find != row.end()) {
            param_nums[it_find->first] = params.size();
            params.push_back(it_find->second);
            where_sql << "(" << it_find->first << " = ?)";
            if (it_where != it_last) {
                where_sql << " AND ";
            }
        }
    }
    if(where_sql.str().empty())
        throw BadSQLOperation("Can't do UPDATE with empty where clause");
    sql_query << where_sql.str();
    sql = sql_query.str();
}

void
Engine::do_gen_sql_delete(string &sql, Values &params,
        const string &table, const Filter &where) const
{
    if (where.get_sql() == Filter().get_sql())
        throw BadSQLOperation("Can't DELETE without where clause");
    ostringstream sql_query;
    sql_query << "DELETE FROM " << table;
    string s = where.collect_params_and_build_sql(params);
    if (s != Filter().get_sql())
        sql_query << " WHERE " << s;
    sql = sql_query.str();
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
