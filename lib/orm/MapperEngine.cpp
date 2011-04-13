#include "MetaDataSingleton.h"
#include "MapperEngine.h"

using namespace std;

namespace Yb {

MapperEngine::MapperEngine(bool read_only)
    : EngineBase(read_only ? EngineBase::READ_ONLY : EngineBase::MANUAL,
        "MYSQL" /* FIX: fake param */)
    , session_(read_only ? EngineBase::READ_ONLY : EngineBase::MANUAL)
    , ds_(theMetaData::instance(), session_)
    , mapper_(theMetaData::instance(), ds_)
{}

// mapper interface methods:
// methods are just proxied

RowData *
MapperEngine::find(const RowData &key)
{
    return mapper_.find(key);
}

LoadedRows
MapperEngine::load_collection(
        const string &table_name, const Filter &filter, const StrList &order_by, int max,
        const string &table_alias)
{
    return mapper_.load_collection(table_name, filter, order_by, max, table_alias);
}

RowData *
MapperEngine::create(const string &table_name)
{
    return mapper_.create(table_name);
}

RowData *
MapperEngine::register_as_new(const RowData &row)
{
    return mapper_.register_as_new(row);
}

void
MapperEngine::flush()
{
    mapper_.flush();
}

const TableMetaDataRegistry &
MapperEngine::get_meta_data_registry()
{
    return mapper_.get_meta_data_registry();
}

// engine policy methods:
// low level access requested, always mapper_.flush() before proceeding

RowsPtr
MapperEngine::on_select(const StrList &what,
        const StrList &from, const Filter &where,
        const StrList &group_by, const Filter &having,
        const StrList &order_by, int max_rows,
        bool for_update)
{
    mapper_.flush();
    return session_.select(what, from, where,
            group_by, having, order_by, max_rows, for_update);
}

const vector<LongInt>
MapperEngine::on_insert(const string &table_name,
        const Rows &rows, const FieldSet &exclude_fields,
        bool collect_new_ids)
{
    mapper_.flush();
    return session_.insert(table_name, rows, exclude_fields,
            collect_new_ids);
}

void
MapperEngine::on_update(const string &table_name,
        const Rows &rows, const FieldSet &key_fields,
        const FieldSet &exclude_fields, const Filter &where)
{
    mapper_.flush();
    session_.update(table_name, rows, key_fields, exclude_fields, where);
}

void
MapperEngine::on_delete(const string &table_name, const Filter &where)
{
    mapper_.flush();
    session_.delete_from(table_name, where);
}

void
MapperEngine::on_exec_proc(const string &proc_code)
{
    mapper_.flush();
    session_.exec_proc(proc_code);
}

void
MapperEngine::on_commit()
{
    mapper_.flush();
    session_.commit();
}

void
MapperEngine::on_rollback()
{
    mapper_.flush();
    session_.rollback();
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
