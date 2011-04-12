#include <sstream>
#include <util/str_utils.hpp>
#include "MetaData.h"
#include "Value.h"

using namespace std;
using namespace Yb::StrUtils;

namespace Yb {

MetaDataError::MetaDataError(const string &msg)
    : logic_error(msg)
{}

BadColumnName::BadColumnName(const string &table, const string &column)
    : MetaDataError("Bad column name '" + column
            + "' while constructing metadata '" + table + "'")
{}

ColumnNotFoundInMetaData::ColumnNotFoundInMetaData(const string &table,
        const string &column)
    : MetaDataError("Column '" + column + "' not found in metadata '"
            + table + "'")
{}

TableWithoutColumns::TableWithoutColumns(const string &table)
    : MetaDataError("Table '" + table + "' has no columns in metadata")
{}

BadTableName::BadTableName(const string &table)
    : MetaDataError("Bad table name '" + table + "'")
{}

TableNotFoundInMetaData::TableNotFoundInMetaData(const string &table)
    : MetaDataError("Table '" + table + "' not found in metadata")
{}

RowNotLinkedToTable::RowNotLinkedToTable()
    : MetaDataError("RowData object is not linked to any table")
{}

ReadOnlyColumn::ReadOnlyColumn(const string &table, const string &column)
    : MetaDataError("Column '" + column + "' in table '" + table + "' is READ-ONLY")
{}

NotSuitableForAutoCreating::NotSuitableForAutoCreating(const string &table)
    : MetaDataError("Table '" + table + "' is not suitable for auto creating its rows")
{}


static
char
underscore_to_dash(char c) { return c == '_'? '-': c; }

const string
mk_xml_name(const string &name, const string &xml_name)
{
    if (xml_name == "-")
        return "";
    if (!xml_name.empty())
        return xml_name;
    return translate(str_to_lower(name), underscore_to_dash);
}

ColumnMetaData::ColumnMetaData(const string &name, int type, size_t size, int flags,
        const string &fk_table, const string &fk, const string &xml_name)
    : name_(str_to_upper(name))
    , type_(type)
    , size_(size)
    , flags_(flags)
    , fk_table_name_(str_to_upper(fk_table))
    , fk_name_(str_to_upper(fk))
    , xml_name_(mk_xml_name(name, xml_name))
{}

TableMetaData::TableMetaData(const string &name, const string &xml_name)
    : name_(str_to_upper(name))
    , xml_name_(mk_xml_name(name, xml_name))
    , autoinc_(false)
    , depth_(0)
{}

const string 
TableMetaData::get_unique_pk() const
{
    Map::const_iterator it = cols_.begin(), end = cols_.end();
    string key_field;
    for (; it != end; ++it) {
        if (it->second.is_pk())
            if (key_field.empty())
                key_field = it->second.get_name();
            else
                throw AmbiguousPK(get_name()); 
    }
    return key_field;
}

void
TableMetaData::set_column(const ColumnMetaData &column)
{
    if (!is_id(column.get_name()))
        throw BadColumnName(get_name(), column.get_name());
    cols_[column.get_name()] = column;
}

const ColumnMetaData &
TableMetaData::get_column(const string &name) const
{
    string good_name = str_to_upper(name);
    Map::const_iterator it = cols_.find(good_name);
    if (it == cols_.end())
        throw ColumnNotFoundInMetaData(get_name(), good_name);
    return it->second;
}

void
TableMetaData::set_seq_name(const string &seq_name)
{
    seq_name_ = str_to_upper(seq_name);
}

const string
TableMetaData::find_synth_pk() const
{
    // This call assumes that we have a table with
    // a synth. numeric primary key.
    if (get_seq_name().empty() && !get_autoinc())
        return string();
    string pk_name;
    Map::const_iterator it = begin(), e = end();
    for (; it != e; ++it) {
        if (it->second.is_pk()) {
            if (!pk_name.empty() ||
                    it->second.get_type() != Value::LONGINT)
            {
                return string();
            }
            pk_name = it->second.get_name();
        }
    }
    return pk_name;
}

const string
TableMetaData::get_synth_pk() const
{
    // This call assumes that we have a table with
    // a synth. numeric primary key.
    string pk_name = find_synth_pk();
    if (pk_name.empty())
        throw NotSuitableForAutoCreating(get_name());
    return pk_name;
}

const string
TableMetaData::get_seq_name() const
{
    return seq_name_;
}

bool
TableMetaData::get_autoinc() const
{
    return autoinc_;
}

void
TableMetaDataRegistry::set_table(const TableMetaData &table_meta_data)
{
    string good_name = str_to_upper(
            table_meta_data.get_name());
    if (!is_id(good_name))
        throw BadTableName(good_name);
    if (table_meta_data.size() == 0)
        throw TableWithoutColumns(good_name);
    tables_[table_meta_data.get_name()] = table_meta_data;
}

const TableMetaData &
TableMetaDataRegistry::get_table(const string &name) const
{
    string good_name = str_to_upper(name);
    Map::const_iterator it = tables_.find(good_name);
    if (it == tables_.end())
        throw TableNotFoundInMetaData(good_name);
    return it->second;
}

void TableMetaDataRegistry::check()
{
    set<string> unique_tables;
    fill_unique_tables(unique_tables);
    StrMap tree;
    map<string, int> depths;
    zero_depths(unique_tables, depths);
    fill_map_tree_by_meta(unique_tables, tree);
    traverse_children(tree, depths);
    set_absolute_depths(depths);
}

void
TableMetaDataRegistry::set_absolute_depths(const map<string, int> &depths)
{
    map<string, int>::const_iterator it = depths.begin(), end = depths.end();
    for (; it != end; ++it)
        tables_[it->first].set_depth(it->second);    
}

void
TableMetaDataRegistry::fill_unique_tables(set<string> &unique_tables)
{
    Map::const_iterator it = tables_.begin(), end = tables_.end();
    for (; it != end; ++it)
        unique_tables.insert(it->first);
}

void
TableMetaDataRegistry::fill_map_tree_by_meta(const set<string> &unique_tables, StrMap &tree_map)
{
    set<string>::const_iterator it = unique_tables.begin(), end = unique_tables.end();
    for (; it != end; ++it) {
        const TableMetaData &t = get_table(*it);
        TableMetaData::Map::const_iterator it_col = t.begin(), end_col = t.end();
        bool has_parent = false;
        for (; it_col != end_col; ++it_col) {
            // if found a foreign key table in the set, add it with this dependent table
            if (it_col->second.has_fk()) {
                string fk_field = it_col->second.get_fk_name();
                string fk_table = it_col->second.get_fk_table_name(); 
                CheckForeignKey(t.get_name(), fk_table, fk_field);
                tree_map.insert(StrMap::value_type(it_col->second.get_fk_table_name(), t.get_name()));
                has_parent = true;
            }
            if (!has_parent)
                tree_map.insert(StrMap::value_type("", t.get_name()));
        }
    }
    // little hack, if no a root found(parent '' in map), the tree contains cycle
    if (tree_map.find("") == tree_map.end())
        throw IntegrityCheckFailed("Cyclic references in DB scheme found");
}

void
TableMetaDataRegistry::CheckForeignKey(const string &table, const string &fk_table, const string &fk_field)
{
    if(tables_.find(fk_table) == tables_.end())
        throw IntegrityCheckFailed(string("Table '") + fk_table +
                "' not found as foreign key for '" + table + "'");
    
    try {
        tables_[fk_table].get_column(fk_field);
    }
    catch (ColumnNotFoundInMetaData &) {
        throw IntegrityCheckFailed(string("Field '") + fk_field + " of table '"
                + fk_table +
                "' not found as foreign key-field' of table '" + table + "'");
    }
}

void
TableMetaDataRegistry::zero_depths(const set<string> &unique_tables, map<string, int> &depths)
{
    map<string, int> new_depths;
    set<string>::const_iterator it = unique_tables.begin(), end = unique_tables.end();
    for (; it != end; ++it)
        new_depths[*it] = 0;
    new_depths.swap(depths);
}

void
TableMetaDataRegistry::traverse_children(const StrMap &parent_child, map<string, int> &depths)
{
    list<string> children;
    children.push_back("");
    while (!children.empty()) {
        pair<StrMap::const_iterator, StrMap::const_iterator> range =
            parent_child.equal_range(*children.begin());
        for (; range.first != range.second; ++range.first) {
            const string &parent = range.first->first;
            const string &child = range.first->second;
            if (std::find(children.begin(), children.end(), child) == children.end()) {
                children.push_back(child);
                int new_depth = (parent.empty()? 0: depths[parent]) + 1;
                if (depths[child] < new_depth)
                    depths[child] = new_depth;
                if (new_depth > parent_child.size())
                    throw IntegrityCheckFailed("Cyclic references in DB scheme found");
            }
        }
        children.erase(children.begin());
    }
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
