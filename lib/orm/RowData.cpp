#include <sstream>
#include <boost/lexical_cast.hpp>
#include "RowData.h"
#include "Session.h"

using namespace std;

namespace Yb {

ORMError::ORMError(const string &msg)
    : logic_error(msg)
{}

ObjectNotFoundByKey::ObjectNotFoundByKey(const string &msg)
    : ORMError(msg)
{}

FieldNotFoundInFetchedRow::FieldNotFoundInFetchedRow(
        const string &table_name, const string &field_name)
    : ORMError("Field not found in fetched row: " + table_name +
            "." + field_name)
{}

BadTypeCast::BadTypeCast(
        const string &table_name, const string &field_name,
        const string &str_value, const string &type)
    : ORMError("Can't cast field " + table_name + "." + field_name +
            " = \"" + str_value + "\" to type " + type)
{}

StringTooLong::StringTooLong(
        const string &table_name, const string &field_name,
        int max_len, const string &value)
    : ORMError("Can't set value of " + table_name + "." + field_name +
            " with '" + value + "', having max length " +
            boost::lexical_cast<string>(max_len))
{}

TableDoesNotMatchRow::TableDoesNotMatchRow(
        const string &table_name, const string &table_name_from_row)
    : ORMError("Table name " + table_name
            + " doesn't match table name from row "
            + table_name_from_row)
{}

DataSource::~DataSource()
{}

RowData::RowData()
    : table_(NULL)
    , ds_(NULL)
    , status_(Undef)
{}

RowData::RowData(const Schema &reg, const string &table_name)
    : table_(&reg.get_table(table_name))
    , ds_(NULL)
    , status_(Undef)
{
    values_.resize(table_->size());
}

const Table &
RowData::get_table() const
{
    if (!table_)
        throw RowNotLinkedToTable();
    return *table_;
}

const Value &
RowData::get(const string &column_name) const
{
    const Table &t = get_table();
    const Column &c = t.get_column(column_name);
    load_if_ghost_and_if_non_key_field_requested(c);
    return values_[t.idx_by_name(c.get_name())];
}

const PKIDValue
RowData::get_id() const
{
    const Table &t = get_table();
    string pk_name = t.get_synth_pk();
    return get(pk_name).as_pkid();
}

const Value
RowData::get_typed_value(const Column &c, const Value &value)
{
    Value x;
    try {
        x = value.get_typed_value(c.get_type());
    }
    catch (const Yb::ValueBadCast &) {
        throw BadTypeCast(get_table().get_name(), c.get_name(),
                value.as_string(), Value::get_type_name(c.get_type()));
    }
    if (!x.is_null() && c.get_type() == Value::STRING &&
            c.get_size() > 0 && x.as_string().size() > c.get_size())
    {
        throw StringTooLong(get_table().get_name(), c.get_name(),
                c.get_size(), x.as_string());
    }
    return x;
}

void
RowData::set(const string &column_name, const Value &value)
{
    const Table &t = get_table();
    const Column &c = t.get_column(column_name);
    load_if_ghost_and_if_non_key_field_requested(c);
    size_t idx = t.idx_by_name(c.get_name());
    if ((c.is_ro() || c.is_pk()) && !values_[idx].is_null()) {
        Value old = values_[idx];
        if (!(old.get_type() == Value::PKID &&
                    (old.as_pkid().is_temp() ||
                     old.as_pkid().as_longint() == value.as_longint())))
            throw ReadOnlyColumn(t.get_name(), c.get_name());
    }
    Value x(value);
    if (x.get_type() != Value::PKID)
        x = get_typed_value(c, x);
    bool update_status = true;
    if (c.is_pk() && x.get_type() != Value::PKID) {
        string pk_name = t.find_synth_pk();
        if (c.get_name() == pk_name) {
            if (!values_[idx].is_null())
            {
                PKIDValue pkid = values_[idx].as_pkid();
                if (pkid.is_temp())
                    pkid.sync(x.as_longint());
                update_status = false;
            }
            else
                x = Value(PKIDValue(t, x.as_longint()));
        }
    }
    values_[idx] = x;
    if (update_status && status_ == Sync)
        status_ = Dirty;
}

void
RowData::set_pk(LongInt pk)
{
    const Table &table = get_table();
    string pk_name = table.get_synth_pk();
    set(pk_name, pk);
}

void
RowData::set_new_pk(boost::shared_ptr<PKIDRecord> new_pk)
{
    const Table &t = get_table();
    string pk_name = t.get_synth_pk();
    Value pk_value(PKIDValue(t, new_pk));
    set(pk_name, pk_value);
}

bool
RowData::lt(const RowData &x, bool key_only) const
{
    if (!table_ && !x.table_)
        return false;
    if (!table_)
        return true;
    if (!x.table_)
        return false;
    if (table_->get_name() != x.table_->get_name())
        return table_->get_name() < x.table_->get_name();
    Columns::const_iterator it = table_->begin(), end = table_->end();
    for (; it != end; ++it) {
        if (!key_only || it->is_pk()) {
            const Value &a = get(it->get_name());
            const Value &b = x.get(it->get_name());
            if (a < b)
                return true;
            if (b < a)
                return false;
        }
    }
    return false;
}

void
RowData::load(void) const
{
    RowDataPtr t = ds_->select_row(*this);
    values_.swap(t->values_);
    status_ = Sync;
}

void
RowData::load_if_ghost_and_if_non_key_field_requested(const Column &c) const
{
    if (is_ghost() && ds_ && !c.is_pk())
        load();
}

const string
FilterBackendByKey::do_get_sql() const
{
    ostringstream sql;
    sql << "1=1";
    const Table &t = key_.get_table();
    Columns::const_iterator it = t.begin(), end = t.end();
    for (; it != end; ++it)
        if (it->is_pk()) {
            sql << " AND " << it->get_name() << " = "
                << key_.get(it->get_name()).sql_str();
        }
    return sql.str();
}

const string
FilterBackendByKey::do_collect_params_and_build_sql(Values &seq) const
{
    ostringstream sql;
    sql << "1=1";
    const Table &t = key_.get_table();
    Columns::const_iterator it = t.begin(), end = t.end();
    for (; it != end; ++it)
        if (it->is_pk()) {
            seq.push_back(key_.get(it->get_name()));
            sql << " AND " << it->get_name() << " = ?";
        }
    return sql.str();
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
