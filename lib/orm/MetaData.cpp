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
    : name_(name)
    , type_(type)
    , size_(size)
    , flags_(flags)
    , fk_table_name_(fk_table)
    , fk_name_(fk)
    , xml_name_(mk_xml_name(name, xml_name))
    , prop_name_(prop_name.empty()? str_to_lower(name): prop_name)
    , table_(NULL)
{}

Table::Table(const String &name, const String &xml_name,
        const String &class_name)
    : name_(name)
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
    String column_uname = str_to_upper(column.get_name());
    IndexMap::const_iterator it = indicies_.find(column_uname);
    int idx = -1;
    if (it == indicies_.end()) {
        idx = cols_.size();
        cols_.push_back(column);
        indicies_[column_uname] = idx;
    }
    else {
        idx = it->second;
        cols_[idx] = column;
    }
    cols_[idx].table(*this);
    if (column.is_pk())
        pk_fields_.insert(column_uname);
}

size_t
Table::idx_by_name(const String &col_name) const
{
    String column_uname = str_to_upper(col_name);
    IndexMap::const_iterator it = indicies_.find(column_uname);
    if (it == indicies_.end())
        throw ColumnNotFoundInMetaData(get_name(), col_name);
    return it->second;
}

void
Table::set_seq_name(const String &seq_name)
{
    seq_name_ = seq_name;
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
    const String &master_tbl = rel->table(0).get_name();
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

Schema::~Schema()
{
    clear_backrefs();
}

void
Schema::clear_backrefs()
{
    TblMap::iterator i = tables_.begin(), iend = tables_.end();
    for (; i != iend; ++i)
        i->second->set_schema(NULL);
    RelVect::iterator j = relations_.begin(), jend = relations_.end();
    for (; j != jend; ++j)
        (*j)->set_tables(NULL, NULL);
}

void
Schema::fix_backrefs()
{
    TblMap::iterator i = tables_.begin(), iend = tables_.end();
    for (; i != iend; ++i)
        i->second->set_schema(this);
    RelVect::iterator j = relations_.begin(), jend = relations_.end();
    for (; j != jend; ++j)
        (*j)->set_tables(
                const_cast<Table *> (&find_table_by_class((*j)->side(0))),
                const_cast<Table *> (&find_table_by_class((*j)->side(1))));
}

Schema &
Schema::operator=(Schema &x)
{
    if (&x != this) {
        clear_backrefs();
        tables_.swap(x.tables_);
        rels_.swap(x.rels_);
        relations_.swap(x.relations_);
        fix_backrefs();
    }
    return *this;
}

void
Schema::add_table(Table::Ptr table)
{
    if (!is_id(table->get_name()))
        throw BadTableName(table->get_name());
    if (table->size() == 0)
        throw TableWithoutColumns(table->get_name());
    String table_uname = str_to_upper(table->get_name());
    tables_[table_uname] = table;
    tables_[table_uname]->set_schema(this);
}

const Table &
Schema::table(const String &name) const
{
    String table_uname = str_to_upper(name);
    TblMap::const_iterator it = tables_.find(table_uname);
    if (it == tables_.end())
        throw TableNotFoundInMetaData(name);
    return *it->second.get();
}

const Table &
Schema::find_table_by_class(const String &class_name) const
{
    TblMap::const_iterator it = tables_.begin(), end = tables_.end();
    for (; it != end; ++it)
        if (it->second->get_class_name() == class_name)
            return *it->second;
    throw ClassNotFoundInMetaData(class_name);
}

void
Schema::add_relation(Relation::Ptr rel)
{
    Relations::iterator i = relations_.begin(), iend = relations_.end();
    for (; i != iend; ++i)
        if ((*i)->eq(*rel))
            return;
    relations_.push_back(rel);
    std::pair<String, Relation::Ptr> p1(rel->side(0), rel);
    rels_.insert(p1);
    if (rel->side(0) != rel->side(1)) {
        std::pair<String, Relation::Ptr> p2(rel->side(1), rel);
        rels_.insert(p2);
    }
}

void
Schema::fill_fkeys()
{
    TblMap::iterator i = tables_.begin(), iend = tables_.end();
    for (; i != iend; ++i) {
        Table &tbl = *i->second;
        Columns::const_iterator j = tbl.begin(), jend = tbl.end(); 
        for (; j != jend; ++j) {
            if (!j->get_fk_table_name().empty() &&
                    j->get_fk_name().empty())
            {
                Column &c = const_cast<Column &> (*j);
                try {
                    c.set_fk_name(table(j->get_fk_table_name()).get_unique_pk());
                }
                catch (const TableNotFoundInMetaData &)
                {}
            }
        }
    }
    RelVect::iterator l = relations_.begin(), lend = relations_.end();
    for (; l != lend; ++l) {
        Table *t1 = NULL;
        try {
            t1 = const_cast<Table *> (&find_table_by_class((*l)->side(0)));
        }
        catch (const ClassNotFoundInMetaData &)
        {}
        Table *t2 = NULL;
        try {
            t2 = const_cast<Table *> (&find_table_by_class((*l)->side(1)));
        }
        catch (const ClassNotFoundInMetaData &)
        {}
        (*l)->set_tables(t1, t2);
    }
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
            (it->second->side(0) == class1 &&
             it->second->side(1) == class2) ||
            (it->second->side(0) == class2 &&
             it->second->side(1) == class1))
        {
            if (!r) {
                if (relation_name.empty() ||
                    (it->second->has_attr(prop_side, _T("property")) &&
                     it->second->attr(prop_side, _T("property")) == relation_name))
                {
                    r = it->second.get();
                }
            }
            else {
                YB_ASSERT(it->second->has_attr(prop_side, _T("property")) &&
                          it->second->attr(prop_side, _T("property")) != relation_name);
            }
        }
    return r;
}

void
Schema::set_absolute_depths(const map<String, int> &depths)
{
    map<String, int>::const_iterator it = depths.begin(), end = depths.end();
    for (; it != end; ++it)
        const_cast<Table *> (&table(it->first))->set_depth(it->second);
}

void
Schema::fill_unique_tables(set<String> &unique_tables)
{
    TblMap::const_iterator it = tables_.begin(), end = tables_.end();
    for (; it != end; ++it)
        unique_tables.insert(it->second->get_name());
}

void
Schema::fill_map_tree_by_meta(const set<String> &unique_tables, StrMap &tree_map)
{
    set<String>::const_iterator it = unique_tables.begin(), end = unique_tables.end();
    for (; it != end; ++it) {
        const Table &t = table(*it);
        Columns::const_iterator it_col = t.begin(), end_col = t.end();
        bool has_parent = false;
        for (; it_col != end_col; ++it_col) {
            // if found a foreign key table in the set, add it with this dependent table
            if (it_col->has_fk()) {
                String fk_field = it_col->get_fk_name();
                String fk_table = it_col->get_fk_table_name(); 
                check_foreign_key(t.get_name(), fk_table, fk_field);
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
Schema::check_foreign_key(const String &table, const String &fk_table, const String &fk_field)
{
    String fk_table_uname = str_to_upper(fk_table);
    if (tables_.find(fk_table_uname) == tables_.end())
        throw IntegrityCheckFailed(String(_T("Table '")) + fk_table +
                _T("' not found as foreign key for '") + table + _T("'"));
    try {
        tables_[fk_table_uname]->get_column(fk_field);
    }
    catch (ColumnNotFoundInMetaData &) {
        throw IntegrityCheckFailed(String(_T("Field '")) + fk_field +
                _T(" of table '") + fk_table +
                _T("' not found as foreign key-field' of table '") +
                table + _T("'"));
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
