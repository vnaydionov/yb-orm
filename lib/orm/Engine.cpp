#include <sstream>
#include <algorithm>
#include <util/str_utils.hpp>
#include <orm/Engine.h>
#include <orm/CodeGen.h>

using namespace std;
using namespace Yb::StrUtils;

namespace Yb {

EngineBase::~EngineBase()
{}

EngineSource::~EngineSource()
{}

SqlResultSet
EngineBase::select_iter(const Expression &select_expr)
{
    bool num_params = get_conn()->get_driver()->numbered_params();
    int count = 1, *pcount = NULL;
    if (num_params)
        pcount = &count;
    Values params;
    String sql = select_expr.generate_sql(&params, pcount);
    auto_ptr<SqlCursor> cursor = get_conn()->new_cursor();
    cursor->prepare(sql);
    SqlResultSet rs = cursor->exec(params);
    rs.own(cursor);
    return rs;
}

RowsPtr
EngineBase::select(const Expression &what,
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
EngineBase::insert(const Table &table, const RowsData &rows,
        bool collect_new_ids)
{
    if (get_mode() == READ_ONLY)
        throw BadOperationInMode(
                _T("Using INSERT operation in read-only mode"));
    vector<LongInt> ids;
    if (!rows.size())
        return ids;
    touch();
    String sql;
    TypeCodes type_codes;
    ParamNums param_nums;
    gen_sql_insert(sql, type_codes, param_nums, table,
            !collect_new_ids, get_conn()->get_driver()->numbered_params());
    Values params(type_codes.size());
    auto_ptr<SqlCursor> cursor = get_conn()->new_cursor();
    cursor->prepare(sql);
    cursor->bind_params(type_codes);
    auto_ptr<SqlCursor> cursor2;
    if (collect_new_ids)
        cursor2.reset(get_conn()->new_cursor().release());
    RowsData::const_iterator r = rows.begin(), rend = rows.end();
    for (; r != rend; ++r) {
        ParamNums::const_iterator f = param_nums.begin(),
            fend = param_nums.end();
        for (; f != fend; ++f)
            params[f->second] = (**r)[table.idx_by_name(f->first)];
        cursor->exec(params);
        if (collect_new_ids) {
            cursor2->prepare(get_dialect()->
                    select_last_inserted_id(table.name()));
            cursor2->exec(Values());
            RowsPtr id_rows = cursor2->fetch_rows();
            ids.push_back((*id_rows)[0][0].second.as_longint());
        }
    }
    return ids;
}

void
EngineBase::update(const Table &table, const RowsData &rows)
{
    if (get_mode() == READ_ONLY)
        throw BadOperationInMode(
                _T("Using UPDATE operation in read-only mode"));
    if (!rows.size())
        return;
    touch();
    String sql;
    TypeCodes type_codes;
    ParamNums param_nums;
    gen_sql_update(sql, type_codes, param_nums, table,
            get_conn()->get_driver()->numbered_params());
    auto_ptr<SqlCursor> cursor = get_conn()->new_cursor();
    cursor->prepare(sql);
    cursor->bind_params(type_codes);
    Values params(type_codes.size());
    RowsData::const_iterator r = rows.begin(), rend = rows.end();
    for (; r != rend; ++r) {
        ParamNums::const_iterator f = param_nums.begin(),
            fend = param_nums.end();
        for (; f != fend; ++f)
            params[f->second] = (**r)[table.idx_by_name(f->first)];
        cursor->exec(params);
    }
}

void
EngineBase::delete_from(const Table &table, const Keys &keys)
{
    if (get_mode() == READ_ONLY)
        throw BadOperationInMode(
                _T("Using DELETE operation in read-only mode"));
    if (!keys.size())
        return;
    touch();
    String sql;
    TypeCodes type_codes;
    gen_sql_delete(sql, type_codes, table,
            get_conn()->get_driver()->numbered_params());
    auto_ptr<SqlCursor> cursor = get_conn()->new_cursor();
    cursor->prepare(sql);
    cursor->bind_params(type_codes);
    Values params(type_codes.size());
    Keys::const_iterator k = keys.begin(), kend = keys.end();
    for (; k != kend; ++k) {
        for (size_t i = 0; i < k->second.size(); ++i)
            params[i] = k->second[i].second;
        cursor->exec(params);
    }
}

void
EngineBase::exec_proc(const String &proc_code)
{
    if (get_mode() == READ_ONLY)
        throw BadOperationInMode(
                _T("Trying to invoke a PROCEDURE in read-only mode"));
    touch();
    auto_ptr<SqlCursor> cursor = get_conn()->new_cursor();
    cursor->exec_direct(proc_code);
}

RowPtr
EngineBase::select_row(const Expression &what,
        const Expression &from, const Expression &where)
{
    RowsPtr rows = select(what, from, where);
    if (rows->size() != 1)
        throw NoDataFound(_T("Unable to fetch exactly one row!"));
    RowPtr row(new Row((*rows)[0]));
    return row;
}

const Value
EngineBase::select1(const Expression &what,
        const Expression &from, const Expression &where)
{
    RowPtr row = select_row(what, from, where);
    if (row->size() != 1)
        throw BadSQLOperation(_T("Unable to fetch exactly one column!"));
    return row->begin()->second;
}

LongInt
EngineBase::get_curr_value(const String &seq_name)
{
    return select1(Expression(get_dialect()->select_curr_value(seq_name)),
            Expression(get_dialect()->dual_name()), Expression()).as_longint();
}

LongInt
EngineBase::get_next_value(const String &seq_name)
{
    return select1(Expression(get_dialect()->select_next_value(seq_name)),
            Expression(get_dialect()->dual_name()), Expression()).as_longint();
}

void
EngineBase::commit()
{
    get_conn()->commit();
}

void
EngineBase::rollback()
{
    get_conn()->rollback();
}

void
EngineBase::touch()
{
    get_conn()->begin_trans_if_necessary();
}

void
EngineBase::create_schema(const Schema &schema, bool ignore_errors)
{
    SqlSchemaGenerator sql_gen(schema, get_dialect());
    String sql;
    while (sql_gen.generate_next_statement(sql)) {
        auto_ptr<SqlCursor> cursor = get_conn()->new_cursor();
        if (ignore_errors) {
            try {
                cursor->exec_direct(sql);
            }
            catch (const DBError &) {
                // ignore
            }
        }
        else
            cursor->exec_direct(sql);
    }
}

void
EngineBase::drop_schema(const Schema &schema, bool ignore_errors)
{
    int max_depth = 0;
    Schema::TblMap::const_iterator i, iend;
    i = schema.tbl_begin(), iend = schema.tbl_end();
    for (; i != iend; ++i)
        if (i->second->get_depth() > max_depth)
            max_depth = i->second->get_depth();
    for (int curr_depth = max_depth; curr_depth >= 0; --curr_depth) {
        i = schema.tbl_begin(), iend = schema.tbl_end();
        for (; i != iend; ++i)
            if (i->second->get_depth() == curr_depth) {
                String sql = _T("DROP TABLE ") + i->second->name();
                auto_ptr<SqlCursor> cursor = get_conn()->new_cursor();
                if (ignore_errors) {
                    try {
                        cursor->exec_direct(sql);
                    }
                    catch (const DBError &e) {
                        logger()->warning(std::string("ignored DB error: ") + e.what());
                    }
                }
                else
                    cursor->exec_direct(sql);
            }
    }
    if (get_dialect()->has_sequences()) {
        i = schema.tbl_begin(), iend = schema.tbl_end();
        for (; i != iend; ++i)
            if (!str_empty(i->second->seq_name())) {
                String sql = get_dialect()->drop_sequence(i->second->seq_name());
                auto_ptr<SqlCursor> cursor = get_conn()->new_cursor();
                if (ignore_errors) {
                    try {
                        cursor->exec_direct(sql);
                    }
                    catch (const DBError &e) {
                        logger()->warning(std::string("ignored DB error: ") + e.what());
                    }
                }
                else
                    cursor->exec_direct(sql);
            }
    }
}

void
EngineBase::gen_sql_insert(String &sql, TypeCodes &type_codes_out,
        ParamNums &param_nums_out, const Table &table,
        bool include_pk, bool numbered_params)
{
    int count = 1, *pcount = NULL;
    if (numbered_params)
        pcount = &count;
    String sql_query;
    sql_query = _T("INSERT INTO ") + table.name() + _T(" (");
    TypeCodes type_codes;
    type_codes.reserve(table.size());
    ParamNums param_nums;
    Strings names, pholders;
    size_t i;
    for (i = 0; i < table.size(); ++i) {
        const Column &col = table[i];
        if ((!col.is_ro() || col.is_pk()) &&
                (!col.is_pk() || include_pk))
        {
            if (pcount)
                pholders.push_back(_T(":") + to_string(count));
            else
                pholders.push_back(_T("?"));
            ++count;
            param_nums[col.name()] = type_codes.size();
            type_codes.push_back(col.type());
            names.push_back(col.name());
        }
    }
    sql_query += ExpressionList(names).get_sql() + _T(") VALUES (") + 
        ExpressionList(pholders).get_sql() + _T(")");
    str_swap(sql, sql_query);
    type_codes_out.swap(type_codes);
    param_nums_out.swap(param_nums);
}

void
EngineBase::gen_sql_update(String &sql, TypeCodes &type_codes_out,
        ParamNums &param_nums_out, const Table &table,
        bool numbered_params)
{
    if (!table.pk_fields().size())
        throw BadSQLOperation(_T("cannot build update statement: no key in table"));
    int count = 1, *pcount = NULL;
    if (numbered_params)
        pcount = &count;
    String sql_query = _T("UPDATE ") + table.name() + _T(" SET ");
    TypeCodes type_codes;
    type_codes.reserve(table.size());
    ParamNums param_nums;
    size_t i;
    for (i = 0; i < table.size(); ++i) {
        const Column &col = table[i];
        if (!col.is_pk() && !col.is_ro()) {
            if (!type_codes.empty())
                sql_query += _T(", ");
            sql_query += col.name() + _T(" = ");
            if (pcount)
                sql_query += _T(":") + to_string(count);
            else
                sql_query += _T("?");
            ++count;
            param_nums[col.name()] = type_codes.size();
            type_codes.push_back(col.type());
        }
    }
    for (i = 0; i < table.pk_fields().size(); ++i) {
        const String &col_name = table.pk_fields()[i];
        param_nums[col_name] = type_codes.size();
        type_codes.push_back(table[col_name].type());
    }
    type_codes_out.swap(type_codes);
    param_nums_out.swap(param_nums);
    Key sample_key;
    table.mk_sample_key(type_codes, sample_key);
    Values sample_params;
    sql_query += _T(" WHERE ")
        + KeyFilter(sample_key).generate_sql(&sample_params, pcount);
    str_swap(sql, sql_query);
}

void
EngineBase::gen_sql_delete(String &sql, TypeCodes &type_codes_out,
        const Table &table, bool numbered_params)
{
    if (!table.pk_fields().size())
        throw BadSQLOperation(_T("cannot build update statement: no key in table"));
    int count = 1, *pcount = NULL;
    if (numbered_params)
        pcount = &count;
    String sql_query = _T("DELETE FROM ") + table.name();
    TypeCodes type_codes;
    Key sample_key;
    table.mk_sample_key(type_codes, sample_key);
    Values sample_params;
    sql_query += _T(" WHERE ")
        + KeyFilter(sample_key).generate_sql(&sample_params, pcount);
    type_codes_out.swap(type_codes);
    str_swap(sql, sql_query);
}

EngineCloned::~EngineCloned()
{
    if (pool_)
        pool_->put(conn_);
}

int EngineCloned::get_mode() { return mode_; }
SqlConnection *EngineCloned::get_conn() { return conn_; }
SqlDialect *EngineCloned::get_dialect() { return dialect_; }
ILogger *EngineCloned::logger() { return logger_; }

const String env_cfg(const String &entry, const String &def_val)
{
    String value = xgetenv(_T("YBORM_") + entry);
    if (str_empty(value))
        return def_val;
    return value;
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

Engine::Engine(int mode)
    : echo_(false)
    , mode_(mode)
    , timeout_(0)
    , conn_(new SqlConnection(sql_source_from_env()))
    , dialect_(conn_->get_dialect())
    , conn_ptr_(NULL)
{}

Engine::Engine(int mode, auto_ptr<SqlConnection> conn)
    : echo_(false)
    , mode_(mode)
    , timeout_(0)
    , conn_(conn)
    , dialect_(conn_->get_dialect())
    , conn_ptr_(NULL)
{}

Engine::Engine(int mode, auto_ptr<SqlPool> pool,
               const String &source_id, int timeout)
    : echo_(false)
    , mode_(mode)
    , pool_(pool)
    , source_id_(source_id)
    , timeout_(timeout)
    , dialect_(NULL)
    , conn_ptr_(NULL)
{}

Engine::~Engine()
{
    if (conn_ptr_ && pool_.get())
        pool_->put(conn_ptr_);
}

int Engine::get_mode() { return mode_; }

SqlConnection *Engine::get_conn()
{
    if (conn_.get())
        return conn_.get();
    if (conn_ptr_)
        return conn_ptr_;
    conn_ptr_ = get_from_pool();
    return conn_ptr_;
}

SqlDialect *Engine::get_dialect() { return dialect_; }
ILogger *Engine::logger() { return logger_.get(); }

auto_ptr<EngineBase> Engine::clone()
{
    if (conn_.get())
        return auto_ptr<EngineBase>(new EngineCloned(
                    mode_, conn_.get(), dialect_, logger_.get()));
    SqlConnection *conn = get_from_pool();
    return auto_ptr<EngineBase>(new EngineCloned(
                mode_, conn, dialect_, logger_.get(), pool_.get()));
}

SqlConnection *Engine::get_from_pool()
{
    if (!pool_.get())
        throw GenericDBError(_T("Engine with no connection"));
    SqlConnection *conn = pool_->get(source_id_, timeout_);
    if (!conn)
        throw GenericDBError(_T("Can't get connection"));
    dialect_ = conn->get_dialect();
    conn->set_echo(echo_);
    conn->init_logger(logger_.get());
    return conn;
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
