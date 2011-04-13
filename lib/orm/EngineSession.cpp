#include "MetaDataSingleton.h"
#include "EngineSession.h"

using namespace std;

namespace Yb {

EngineSession::EngineSession(bool read_only)
    : EngineBase(read_only ? EngineBase::READ_ONLY : EngineBase::MANUAL,
        "MYSQL" /* FIX: fake param */)
    , engine_(read_only ? EngineBase::READ_ONLY : EngineBase::MANUAL)
    , ds_(theMetaData::instance(), engine_)
    , session_(theMetaData::instance(), ds_)
{}

// session interface methods:
// methods are just proxied

RowData *
EngineSession::find(const RowData &key)
{
    return session_.find(key);
}

LoadedRows
EngineSession::load_collection(
        const string &table_name, const Filter &filter, const StrList &order_by, int max,
        const string &table_alias)
{
    return session_.load_collection(table_name, filter, order_by, max, table_alias);
}

RowData *
EngineSession::create(const string &table_name)
{
    return session_.create(table_name);
}

RowData *
EngineSession::register_as_new(const RowData &row)
{
    return session_.register_as_new(row);
}

void
EngineSession::flush()
{
    session_.flush();
}

const TableMetaDataRegistry &
EngineSession::get_meta_data_registry()
{
    return session_.get_meta_data_registry();
}

// engine policy methods:
// low level access requested, always session_.flush() before proceeding

RowsPtr
EngineSession::on_select(const StrList &what,
        const StrList &from, const Filter &where,
        const StrList &group_by, const Filter &having,
        const StrList &order_by, int max_rows,
        bool for_update)
{
    session_.flush();
    return engine_.select(what, from, where,
            group_by, having, order_by, max_rows, for_update);
}

const vector<LongInt>
EngineSession::on_insert(const string &table_name,
        const Rows &rows, const FieldSet &exclude_fields,
        bool collect_new_ids)
{
    session_.flush();
    return engine_.insert(table_name, rows, exclude_fields,
            collect_new_ids);
}

void
EngineSession::on_update(const string &table_name,
        const Rows &rows, const FieldSet &key_fields,
        const FieldSet &exclude_fields, const Filter &where)
{
    session_.flush();
    engine_.update(table_name, rows, key_fields, exclude_fields, where);
}

void
EngineSession::on_delete(const string &table_name, const Filter &where)
{
    session_.flush();
    engine_.delete_from(table_name, where);
}

void
EngineSession::on_exec_proc(const string &proc_code)
{
    session_.flush();
    engine_.exec_proc(proc_code);
}

void
EngineSession::on_commit()
{
    session_.flush();
    engine_.commit();
}

void
EngineSession::on_rollback()
{
    session_.flush();
    engine_.rollback();
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
