
#include "Session.h"

using namespace std;

namespace Yb {
namespace SQL {
    
DBError::DBError(const string &msg)
    : logic_error(msg)
{}

GenericDBError::GenericDBError(const string &err)
    : DBError("Database error, details: " + err)
{}

NoDataFound::NoDataFound(const std::string &msg)
    : DBError("Data wasn't found, details: " + msg)
{}

BadSQLOperation::BadSQLOperation(const string &msg)
    : DBError(msg)
{}

BadOperationInMode::BadOperationInMode(const string &msg)
    : DBError(msg)
{}

Session::Session(mode work_mode)
    : touched_(false)
    , mode_(work_mode)
{}

Session::~Session()
{}

RowsPtr
Session::select(const StrList &what,
        const StrList &from, const Filter &where,
        const StrList &group_by, const Filter &having,
        const StrList &order_by, int max_rows, bool for_update)
{
    bool select_mode = (mode_ == FORCE_SELECT_UPDATE) ? true : for_update;
    if ((mode_ == READ_ONLY) && select_mode)
        throw BadOperationInMode("Using SELECT FOR UPDATE in read-only mode");  
    RowsPtr rows(on_select(what, from, where,
                group_by, having, order_by, max_rows, select_mode));
    if (select_mode)
        touched_ = true;
    return rows;
}

const std::vector<long long>
Session::insert(const string &table_name,
        const Rows &rows, const FieldSet &exclude_fields,
        bool collect_new_ids)
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode("Using INSERT operation in read-only mode");
    touched_ = true;
    return on_insert(table_name, rows, exclude_fields, collect_new_ids);
}

void
Session::update(const string &table_name,
        const Rows &rows, const FieldSet &key_fields,
        const FieldSet &exclude_fields, const Filter &where)
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode("Using UPDATE operation in read-only mode");
    on_update(table_name, rows, key_fields, exclude_fields, where);
    touched_ = true;
}

void
Session::delete_from(const string &table_name, const Filter &where)
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode("Using DELETE operation in read-only mode");
    on_delete(table_name, where);
    touched_ = true;
}

void
Session::exec_proc(const string &proc_code)
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode("Trying to invoke a PROCEDURE in read-only mode");
    on_exec_proc(proc_code);
    touched_ = true;
}

void
Session::commit()
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode("Using COMMIT operation in read-only mode");	
    on_commit();
    touched_ = false;
}

void
Session::rollback()
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode("Using ROLLBACK operation in read-only mode");	
    on_rollback();
    touched_ = false;
}

RowPtr
Session::select_row(const StrList &what, const StrList &from, const Filter &where)
{
    RowsPtr rows = select(what, from, where);
    if (rows->size() != 1)
        throw NoDataFound("Unable to fetch exactly one row!");
    RowPtr row(new Row((*rows)[0]));
    return row;
}

RowPtr
Session::select_row(const StrList &from, const Filter &where)
{
    return select_row("*", from, where);
}

const Value
Session::select1(const string &what, const string &from, const Filter &where)
{
    RowPtr row(select_row(what, from, where));
    if (row->size() != 1)
        throw BadSQLOperation("Unable to fetch exactly one column!");
    return row->begin()->second;
}

long long
Session::get_curr_value(const string &seq_name)
{
    return select1(seq_name + ".CURRVAL", "DUAL", Filter()).as_long_long();
}

long long
Session::get_next_value(const string &seq_name)
{
    return select1(seq_name + ".NEXTVAL", "DUAL", Filter()).as_long_long();
}

const boost::posix_time::ptime
Session::fix_dt_hook(const boost::posix_time::ptime &t)
{
    // by default do nothing:
    return t;
}

} // namespace SQL
} // namespace Yb

// vim:ts=4:sts=4:sw=4:et

