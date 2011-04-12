#include <sstream>
#include <util/str_utils.hpp>
#include "OdbcSession.h"

using namespace std;
using namespace Yb::StrUtils;

namespace Yb {

OdbcSession::OdbcSession(mode work_mode)
    : Session(work_mode, xgetenv("YBORM_DBTYPE"))
    , drv_(new OdbcDriver())
{
    drv_->open(xgetenv("YBORM_DB"), xgetenv("YBORM_USER"), xgetenv("YBORM_PASSWD"));
}

OdbcSession::~OdbcSession()
{
    delete drv_;
}

RowsPtr
OdbcSession::on_select(const StrList &what,
        const StrList &from, const Filter &where,
        const StrList &group_by, const Filter &having,
        const StrList &order_by, int max_rows,
        bool for_update)
{
    string sql;
    Values params;
    do_gen_sql_select(sql, params,
            what, from, where, group_by, having, order_by, for_update);
    drv_->prepare(sql);
    drv_->exec(params);
    return drv_->fetch_rows(max_rows);
}

const vector<LongInt>
OdbcSession::on_insert(const string &table_name,
        const Rows &rows, const FieldSet &exclude_fields,
        bool collect_new_ids)
{
    vector<LongInt> ids;
    if (!rows.size())
        return ids;
    string sql;
    Values params;
    ParamNums param_nums;
    do_gen_sql_insert(sql, params, param_nums, table_name, rows[0], exclude_fields);
    if (!collect_new_ids) {
        drv_->prepare(sql);
        Rows::const_iterator it = rows.begin(), end = rows.end();
        for (; it != end; ++it) {
            Row::const_iterator f = it->begin(), fend = it->end();
            for (; f != fend; ++f)
                if (exclude_fields.find(f->first) == exclude_fields.end())
                    params[param_nums[f->first]] = f->second;
            drv_->exec(params);
        }
    }
    else {
        Rows::const_iterator it = rows.begin(), end = rows.end();
        for (; it != end; ++it) {
            drv_->prepare(sql);
            Row::const_iterator f = it->begin(), fend = it->end();
            for (; f != fend; ++f)
                if (exclude_fields.find(f->first) == exclude_fields.end())
                    params[param_nums[f->first]] = f->second;
            drv_->exec(params);
            drv_->exec_direct("SELECT LAST_INSERT_ID() LID");
            RowsPtr id_rows = drv_->fetch_rows();
            ids.push_back((*id_rows)[0]["LID"].as_longint());
        }
    }
    return ids;
}

void
OdbcSession::on_update(const string &table_name,
        const Rows &rows, const FieldSet &key_fields,
        const FieldSet &exclude_fields, const Filter &where)
{
    if (!rows.size())
        return;
    string sql;
    Values params;
    ParamNums param_nums;
    do_gen_sql_update(sql, params, param_nums, table_name, rows[0], key_fields, exclude_fields, where);
    drv_->prepare(sql);
    Rows::const_iterator it = rows.begin(), end = rows.end();
    for (; it != end; ++it) {
        Row::const_iterator f = it->begin(), fend = it->end();
        for (; f != fend; ++f)
            if (exclude_fields.find(f->first) == exclude_fields.end())
                params[param_nums[f->first]] = f->second;
        drv_->exec(params);
    }
}

void
OdbcSession::on_delete(const string &table_name, const Filter &where)
{
    string sql;
    Values params;
    do_gen_sql_delete(sql, params, table_name, where);
    drv_->prepare(sql);
    drv_->exec(params);
}

void
OdbcSession::on_exec_proc(const string &proc_code)
{
    drv_->exec_direct(proc_code);
}

void
OdbcSession::on_commit()
{
    drv_->commit();
}

void
OdbcSession::on_rollback()
{
    drv_->rollback();
}

void
OdbcSession::do_gen_sql_select(string &sql, Values &params,
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
            throw BadSQLOperation("Trying to use HAVING without GROUP BY clause");
        sql_query << " HAVING " << s;
    }
    if (!order_by.get_str().empty())
        sql_query << " ORDER BY " << order_by.get_str();
    if (for_update)
        sql_query << " FOR UPDATE";
    sql = sql_query.str();
}

void
OdbcSession::do_gen_sql_insert(string &sql, Values &params,
        ParamNums &param_nums, const string &table_name,
        const Row &row, const FieldSet &exclude_fields) const
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
        if (exclude_fields.find(it->first) == exclude_fields.end()) {
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
OdbcSession::do_gen_sql_update(string &sql, Values &params,
        ParamNums &param_nums, const string &table_name,
        const Row &row, const FieldSet &key_fields,
        const FieldSet &exclude_fields, const Filter &where) const
{
    if (key_fields.empty() && (where.get_sql() == Filter().get_sql()))
        throw BadSQLOperation("Can't do UPDATE without where clause and empty key_fields");
    if (row.empty())
        throw BadSQLOperation("Can't UPDATE with empty row set");

    Row excluded_row;
    ostringstream sql_query;
    sql_query << "UPDATE " << table_name << " SET ";
    Row::const_iterator it = row.begin(), end = row.end();
    for (; it != end; ++it) {
        if ((exclude_fields.find(it->first) == exclude_fields.end()) &&
                (key_fields.find(it->first) == key_fields.end()))
            excluded_row.insert(make_pair(it->first, it->second));
    }

    Row::const_iterator last = (excluded_row.empty() ? excluded_row.end() : --excluded_row.end());
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
    FieldSet::const_iterator it_where = key_fields.begin(), end_where = key_fields.end();
    FieldSet::const_iterator it_last =
        (key_fields.empty()) ? key_fields.end() : --key_fields.end();
    for (; it_where != end_where; ++it_where) {
        Row::const_iterator it_find = row.find(*it_where);
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
OdbcSession::do_gen_sql_delete(string &sql, Values &params,
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
