#include "Session.h"

using namespace std;

namespace Yb {

SessionBase::~SessionBase()
{}

Session::Session(const TableMetaDataRegistry &reg, DataSource &ds)
    : reg_(reg)
    , ds_(ds)
    , new_id_(1)
{}

RowData *
Session::find(const RowData &key)
{
    RowSet::iterator it = rows_.find(key);
    if (it != rows_.end())
        return it->second.get();
    auto_ptr<RowData> x(new RowData(key));
    x->set_data_source(&ds_);
    x->set_ghost();
    rows_[key] = boost::shared_ptr<RowData>(x.release());
    it = rows_.find(key);
    if (it != rows_.end())
        return it->second.get();
    return NULL;
}

LoadedRows
Session::load_collection(
        const string &table_name, const Filter &filter, const StrList &order_by,
        int max, const string &table_alias)
{
    RowDataVectorPtr vp = ds_.select_rows(table_name, filter, order_by, max, table_alias);
    LoadedRows ld(new vector<RowData * > () );
    RowDataVector::const_iterator it = vp->begin(), end = vp->end();
    for (; it != end; ++it) {
        RowData *d = find(*it);
        *d = *it;
        d->set_sync();
        ld->push_back(d);
    }
    return ld;
}

RowData *
Session::create(const string &table_name)
{
    RowData key(reg_, table_name);
    const TableMetaData &t = key.get_table();
    key.set_new_pk(create_temp_pkid(t));
    RowData *d = find(key);
    d->set_new(); // TODO: iff !is_sync()
    tables_for_insert_.insert(t.get_name());
    return d;
}

RowData *
Session::register_as_new(const RowData &row)
{
    RowData *d = find(row);
    *d = row;
    d->set_new(); // TODO: iff !is_sync()
    tables_for_insert_.insert(d->get_table().get_name());
    return d;
}

void
Session::flush()
{
    flush_new();
    // update
    // TODO: sort by table
    RowSet::iterator it = rows_.begin(), end = rows_.end();
    for (; it != end; ++it) {
        if (it->second->is_dirty()) {
            RowDataVector v;
            v.push_back(*it->second);
            ds_.update_rows(it->second->get_table().get_name(), v);
            it->second->set_ghost();
        }
    }
    it = rows_.begin(), end = rows_.end();
    for (; it != end; ) {
        if (it->second->is_deleted()) {
            ds_.delete_row(*it->second);
            rows_.erase(it++);
        }
        else {
            ++it;
        }
    }
}

const TableMetaDataRegistry &
Session::get_meta_data_registry()
{
    return reg_;
}

void
Session::flush_new()
{
    list<string> ordered_ins_tables;
    sort_tables(tables_for_insert_, ordered_ins_tables);
    tables_for_insert_.clear();
    list<string>::const_iterator it = ordered_ins_tables.begin(),
        end = ordered_ins_tables.end();
    for (; it != end; ++it)
        insert_new_to_table(*it);
    mark_new_as_ghost();
    vector<boost::shared_ptr<PKIDRecord> > new_ids0;
    new_ids_.swap(new_ids0);
}

void
Session::insert_new_to_table(const string &table_name)
{
    const TableMetaData &table = reg_.get_table(table_name);
    string pk_name = table.find_synth_pk();
    RowSet::iterator it = rows_.begin(), end = rows_.end();
    vector<boost::shared_ptr<RowData> > pkid_changed;
    RowDataVector v;
    for (; it != end;) {
        if (it->second->is_new() &&
                it->second->get_table().get_name() == table_name)
        {
            v.push_back(*it->second);
            if (!pk_name.empty()) {
                pkid_changed.push_back(it->second);
                rows_.erase(it++);
            }
            else
                ++it;
        }
        else
            ++it;
    }
    ds_.insert_rows(table_name, v);
    for (int j = 0; j < pkid_changed.size(); ++j) {
        PKIDValue pkid(table,
                pkid_changed[j]->get(pk_name).as_pkid().as_longint());
        pkid_changed[j]->set(pk_name, pkid);
        rows_[*pkid_changed[j]] = pkid_changed[j];
    }
}

void
Session::mark_new_as_ghost()
{
    RowSet::iterator it = rows_.begin(), end = rows_.end();
    for (; it != end; ++it)
        if (it->second->is_new())
            it->second->set_ghost();
}

void
Session::get_unique_tables_for_insert(set<string> &unique_tables)
{
    set<string> ut;
    RowSet::const_iterator it = rows_.begin(), end = rows_.end();
    for (; it != end; ++it)
        if (it->second->is_new())
            ut.insert(it->first.get_table().get_name());
    ut.swap(unique_tables);
}

void
Session::sort_tables(const set<string> &unique_tabs,
        list<string> &ordered_tables)
{
    typedef multimap<int, string> IntStrMap;
    IntStrMap order;
    {
        set<string>::const_iterator it = unique_tabs.begin(), end = unique_tabs.end();
        for (; it != end; ++it)
            order.insert(IntStrMap::value_type(reg_.get_table(*it).get_depth(), *it));
    }
    
    IntStrMap::const_iterator it = order.begin(), end = order.end();
    for (; it != end; ++it)
        ordered_tables.push_back(it->second);
}

boost::shared_ptr<PKIDRecord>
Session::create_temp_pkid(const TableMetaData &table)
{
    new_ids_.push_back(boost::shared_ptr<PKIDRecord>(
                new PKIDRecord(&table, new_id_++, true)));
    return new_ids_[new_ids_.size() - 1];
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
