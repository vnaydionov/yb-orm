#include <sstream>
#include <algorithm>
#include "util/string_utils.h"
#include "util/exception.h"
#include "util/value_type.h"
#include "orm/schema.h"
#include "orm/domain_factory.h"

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

FkNotFoundInMetaData::FkNotFoundInMetaData(const String &master_tbl,
        const String &slave_tbl)
    : MetaDataError(_T("Foreign key from table '") + slave_tbl
            + _T("' to table '") + master_tbl + _T("' not found in metadata"))
{}

RowNotLinkedToTable::RowNotLinkedToTable()
    : MetaDataError(_T("RowData object is not linked to any table"))
{}

ReadOnlyColumn::ReadOnlyColumn(const String &table, const String &column)
    : MetaDataError(_T("Column '") + column + _T("' in table '") + table + _T("' is READ-ONLY"))
{}

TableHasNoSurrogatePK::TableHasNoSurrogatePK(const String &table)
    : MetaDataError(_T("Table '") + table + _T("' has no surrogate primary key"))
{}


static
Char
underscore_to_dash(Char c) { return c == _T('_')? _T('-'): c; }

const String
mk_xml_name(const String &name, const String &xml_name)
{
    if (xml_name == _T("-"))
        return _T("");
    if (!str_empty(xml_name))
        return xml_name;
    return translate(str_to_lower(name), underscore_to_dash);
}

Column::Column(const String &name, int type, size_t size, int flags)
    : table_(NULL)
    , type_(type)
    , flags_(flags)
    , size_(size)
    , name_(name)
    , xml_name_(mk_xml_name(name, _T("")))
    , prop_name_(str_to_lower(name))
{}

Column::Column(const String &name, int type, size_t size, int flags,
        const Value &default_value,
        const String &fk_table, const String &fk,
        const String &xml_name, const String &prop_name,
        const String &index_name)
    : table_(NULL)
    , type_(type)
    , flags_(flags)
    , size_(size)
    , name_(name)
    , xml_name_(mk_xml_name(name, xml_name))
    , prop_name_(str_empty(prop_name)? str_to_lower(name): prop_name)
    , index_name_(index_name)
    , fk_table_name_(fk_table)
    , fk_name_(fk)
    , default_value_(default_value)
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

void
Table::add_column(const Column &column)
{
    if (!is_id(column.name()))
        throw BadColumnName(name(), column.name());
    String col_uname = str_to_upper(column.name());
    IndexMap::const_iterator it = indicies_.find(col_uname);
    int idx = -1;
    if (it == indicies_.end()) {
        idx = cols_.size();
        cols_.push_back(column);
        indicies_[column.name()] = idx;
        indicies_[col_uname] = idx;
        indicies_[str_to_lower(column.name())] = idx;
    }
    else {
        idx = it->second;
        cols_[idx] = column;
    }
    cols_[idx].set_table(*this);
    if (column.is_pk())
        pk_fields_.push_back(column.name());
}

size_t
Table::idx_by_name(const String &col_name) const
{
    IndexMap::const_iterator it = indicies_.find(col_name);
    if (it == indicies_.end())
        throw ColumnNotFoundInMetaData(name(), col_name);
    return it->second;
}

void
Table::set_seq_name(const String &seq_name)
{
    seq_name_ = seq_name;
}

const String &
Table::get_surrogate_pk() const
{
    if (pk_fields_.size() != 1)
        throw TableHasNoSurrogatePK(name());
    const Column &c = column(*pk_fields_.begin());
    if (c.type() != Value::LONGINT && c.type() != Value::INTEGER)
        throw TableHasNoSurrogatePK(name());
    return c.name();
}

Strings &
Table::find_fk_for(const Relation &rel, Strings &fkey_parts) const
{
    const String &master_tbl = rel.table(0).name();
    Strings new_fkey_parts;
    if (rel.has_attr(1, _T("key"))) {
        Strings parts0;
        StrUtils::split_str(rel.attr(1, _T("key")), _T(","), parts0);
        Strings::const_iterator i = parts0.begin(), iend = parts0.end();
        for (; i != iend; ++i) {
            const Column &c = column(*i);
            if (!c.has_fk() || c.fk_table_name() != master_tbl)
                throw BadColumnName(name(), c.name());
            new_fkey_parts.push_back(c.name());
        }
    }
    else {
        Columns::const_iterator i = begin(), iend = end();
        for (; i != iend; ++i)
            if (i->has_fk() && i->fk_table_name() == master_tbl)
                new_fkey_parts.push_back(i->name());
    }
    fkey_parts.swap(new_fkey_parts);
    return fkey_parts;
}

