#include "MetaDataSingleton.h"
#include "MapperSession.h"

using namespace std;

namespace Yb {

MapperSession::MapperSession(bool read_only)
    : Session(read_only ? Session::READ_ONLY : Session::MANUAL,
        "MYSQL" /* FIX: fake param */)
    , session_(read_only ? Session::READ_ONLY : Session::MANUAL)
    , ds_(theMetaData::instance(), session_)
    , mapper_(theMetaData::instance(), ds_)
{}

// mapper interface methods:
// methods are just proxied

RowData *
MapperSession::find(const RowData &key)
{
    return mapper_.find(key);
}

LoadedRows
MapperSession::load_collection(
        const string &table_name, const Filter &filter, const StrList &order_by, int max,
        const string &table_alias)
{
    return mapper_.load_collection(table_name, filter, order_by, max, table_alias);
}

RowData *
MapperSession::create(const string &table_name)
{
    return mapper_.create(table_name);
}

RowData *
MapperSession::register_as_new(const RowData &row)
{
    return mapper_.register_as_new(row);
}

void
MapperSession::flush()
{
    mapper_.flush();
}

const TableMetaDataRegistry &
MapperSession::get_meta_data_registry()
{
    return mapper_.get_meta_data_registry();
}

// session policy methods:
// low level access requested, always mapper_.flush() before proceeding

RowsPtr
MapperSession::on_select(const StrList &what,
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
MapperSession::on_insert(const string &table_name,
        const Rows &rows, const FieldSet &exclude_fields,
        bool collect_new_ids)
{
    mapper_.flush();
    return session_.insert(table_name, rows, exclude_fields,
            collect_new_ids);
}

void
MapperSession::on_update(const string &table_name,
        const Rows &rows, const FieldSet &key_fields,
        const FieldSet &exclude_fields, const Filter &where)
{
    mapper_.flush();
    session_.update(table_name, rows, key_fields, exclude_fields, where);
}

void
MapperSession::on_delete(const string &table_name, const Filter &where)
{
    mapper_.flush();
    session_.delete_from(table_name, where);
}

void
MapperSession::on_exec_proc(const string &proc_code)
{
    mapper_.flush();
    session_.exec_proc(proc_code);
}

void
MapperSession::on_commit()
{
    mapper_.flush();
    session_.commit();
}

void
MapperSession::on_rollback()
{
    mapper_.flush();
    session_.rollback();
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
