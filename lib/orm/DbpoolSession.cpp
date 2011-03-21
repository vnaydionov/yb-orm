
#include <time.h>
#include <sstream>
#include <stdexcept>
#include <util/str_utils.hpp>
#include <dbpool/mypp.h>
#include <dbpool/handle_var.h>
#include <dbpool/db_pool_single.h>
#include "DbpoolSession.h"

#define handle_var_ (*(mypp::Handle_var *)h_)

using namespace std;

namespace Yb {
namespace SQL {

static string cfg(const string &name, bool required = true)
{
    string value = ::Yb::StrUtils::xgetenv(name);
    if (value.empty() && required)
        throw std::runtime_error("Can't read required env.var: " + name);
    return value;
}

DBPoolSession::DBPoolSession(mode work_mode)
    : Session(work_mode)
    , h_(NULL)
{
    mypp::DSN dsn(cfg("YBORM_DRIVER"), cfg("YBORM_HOST"),
            boost::lexical_cast<int>(cfg("YBORM_PORT")),
            cfg("YBORM_USER"), cfg("YBORM_PASSWD"),
            cfg("YBORM_DB"));
    int timeout = boost::lexical_cast<int>(cfg("YBORM_TIMEOUT"));
    try {
        h_ = (void *)new mypp::Handle_var(
                mypp::theDBPool::Instance(), dsn, timeout);
    }
    catch (const mypp::Exception &e) {
        throw GenericDBError(e.error());
    }
}

DBPoolSession::~DBPoolSession()
{
    if (is_touched())
        rollback();
    delete (mypp::Handle_var *)h_;
}

static time_t ptime2time_t(const boost::posix_time::ptime &t)
{   
    tm stm;
    stm.tm_year = (int)t.date().year() - 1900;
    stm.tm_mon = (int)t.date().month() - 1;
    stm.tm_mday = (int)t.date().day();
    stm.tm_hour = t.time_of_day().hours();
    stm.tm_min = t.time_of_day().minutes();
    stm.tm_sec = t.time_of_day().seconds();
    stm.tm_wday = stm.tm_yday = 0;
    stm.tm_isdst = -1;
    return mktime(&stm);
}

const boost::posix_time::ptime
DBPoolSession::fix_dt_hook(const boost::posix_time::ptime &t)
{
#if 0
    // Fix DBPool bug: it returns DateTime - GMT_offset
    time_t old_t = ptime2time_t(t);
    // Calculate current GMT offset
    time_t curr_t = time(NULL);
    tm gmt_stm;
    gmtime_r(&curr_t, &gmt_stm);
    gmt_stm.tm_isdst = -1;
    long gmt_offset = curr_t - mktime(&gmt_stm);
    LOG(LogLevel::DEBUG, "DBPool: fix_dt_hook: gmt_offset = " << gmt_offset);
    // Add the offset
    time_t new_t = old_t + gmt_offset;
    tm stm;
    localtime_r(&new_t, &stm);
    return boost::posix_time::ptime(
            boost::gregorian::date(stm.tm_year + 1900, stm.tm_mon + 1, stm.tm_mday),
            boost::posix_time::time_duration(stm.tm_hour, stm.tm_min, stm.tm_sec));
#else
    return t;
#endif
}

RowsPtr
DBPoolSession::on_select(const StrList &what,
        const StrList &from, const Filter &where,
        const StrList &group_by, const Filter &having,
        const StrList &order_by, int max_rows,
        bool for_update)
{
    string sql;
    do_gen_sql_select(sql, what, from, where, group_by,
            having, order_by, for_update);

    try {
        RowsPtr rows(new Rows);
        mypp::Statement stmt(handle_var_, sql);
        stmt.Execute();
        int row_count = 0;
        for (mypp::Row row; stmt.Fetch(row); ) {
            if (max_rows >= 0 && row_count >= max_rows)
                break;
            Row orm_row;
            for (int i = 0; i < row.size(); ++i) {
                const string name = Yb::StrUtils::str_to_upper(row[i].getName());
                if (row[i].isNull())
                    orm_row[name] = Value();
                else
                    orm_row[name] = Value(row[i].asString());
            }
            rows->push_back(orm_row);
            ++row_count;
        }
        return rows;
    }
    catch (const mypp::Exception &e) {
        throw GenericDBError(e.error());
    }
}

const std::vector<long long>
DBPoolSession::on_insert(const string &table_name,
        const Rows &rows, const FieldSet &exclude_fields,
        bool collect_new_ids)
{
    try {
        std::vector<long long> ids;
        Rows::const_iterator it = rows.begin(), end = rows.end();
        for(; it != end; ++it) {
            string sql;
            do_gen_sql_insert(sql, table_name, *it, exclude_fields);
            {
                mypp::Statement stmt(handle_var_, sql);
                stmt.Execute();
            }
            if (collect_new_ids) {
                mypp::Statement stmt(handle_var_,
                        "SELECT LAST_INSERT_ID() LID");
                stmt.Execute();
                mypp::Row row;
                if (stmt.Fetch(row))
                    ids.push_back(row[0].asLongLong());
            }
        }
        return ids;
    } catch (const mypp::Exception &e) {
        throw GenericDBError(e.error());
    }
}

void
DBPoolSession::on_update(const string &table_name,
        const Rows &rows, const FieldSet &key_fields,
        const FieldSet &exclude_fields, const Filter &where)
{
    try {
        Rows::const_iterator it = rows.begin(), end = rows.end();
        for(; it != end; ++it) {
            string sql;
            do_gen_sql_update(sql, table_name, *it, key_fields, exclude_fields, where);
            mypp::Statement stmt(handle_var_, sql);
            stmt.Execute();
        }
    } catch (const mypp::Exception &e) {
        throw GenericDBError(e.error());
    }
}

void
DBPoolSession::on_delete(const string &table_name, const Filter &where)
{
    string sql;
    do_gen_sql_delete(sql, table_name, where);
    try {
        mypp::Statement stmt(handle_var_, sql);
        stmt.Execute();
    } catch (const mypp::Exception &e) {
        throw GenericDBError(e.error());
    }
}

void
DBPoolSession::on_exec_proc(const string &proc_code)
{
    try {
        mypp::Statement stmt(handle_var_, proc_code);
        stmt.Execute();
    } catch (const mypp::Exception &e) {
        throw GenericDBError(e.error());
    }
}

void
DBPoolSession::on_commit()
{
    try {
        mypp::Statement stmt(handle_var_, "COMMIT");
        stmt.Execute();
    } catch (const mypp::Exception &e) {
        throw GenericDBError(e.error());
    }
}

void
DBPoolSession::on_rollback()
{
    try {
        mypp::Statement stmt(handle_var_, "ROLLBACK");
        stmt.Execute();
    } catch(const mypp::Exception &e) {
        throw GenericDBError(e.error());
    }
}

void
DBPoolSession::do_gen_sql_insert(string &sql, const string &table_name,
        const Row &row, const FieldSet &exclude_fields) const
{
    if (row.empty())
        throw BadSQLOperation("Can't do INSERT with empty row");
    ostringstream sql_query;
    sql_query << "INSERT INTO " << table_name << " (";
    typedef vector<string> vector_string;
    vector_string values;
    vector_string names;
    Row::const_iterator it = row.begin(), end = row.end();
    for (; it != end; ++it) {
        if (exclude_fields.find(it->first) == exclude_fields.end()) {
            values.push_back(it->second.sql_str());
            names.push_back(it->first);
        }
    }
    sql_query << StrList(names).get_str()
        << ") VALUES ("
        << StrList(values).get_str()
        << ")";
    sql = sql_query.str();
}

void
DBPoolSession::do_gen_sql_update(string &sql, const string &table_name,
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
        sql_query << it->first << " = " << it->second.sql_str();
        if (it != last)
            sql_query << ", ";
    }