void
Table::mk_sample_key(TypeCodes &type_codes, Key &sample_key) const
{
    sample_key.first = name();
    ValueMap key_values;
    key_values.reserve(pk_fields().size());
    Strings::const_iterator i = pk_fields().begin(), iend = pk_fields().end();
    for (; i != iend; ++i) {
        int col_type = column(*i).type();
        Value x(_T("0"));
        x.fix_type(col_type);
        type_codes.push_back(col_type);
        key_values.push_back(make_pair(*i, x));
    }
    sample_key.second.swap(key_values);
}

bool
Table::mk_key(const Values &row_values, Key &key) const
{
    key.first = name();
    bool assigned_key = true;
    ValueMap key_values;
    key_values.reserve(pk_fields().size());
    Strings::const_iterator i = pk_fields().begin(), iend = pk_fields().end();
    for (; i != iend; ++i) {
        key_values.push_back(make_pair(*i, row_values[idx_by_name(*i)]));
        if (key_values[key_values.size() - 1].second.is_null())
            assigned_key = false;
    }
    key.second.swap(key_values);
    return assigned_key;
}

bool
Table::mk_key(const Row &row_values, Key &key) const
{
    key.first = name();
    bool assigned_key = true;
    ValueMap key_values;
    key_values.reserve(pk_fields().size());
    Strings::const_iterator i = pk_fields().begin(), iend = pk_fields().end();
    for (; i != iend; ++i) {
        key_values.push_back(make_pair(*i, row_values[idx_by_name(*i)].second));
        if (key_values[key_values.size() - 1].second.is_null())
            assigned_key = false;
    }
    key.second.swap(key_values);
    return assigned_key;
}

const Key
Table::mk_key(const Row &row_values) const
{
    Key key;
    mk_key(row_values, key);
    return key;
}

const Key
Table::mk_key(LongInt id) const
{
    Key key;
    key.first = name();
    String pk_name = get_surrogate_pk();
    key.second.push_back(make_pair(pk_name, Value(id)));
    return key;
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

Expression
Relation::join_condition() const
{
    YB_ASSERT(table1_ && table2_);
    const Strings &key_parts1 = table1_->pk_fields();
    const Strings &key_parts2 = fk_fields();
    YB_ASSERT(key_parts1.size() == key_parts2.size());
    Expression expr;
    Strings::const_iterator i = key_parts1.begin(), j = key_parts2.begin(),
        iend = key_parts1.end();
    for (; i != iend; ++i, ++j)
        expr = expr && BinaryOpExpr(
                ColumnExpr(table1_->name(), *i), _T("="),
                ColumnExpr(table2_->name(), *j));
    return expr;
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
        tables_lookup_.swap(x.tables_lookup_);
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
    if (!is_id(table->name()))
        throw BadTableName(table->name());
    if (table->size() == 0)
        throw TableWithoutColumns(table->name());
    tables_[table->name()] = table;
    tables_lookup_[table->name()] = table;
    tables_lookup_[str_to_upper(table->name())] = table;
    tables_lookup_[str_to_lower(table->name())] = table;
    table->set_schema(this);
}

const Table &
Schema::table(const String &name) const
{
    TblMap::const_iterator it = tables_lookup_.find(name);
    if (it == tables_lookup_.end())
        throw TableNotFoundInMetaData(name);
    return *shptr_get(it->second);
}

const Table &
Schema::find_table_by_class(const String &class_name) const
{
    TblMap::const_iterator it = tables_.begin(), end = tables_.end();
    for (; it != end; ++it)
        if (it->second->class_name() == class_name)
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
            if (!str_empty(j->fk_table_name()) &&
                    str_empty(j->fk_name()))
            {
                Column &c = const_cast<Column &> (*j);
                try {
                    c.set_fk_name(table(j->fk_table_name()).get_surrogate_pk());
                }
                catch (const TableHasNoSurrogatePK &)
                {}
            }
        }
    }
    RelVect::iterator l = relations_.begin(), lend = relations_.end();
    for (; l != lend; ++l) {
        Relation &r = **l;
        Table *t0 = const_cast<Table *> (&find_table_by_class(r.side(0))),
              *t1 = const_cast<Table *> (&find_table_by_class(r.side(1)));
        r.set_tables(t0, t1);
        if (r.type() == Relation::ONE2MANY) {
            const Strings &fkey_parts = r.fk_fields();
            if (!fkey_parts.size() || fkey_parts.size() != t0->pk_fields().size())
                throw FkNotFoundInMetaData(t0->name(), t1->name());
        }
    }
}

