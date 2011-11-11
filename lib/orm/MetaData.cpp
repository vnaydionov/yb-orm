#include <sstream>
#include <util/str_utils.hpp>
#include <util/Exception.h>
#include <orm/MetaData.h>
#include <orm/Value.h>

using namespace std;
using namespace Yb::StrUtils;

namespace Yb {

MetaDataError::MetaDataError(const String &msg)
    : BaseError(msg)
{}

BadAttributeName::BadAttributeName(const String &obj, const String &attr)
    : MetaDataError(_T("Bad attribute name '") + attr + _T("' of object '") +
            obj + _T("'"))
{}

BadColumnName::BadColumnName(const String &table, const String &column)
    : MetaDataError(_T("Bad column name '") + column
            + _T("' while constructing metadata '") + table + _T("'"))
{}

ColumnNotFoundInMetaData::ColumnNotFoundInMetaData(const String &table,
        const String &column)
    : MetaDataError(_T("Column '") + column + _T("' not found in metadata '")
            + table + _T("'"))
{}

TableWithoutColumns::TableWithoutColumns(const String &table)
    : MetaDataError(_T("Table '") + table + _T("' has no columns in metadata"))
{}

BadTableName::BadTableName(const String &table)
    : MetaDataError(_T("Bad table name '") + table + _T("'"))
{}

TableNotFoundInMetaData::TableNotFoundInMetaData(const String &table)
    : MetaDataError(_T("Table '") + table + _T("' not found in metadata"))
{}

ClassNotFoundInMetaData::ClassNotFoundInMetaData(const String &class_name)
    : MetaDataError(_T("Class '") + class_name + _T("' not found in metadata"))
{}

RowNotLinkedToTable::RowNotLinkedToTable()
    : MetaDataError(_T("RowData object is not linked to any table"))
{}

ReadOnlyColumn::ReadOnlyColumn(const String &table, const String &column)
    : MetaDataError(_T("Column '") + column + _T("' in table '") + table + _T("' is READ-ONLY"))
{}

NotSuitableForAutoCreating::NotSuitableForAutoCreating(const String &table)
    : MetaDataError(_T("Table '") + table + _T("' is not suitable for auto creating its rows"))
{}


static
Char
underscore_to_dash(Char c) { return c == _T('_')? _T('-'): c; }

const String
mk_xml_name(const String &name, const String &xml_name)
{
    if (xml_name == _T("-"))
        return _T("");
    if (!xml_name.empty())
        return xml_name;
    return translate(str_to_lower(name), underscore_to_dash);
}

Column::Column(const String &name, int type, size_t size, int flags,
        const String &fk_table, const String &fk,
        const String &xml_name, const String &prop_name)
    : name_(str_to_upper(name))
    , type_(type)
    , size_(size)
    , flags_(flags)
    , fk_table_name_(str_to_upper(fk_table))
    , fk_name_(str_to_upper(fk))
    , xml_name_(mk_xml_name(name, xml_name))
    , prop_name_(prop_name.empty()? str_to_lower(name): prop_name)
    , table_(NULL)
{}

Table::Table(const String &name, const String &xml_name,
        const String &class_name)
    : name_(str_to_upper(name))
    , xml_name_(mk_xml_name(name, xml_name))
    , class_name_(class_name)
    , autoinc_(false)
    , depth_(0)
    , schema_(NULL)
{}

const String &
Table::get_unique_pk() const
{
    StringSet::const_iterator i = pk_fields_.begin();
    if (pk_fields_.size() != 1)
        throw AmbiguousPK(*++i);
    return *i;
}

void
Table::add_column(const Column &column)
{
    if (!is_id(column.get_name()))
        throw BadColumnName(get_name(), column.get_name());
    IndexMap::const_iterator it = indicies_.find(column.get_name());
    int idx = -1;
    if (it == indicies_.end()) {
        idx = cols_.size();
        cols_.push_back(column);
        indicies_[column.get_name()] = idx;
    }
    else {
        idx = it->second;
        cols_[idx] = column;
    }
    cols_[idx].table(*this);
    if (column.is_pk())
        pk_fields_.insert(column.get_name());
}

size_t
Table::idx_by_name(const String &col_name) const
{
    String fixed_name = str_to_upper(col_name);
    IndexMap::const_iterator it = indicies_.find(fixed_name);
    if (it == indicies_.end())
        throw ColumnNotFoundInMetaData(get_name(), fixed_name);
    return it->second;
}

void
Table::set_seq_name(const String &seq_name)
{
    seq_name_ = str_to_upper(seq_name);
}

const String
Table::find_synth_pk() const
{
    // This call assumes that we have a table with
    // a synth. numeric primary key.
    if (get_seq_name().empty() && !get_autoinc())
        return String();
    if (pk_fields_.size() != 1)
        return String();
    const Column &c = get_column(*pk_fields_.begin());
    if (c.get_type() != Value::LONGINT)
        return String();
    return c.get_name();
}

const String
Table::get_synth_pk() const
{
    // This call assumes that we have a table with
    // a synth. numeric primary key.
    String pk_name = find_synth_pk();
    if (pk_name.empty())
        throw NotSuitableForAutoCreating(get_name());
    return pk_name;
}

const String
Table::get_fk_for(const Relation *rel) const
{
    vector<String> parts;
    Columns::const_iterator i = begin(), iend = end();
    const String &master_tbl = rel->table(0);
    for (; i != iend; ++i)
        if (i->has_fk()
            && i->get_fk_table_name() == master_tbl
            && (!rel->has_attr(1, _T("key"))
                || rel->attr(1, _T("key")) == i->get_name()))
        {
            parts.push_back(i->get_name());
        }
    return StrUtils::join_str(_T(","), parts);
}

const String
Table::get_seq_name() const
{
    return seq_name_;
}

bool
Table::get_autoinc() const
{
    return autoinc_;
}

const Key
Table::mk_key(const ValueMap &key_fields) const
{
    return Key(name_, key_fields);
}

const Key
Table::mk_key(LongInt id) const
{
    ValueMap key_fields;
    key_fields[get_synth_pk()] = Value(id);
    return Key(name_, key_fields);
}

bool
Relation::has_attr(int n, const String &name) const {
    const AttrMap &a = !n? attr1_: attr2_;
    AttrMap::const_iterator it = a.find(name);
    return it != a.end();
}

const String &
Relation::attr(int n, const String &name) const {
    const AttrMap &a = !n? attr1_: attr2_;
    AttrMap::const_iterator it = a.find(name);
    if (it == a.end())
        throw BadAttributeName(_T("relation"), name);
    return it->second;
}

void
Schema::set_table(const Table &table_meta_data)
{
    String good_name = str_to_upper(
            table_meta_data.get_name());
    if (!is_id(good_name))
        throw BadTableName(good_name);
    if (table_meta_data.size() == 0)
        throw TableWithoutColumns(good_name);
    tables_[table_meta_data.get_name()] = table_meta_data;
    tables_[table_meta_data.get_name()].schema(*this);
}

const Table &
Schema::get_table(const String &name) const
{
    String good_name = str_to_upper(name);
    Map::const_iterator it = tables_.find(good_name);
    if (it == tables_.end())
        throw TableNotFoundInMetaData(good_name);
    return it->second;
}

const Table &
Schema::find_table_by_class(const String &class_name) const
{
    Map::const_iterator it = tables_.begin(), end = tables_.end();
    for (; it != end; ++it)
        if (it->second.get_class_name() == class_name)
            return it->second;
    throw ClassNotFoundInMetaData(class_name);
}

void
Schema::add_relation(const Relation &rel)
{
    relations_.push_back(rel);
    std::pair<String, Relation> p1(rel.side(0), rel);
    rels_.insert(p1);
    if (rel.side(0) != rel.side(1)) {
        std::pair<String, Relation> p2(rel.side(1), rel);
        rels_.insert(p2);
    }
}

void
Schema::fill_fkeys()
{
    Map::iterator i = tables_.begin(), iend = tables_.end();
    for (; i != iend; ++i) {
        Table &table = i->second;
        Columns::const_iterator j = table.begin(), jend = table.end(); 
        for (; j != jend; ++j) {
            if (!j->get_fk_table_name().empty() &&
                    j->get_fk_name().empty())
            {
                Column &c = const_cast<Column &>(*j);
                c.set_fk_name(get_table(j->get_fk_table_name()).get_synth_pk());
            }
        }
    }
    RelMap::iterator k = rels_.begin(), kend = rels_.end();
    for (; k != kend; ++k)
        k->second.set_tables(
                find_table_by_class(k->second.side(0)).get_name(),
                find_table_by_class(k->second.side(1)).get_name());
    Relations::iterator l = relations_.begin(), lend = relations_.end();
    for (; l != lend; ++l)
        l->set_tables(
                find_table_by_class(l->side(0)).get_name(),
                find_table_by_class(l->side(1)).get_name());
}

void
Schema::check()
{
    fill_fkeys();
    set<String> unique_tables;
    fill_unique_tables(unique_tables);
    StrMap tree;
    map<String, int> depths;
    zero_depths(unique_tables, depths);
    fill_map_tree_by_meta(unique_tables, tree);
    traverse_children(tree, depths);
    set_absolute_depths(depths);
}

const Relation *
Schema::find_relation(const String &class1,
                      const String &relation_name,
                      const String &class2,
                      int prop_side) const
{
    const Relation *r = NULL;
    RelMap::const_iterator
        it = rels_lower_bound(class1),
        end = rels_upper_bound(class1);
    for (; it != end; ++it)
        if (class2.empty() ||
            (it->second.side(0) == class1 &&
             it->second.side(1) == class2) ||
            (it->second.side(0) == class2 &&
             it->second.side(1) == class1))
        {
            if (!r) {
                if (relation_name.empty() ||
                    (it->second.has_attr(prop_side, _T("property")) &&
                     it->second.attr(prop_side, _T("property")) == relation_name))
                {
                    r = &it->second;
                }
            }
            else {
                YB_ASSERT(it->second.has_attr(prop_side, _T("property")) &&
                          it->second.attr(prop_side, _T("property")) != relation_name);
            }
        }
    return r;
}

void
Schema::set_absolute_depths(const map<String, int> &depths)
{
    map<String, int>::const_iterator it = depths.begin(), end = depths.end();
    for (; it != end; ++it)
        tables_[it->first].set_depth(it->second);    
}

void
Schema::fill_unique_tables(set<String> &unique_tables)
{
    Map::const_iterator it = tables_.begin(), end = tables_.end();
    for (; it != end; ++it)
        unique_tables.insert(it->first);
}

void
Schema::fill_map_tree_by_meta(const set<String> &unique_tables, StrMap &tree_map)
{
    set<String>::const_iterator it = unique_tables.begin(), end = unique_tables.end();
    for (; it != end; ++it) {
        const Table &t = get_table(*it);
        Columns::const_iterator it_col = t.begin(), end_col = t.end();
        bool has_parent = false;
        for (; it_col != end_col; ++it_col) {
            // if found a foreign key table in the set, add it with this dependent table
            if (it_col->has_fk()) {
                String fk_field = it_col->get_fk_name();
                String fk_table = it_col->get_fk_table_name(); 
                CheckForeignKey(t.get_name(), fk_table, fk_field);
                tree_map.insert(StrMap::value_type(it_col->get_fk_table_name(), t.get_name()));
                has_parent = true;
            }
        }
        if (!has_parent)
            tree_map.insert(StrMap::value_type(_T(""), t.get_name()));
    }
    // little hack, if no a root found(parent '' in map), the tree contains cycle
    if (tree_map.find(_T("")) == tree_map.end())
        throw IntegrityCheckFailed(_T("Cyclic references in DB schema found"));
}

void
Schema::CheckForeignKey(const String &table, const String &fk_table, const String &fk_field)
{
    if(tables_.find(fk_table) == tables_.end())
        throw IntegrityCheckFailed(String(_T("Table '")) + fk_table +
                _T("' not found as foreign key for '") + table + _T("'"));
    
    try {
        tables_[fk_table].get_column(fk_field);
    }
    catch (ColumnNotFoundInMetaData &) {
        throw IntegrityCheckFailed(String(_T("Field '")) + fk_field + _T(" of table '")
                + fk_table +
                _T("' not found as foreign key-field' of table '") + table + _T("'"));
    }
}

void
Schema::zero_depths(const set<String> &unique_tables, map<String, int> &depths)
{
    map<String, int> new_depths;
    set<String>::const_iterator it = unique_tables.begin(), end = unique_tables.end();
    for (; it != end; ++it)
        new_depths[*it] = 0;
    new_depths.swap(depths);
}

void
Schema::traverse_children(const StrMap &parent_child, map<String, int> &depths)
{
    list<String> children;
    children.push_back(_T(""));
    while (!children.empty()) {
        pair<StrMap::const_iterator, StrMap::const_iterator> range =
            parent_child.equal_range(*children.begin());
        for (; range.first != range.second; ++range.first) {
            const String &parent = range.first->first;
            const String &child = range.first->second;
            if (std::find(children.begin(), children.end(), child) == children.end()) {
                children.push_back(child);
                int new_depth = (parent.empty()? 0: depths[parent]) + 1;
                if (depths[child] < new_depth)
                    depths[child] = new_depth;
                if (new_depth > (int)parent_child.size())
                    throw IntegrityCheckFailed(_T("Cyclic references in DB schema found"));
            }
        }
        children.erase(children.begin());
    }
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
