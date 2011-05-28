#include "SqlDataSource.h"
#include "Session.h"

using namespace std;

namespace Yb {

SqlDataSource::SqlDataSource(const Schema &reg,
        Engine &engine)
    : reg_(reg)
    , engine_(engine)
{}

RowDataPtr
SqlDataSource::select_row(const RowData &key)
{
    RowDataVectorPtr vp = select_rows(
            key.get_table().get_name(), FilterByKey(key));
    if (!vp.get() || vp->size() != 1)
        throw ObjectNotFoundByKey("Can't fetch exactly one object.");
    RowDataPtr p(new RowData((*vp)[0]));
    return p;
}

const RowData
SqlDataSource::sql_row2row_data(const string &table_name, const Row &row)
{
    RowData d(reg_, table_name);
    const Table &table = reg_.get_table(table_name);
    Columns::const_iterator it = table.begin(), end = table.end();
    for (; it != end; ++it) {
        Row::const_iterator x = row.find(it->get_name());
        if (x == row.end())
            throw FieldNotFoundInFetchedRow(table_name, it->get_name());
        d.set(it->get_name(), x->second);
    }
    return d;
}

RowPtr
SqlDataSource::row_data2sql_row(const RowData &rd)
{
    RowPtr row(new Row());
    const Table &table = rd.get_table();
    Columns::const_iterator it = table.begin(), end = table.end();
    for (; it != end; ++it)
        (*row)[it->get_name()] = rd.get(it->get_name());
    return row;
}

RowsPtr
SqlDataSource::row_data_vector2sql_rows(const Table &table,
        const RowDataVector &rows, int filter)
{
    string seq_name;
    string pk_name = table.find_synth_pk();
    if (!pk_name.empty() && engine_.get_dialect()->has_sequences())
        seq_name = table.get_seq_name();
    RowsPtr sql_rows(new Rows());
    RowDataVector::const_iterator it = rows.begin(), end = rows.end();
    for (; it != end; ++it) {
        if (table.get_name() != it->get_table().get_name())
            throw TableDoesNotMatchRow(table.get_name(),
                    it->get_table().get_name());
        if (!seq_name.empty()) {
            PKIDValue pkid = it->get(pk_name).as_pkid();
            if (pkid.is_temp())
                pkid.sync(engine_.get_next_value(seq_name));
        }
        if (0 == filter || 1 == filter) {
            bool is_temp = false;
            if (!pk_name.empty()) {
                PKIDValue pkid = it->get(pk_name).as_pkid();
                is_temp = pkid.is_temp();
            }
            if (int(is_temp) != filter)
                continue;
        }
        RowPtr sql_row = row_data2sql_row(*it);
        sql_rows->push_back(*sql_row);
    }
    return sql_rows;
}

RowDataVectorPtr
SqlDataSource::select_rows(
        const string &table_name, const Filter &filter, const StrList &order_by, int max,
        const string &table_alias)
{
    RowDataVectorPtr vp(new RowDataVector);
    RowsPtr rows = engine_.select("*", table_name, filter, StrList(), Filter(), order_by, max);
    if (rows.get()) {
        Rows::const_iterator it = rows->begin(), end = rows->end();
        for (; it != end; ++it)
            vp->push_back(sql_row2row_data(table_alias.empty()? table_name: table_alias, *it));
    }
    return vp;
}

void
SqlDataSource::do_insert_rows(const string &table_name,
        const RowDataVector &rows, bool process_autoinc)
{
    const Table &table = reg_.get_table(table_name);
    RowsPtr sql_rows = row_data_vector2sql_rows(table, rows,
            process_autoinc? 1: 0);
    if (!sql_rows->size())
        return;
    string pk_name = table.find_synth_pk();
    FieldSet excluded;
    Columns::const_iterator it = table.begin(), end = table.end();
    for (; it != end; ++it)
        if ((it->is_ro() && !it->is_pk()) ||
                (it->get_name() == pk_name && process_autoinc))
            excluded.insert(it->get_name());
    vector<LongInt> new_ids = engine_.insert(
            table_name, *sql_rows, excluded, process_autoinc);
    if (process_autoinc) {
        if (new_ids.size() != sql_rows->size())
            throw NoDataFound("Can't fetch auto-inserted IDs");
        for (unsigned i = 0; i < sql_rows->size(); ++i) {
            PKIDValue pkid = (*sql_rows)[i][pk_name].as_pkid();
            pkid.sync(new_ids[i]);
        }
    }
}

void
SqlDataSource::insert_rows(const string &table_name,
        const RowDataVector &rows)
{
    const Table &table = reg_.get_table(table_name);
    do_insert_rows(table_name, rows, false);
    if (!engine_.get_dialect()->has_sequences() &&
            (!table.get_seq_name().empty() || table.get_autoinc()))
        do_insert_rows(table_name, rows, true);
}

void
SqlDataSource::update_rows(const string &table_name,
        const RowDataVector &rows)
{
    const Table &table = reg_.get_table(table_name);
    RowsPtr sql_rows = row_data_vector2sql_rows(table, rows);
    FieldSet excluded, keys;
    Columns::const_iterator it = table.begin(), end = table.end();
    for (; it != end; ++it) {
        if (it->is_pk())
            keys.insert(it->get_name());
        if (it->is_ro() && !it->is_pk())
            excluded.insert(it->get_name());
    }
    engine_.update(table_name, *sql_rows, keys, excluded);
}

void
SqlDataSource::delete_row(const RowData & /* row */)
{
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