void
Schema::check_cycles()
{
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
    //YB_ASSERT(!str_empty(relation_name) || !str_empty(class2));
    RelMap::const_iterator it = rels_lower_bound(class1),
                           end = rels_upper_bound(class1);
    for (; it != end; ++it) {
        const Relation *t = shptr_get(it->second);
        if (str_empty(class2) ||
            t->side(1) == class2 && t->side(0) == class1 ||
            t->side(0) == class2 && t->side(1) == class1)
        {
            if (!r) {
                if (str_empty(relation_name) ||
                    (t->has_attr(prop_side, _T("property")) &&
                     t->attr(prop_side, _T("property")) == relation_name))
                {
                    r = t;
                }
            }
            else {
                YB_ASSERT(t != r ||
                          t->has_attr(prop_side, _T("property")) &&
                          t->attr(prop_side, _T("property")) != relation_name);
            }
        }
    }
    //YB_ASSERT(r);
    return r;
}

const Relation &
Schema::find_single_relation_between_tables(
        const String &tbl1, const String &tbl2) const
{
    const Relation *r = find_relation(
            table(tbl1).class_name(), String(), table(tbl2).class_name());
    YB_ASSERT(r != NULL);
    return *r;
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
        unique_tables.insert(it->second->name());
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
                String fk_field = it_col->fk_name();
                String fk_table = it_col->fk_table_name(); 
                check_foreign_key(t.name(), fk_table, fk_field);
                tree_map.insert(StrMap::value_type(it_col->fk_table_name(), t.name()));
                has_parent = true;
            }
        }
        if (!has_parent)
            tree_map.insert(StrMap::value_type(_T(""), t.name()));
    }
    // little hack, if no a root found(parent '' in map), the tree contains cycle
    //if (tree_map.find(_T("")) == tree_map.end())
    //    throw IntegrityCheckFailed(_T("Cyclic references in DB schema found"));
}

void
Schema::check_foreign_key(const String &table, const String &fk_table, const String &fk_field)
{
    TblMap::const_iterator i = tables_lookup_.find(fk_table);
    if (const_cast<const TblMap *>(&tables_lookup_)->end() == i)
        throw IntegrityCheckFailed(String(_T("Table '")) + fk_table +
                _T("' not found as foreign key for '") + table + _T("'"));
    try {
        i->second->column(fk_field);
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
                int new_depth = (str_empty(parent)? 0: depths[parent]) + 1;
                if (depths[child] < new_depth)
                    depths[child] = new_depth;
                if (new_depth > (int)parent_child.size())
                    throw IntegrityCheckFailed(_T("Cyclic references in DB schema found"));
            }
        }
        children.erase(children.begin());
    }
}

Expression
Schema::make_join_expr(const Expression &expr1, const String &tbl1,
        Strings::const_iterator it, Strings::const_iterator end) const
{
    if (it == end)
        return expr1;
    const String &tbl2 = *it;
    return make_join_expr(
            JoinExpr(expr1, Expression(tbl2),
                find_single_relation_between_tables(tbl1, tbl2).join_condition()),
            tbl2, ++it, end);
}

Expression
Schema::join_expr(const Strings &tables) const
{
    if (!tables.size())
        return Expression();
    Strings::const_iterator it = tables.begin();
    const String &tbl1 = *it;
    return make_join_expr(Expression(tbl1), tbl1, ++it, tables.end());
}

Schema &init_schema()
{
    Schema &schema = theSchema::instance();
    DomainObject::save_registered(schema);
    schema.fill_fkeys();
    schema.check_cycles();
    return schema;
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