    sql_query << " WHERE ";
    ostringstream where_sql;
    if (where.get_sql() != Filter().get_sql())
    {
        where_sql << "(" << where.get_sql() << ")";
        if (!key_fields.empty())
            where_sql << " AND ";
    }
    FieldSet::const_iterator it_where = key_fields.begin(), end_where = key_fields.end();
    FieldSet::const_iterator it_last =
        (key_fields.empty()) ? key_fields.end() : --key_fields.end();
    for (; it_where != end_where; ++it_where) {
        Row::const_iterator it_find = row.find(*it_where);
        if (it_find != row.end()) {
            where_sql << "(" << it_find->first << " = " << it_find->second.sql_str() << ")";
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
DBPoolSession::do_gen_sql_select(string &sql, const StrList &what,
        const StrList &from, const Filter &where,
        const StrList &group_by, const Filter &having,
        const StrList &order_by, bool for_update) const
{
    ostringstream sql_query;
    sql_query << "SELECT " << what.get_str() << " FROM " << from.get_str();

    if (where.get_sql() != Filter().get_sql())
        sql_query << " WHERE " << where.get_sql();
    if (!group_by.get_str().empty())
        sql_query << " GROUP BY " << group_by.get_str();
    if (having.get_sql() != Filter().get_sql()) {
        if (group_by.get_str().empty())
            throw BadSQLOperation("Trying to use HAVING without GROUP BY clause");
        sql_query << " HAVING " << having.get_sql();
    }
    if (!order_by.get_str().empty())
        sql_query << " ORDER BY " << order_by.get_str();
    if (for_update)
        sql_query << " FOR UPDATE";
    sql = sql_query.str();
}

void
DBPoolSession::do_gen_sql_delete(string &sql, const string &table,
        const Filter &where) const
{
    if (where.get_sql() == Filter().get_sql())
        throw BadSQLOperation("Can't DELETE without where clause");
    ostringstream sql_query;
    sql_query << "DELETE FROM " << table << " WHERE " << where.get_sql();
    sql = sql_query.str();
}

} // namespace SQL
} // namespace Yb

// vim:ts=4:sts=4:sw=4:et

