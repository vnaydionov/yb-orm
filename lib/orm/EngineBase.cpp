#include "EngineBase.h"

using namespace std;

namespace Yb {

DBError::DBError(const string &msg)
    : logic_error(msg)
{}

GenericDBError::GenericDBError(const string &err)
    : DBError("Database error, details: " + err)
{}

NoDataFound::NoDataFound(const string &msg)
    : DBError("Data wasn't found, details: " + msg)
{}

BadSQLOperation::BadSQLOperation(const string &msg)
    : DBError(msg)
{}

BadOperationInMode::BadOperationInMode(const string &msg)
    : DBError(msg)
{}

SqlDialectError::SqlDialectError(const string &msg)
    : DBError(msg)
{}

SqlDialect::~SqlDialect()
{}

class OracleDialect: public SqlDialect
{
public:
    const string get_name() { return "ORACLE"; }
    bool has_sequences() { return true; }
    const string select_curr_value(const string &seq_name)
    { return seq_name + ".CURRVAL"; }
    const string select_next_value(const string &seq_name)
    { return seq_name + ".NEXTVAL"; }
    const string dual_name()
    { return "DUAL"; }
};

class InterbaseDialect: public SqlDialect
{
public:
    const string get_name() { return "INTERBASE"; }
    bool has_sequences() { return true; }
    const string select_curr_value(const string &seq_name)
    { return "GEN_ID(" + seq_name + ", 0)"; }
    const string select_next_value(const string &seq_name)
    { return "GEN_ID(" + seq_name + ", 1)"; }
    const string dual_name()
    { return "RDB$DATABASE"; }
};

class MysqlDialect: public SqlDialect
{
public:
    const string get_name() { return "MYSQL"; }
    bool has_sequences() { return false; }
    const string select_curr_value(const string &seq_name)
    { throw SqlDialectError("No sequences, please"); }
    const string select_next_value(const string &seq_name)
    { throw SqlDialectError("No sequences, please"); }
    const string dual_name()
    { return "DUAL"; }
};

SqlDialect *mk_dialect(const string &name)
{
    auto_ptr<SqlDialect> d(
        !name.compare("ORACLE")? (SqlDialect *)new OracleDialect():
        !name.compare("INTERBASE")? (SqlDialect *)new InterbaseDialect():
        !name.compare("MYSQL")? (SqlDialect *)new MysqlDialect():
        NULL);
    if (!d.get())
        throw SqlDialectError("Unknown dialect: " + name);
    return d.release();
}

EngineBase::EngineBase(mode work_mode, const string &dialect_name)
    : touched_(false)
    , mode_(work_mode)
    , dialect_(mk_dialect(dialect_name))
{}

EngineBase::~EngineBase()
{}

RowsPtr
EngineBase::select(const StrList &what,
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

const vector<LongInt>
EngineBase::insert(const string &table_name,
        const Rows &rows, const FieldSet &exclude_fields,
        bool collect_new_ids)
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode("Using INSERT operation in read-only mode");
    touched_ = true;
    return on_insert(table_name, rows, exclude_fields, collect_new_ids);
}

void
EngineBase::update(const string &table_name,
        const Rows &rows, const FieldSet &key_fields,
        const FieldSet &exclude_fields, const Filter &where)
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode("Using UPDATE operation in read-only mode");
    on_update(table_name, rows, key_fields, exclude_fields, where);
    touched_ = true;
}

void
EngineBase::delete_from(const string &table_name, const Filter &where)
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode("Using DELETE operation in read-only mode");
    on_delete(table_name, where);
    touched_ = true;
}

void
EngineBase::exec_proc(const string &proc_code)
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode("Trying to invoke a PROCEDURE in read-only mode");
    on_exec_proc(proc_code);
    touched_ = true;
}

void
EngineBase::commit()
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode("Using COMMIT operation in read-only mode");	
    on_commit();
    touched_ = false;
}

void
EngineBase::rollback()
{
    if (mode_ == READ_ONLY)
        throw BadOperationInMode("Using ROLLBACK operation in read-only mode");	
    on_rollback();
    touched_ = false;
}

RowPtr
EngineBase::select_row(const StrList &what, const StrList &from, const Filter &where)
{
    RowsPtr rows = select(what, from, where);
    if (rows->size() != 1)
        throw NoDataFound("Unable to fetch exactly one row!");
    RowPtr row(new Row((*rows)[0]));
    return row;
}

RowPtr
EngineBase::select_row(const StrList &from, const Filter &where)
{
    return select_row("*", from, where);
}

const Value
EngineBase::select1(const string &what, const string &from, const Filter &where)
{
    RowPtr row(select_row(what, from, where));
    if (row->size() != 1)
        throw BadSQLOperation("Unable to fetch exactly one column!");
    return row->begin()->second;
}

LongInt
EngineBase::get_curr_value(const string &seq_name)
{
    return select1(dialect_->select_curr_value(seq_name),
            dialect_->dual_name(), Filter()).as_longint();
}

LongInt
EngineBase::get_next_value(const string &seq_name)
{
    return select1(dialect_->select_next_value(seq_name),
            dialect_->dual_name(), Filter()).as_longint();
}

const DateTime
EngineBase::fix_dt_hook(const DateTime &t)
{
    // by default do nothing:
    return t;
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
