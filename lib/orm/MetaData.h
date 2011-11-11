// -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#ifndef YB__ORM__META_DATA__INCLUDED
#define YB__ORM__META_DATA__INCLUDED

#include <string>
#include <vector>
#include <map>
#include <set>
#include <stdexcept>
#include <list>
#include <orm/Value.h>
#include <util/Exception.h>

class TestMetaData;

namespace Yb {

class MetaDataError : public BaseError
{
public:
    MetaDataError(const String &msg);
};

class BadAttributeName: public MetaDataError
{
public:
    BadAttributeName(const String &obj, const String &attr);
};

class BadColumnName : public MetaDataError {
public:
    BadColumnName(const String &table, const String &column);
};

class ColumnNotFoundInMetaData : public MetaDataError
{
public:
    ColumnNotFoundInMetaData(const String &table, const String &column);
};

class TableWithoutColumns : public MetaDataError
{
public:
    TableWithoutColumns(const String &table);
};

class BadTableName : public MetaDataError
{
public:
    BadTableName(const String &table);
};

class TableNotFoundInMetaData : public MetaDataError
{
public:
    TableNotFoundInMetaData(const String &table);
};

class ClassNotFoundInMetaData : public MetaDataError
{
public:
    ClassNotFoundInMetaData(const String &class_name);
};

class RowNotLinkedToTable: public MetaDataError
{
public:
    RowNotLinkedToTable();
};

class ReadOnlyColumn : public MetaDataError
{
public:
    ReadOnlyColumn(const String &table, const String &column);
};

class NotSuitableForAutoCreating : public MetaDataError
{
public:
    NotSuitableForAutoCreating(const String &table);
};

class IntegrityCheckFailed: public MetaDataError
{
public:
    IntegrityCheckFailed(const String &what)
        : MetaDataError(what)
    {}
};

class AmbiguousPK : public MetaDataError
{
public:
    AmbiguousPK(const String &table)
        : MetaDataError(String(_T("Ambiguous primary key for table '")  + table + _T("'")))
    {}
};

class Table;

class NullPointer: public std::runtime_error {
public: NullPointer(const String &ctx)
    : std::runtime_error(NARROW(ctx)) {}
};

template <class T, class U>
T *check_not_null(T *p, U ctx)
{
    if (!p)
        throw NullPointer(ctx);
    return p;
}

class Column
{
public:
    enum { PK = 1, RO = 2, NULLABLE = 4 };
    Column(const String &name = _T(""),
            int type = 0, size_t size = 0, int flags = 0,
            const String &fk_table = _T(""), const String &fk = _T(""),
            const String &xml_name = _T(""),
            const String &prop_name = _T(""));
    Table &table() const {
        return *check_not_null(table_, _T("get column's parent table")); }
    void table(Table &t) { table_ = &t; }
    void set_default_value(const Value &value) {
        default_value_ = value;
    }
    const Value get_default_value() const {
        return default_value_;
    }
    const String &get_name() const { return name_; }
    int get_type() const { return type_; }
    size_t get_size() const { return size_; }
    int get_flags() const { return flags_; }
    bool is_pk() const { return (flags_ & PK) != 0; }
    bool is_ro() const { return (flags_ & RO) != 0; }
    bool is_nullable() const { return (flags_ & NULLABLE) != 0; }
    const String &get_fk_table_name() const { return fk_table_name_; }
    const String &get_fk_name() const { return fk_name_; }
    bool has_fk() const {
        return !fk_table_name_.empty() && !fk_name_.empty();
    }
    void set_fk_name(const String &fk_name) { fk_name_ = fk_name; }
    const String &get_xml_name() const { return xml_name_; }
    const String &get_prop_name() const { return prop_name_; }
private:
    String name_;
    int type_;
    size_t size_;
    int flags_;
    String fk_table_name_;
    String fk_name_;
    String xml_name_;
    String prop_name_;
    Value default_value_;
    Table *table_;
};

typedef std::vector<Column> Columns;
typedef std::map<String, int> IndexMap;

class Schema;
class Relation;

class Table
{
public:
    const String &get_unique_pk() const;
    Table(const String &name = _T(""), const String &xml_name = _T(""),
        const String &class_name = _T(""));
    Schema &schema() const {
        return *check_not_null(schema_, _T("get table's parent schema")); }
    void schema(Schema &s) { schema_ = &s; }
    const String &get_name() const { return name_; }
    const String &get_xml_name() const { return xml_name_; }
    const String &get_class_name() const { return get_class(); }
    const String &get_class() const { return class_name_; }
    Columns::const_iterator begin() const { return cols_.begin(); }
    Columns::const_iterator end() const { return cols_.end(); }
    size_t size() const { return cols_.size(); }
    size_t idx_by_name(const String &col_name) const;
    const Column &get_column(const String &col_name) const
        { return cols_[idx_by_name(col_name)]; }
    const Column &get_column(size_t idx) const
        { return cols_[idx]; }
    const String get_seq_name() const;
    bool get_autoinc() const;
    const String find_synth_pk() const;
    const String get_synth_pk() const;
    const String get_fk_for(const Relation *rel) const;
    int get_depth() const { return depth_; }
    void add_column(const Column &column);
    void set_seq_name(const String &seq_name);
    void set_autoinc(bool autoinc) { autoinc_ = autoinc; }
    void set_name(const String &name) { name_ = name; }
    void set_xml_name(const String &xml_name) { xml_name_ = xml_name; }
    void set_class_name(const String &class_name) { class_name_ = class_name; }
    void set_depth(int depth) { depth_ = depth; }
    const StringSet &pk_fields() const { return pk_fields_; }
    const Key mk_key(const ValueMap &key_fields) const;
    const Key mk_key(LongInt id) const;
    void refresh();
private:
    String name_;
    String xml_name_;
    String class_name_;
    String seq_name_;
    bool autoinc_;
    Columns cols_;
    IndexMap indicies_;
    StringSet pk_fields_;
    int depth_;
    Schema *schema_;
};

class Relation {
public:
    typedef std::map<String, String> AttrMap;
    enum { UNKNOWN = 0, ONE2MANY, MANY2MANY, PARENT2CHILD };
    enum { Restrict = 0, Nullify, Delete };
    Relation()
        : type_(UNKNOWN), cascade_(Restrict) {}
    Relation(int _type,
            const String &_side1, const AttrMap &_attr1,
            const String &_side2, const AttrMap &_attr2,
            int cascade_delete_action = Restrict)
        : type_(_type), side1_(_side1), side2_(_side2),
        attr1_(_attr1), attr2_(_attr2),
        cascade_(cascade_delete_action)
    {}
    int type() const { return type_; }
    int cascade() const { return cascade_; }
    void set_cascade(int cascade_mode) { cascade_ = cascade_mode; }
    const String &side(int n) const { return n == 0? side1_: side2_; }
    const String &table(int n) const { return n == 0? table1_: table2_; }
    bool has_attr(int n, const String &name) const;
    const String &attr(int n, const String &name) const;
    void set_tables(const String &table1, const String &table2) {
        table1_ = table1;
        table2_ = table2;
    }
    bool operator==(const Relation &o) const {
        if (this == &o)
            return true;
        return type_ == o.type_ && cascade_ == o.cascade_ &&
            side1_ == o.side1_ && side2_ == o.side2_ &&
            attr1_ == o.attr1_ && attr2_ == o.attr2_;
    }
    bool operator!=(const Relation &o) const {
        return !((*this) == o);
    }
private:
    int type_, cascade_;
    String side1_, side2_;
    String table1_, table2_;
    AttrMap attr1_, attr2_;
};

typedef std::vector<Relation> Relations;

class Schema
{
    friend class ::TestMetaData;
    typedef std::multimap<String, String> StrMap;    
public:
    typedef std::map<String, Table> Map;
    typedef std::multimap<String, Relation> RelMap;
    Map::const_iterator begin() const { return tables_.begin(); }
    Map::const_iterator end() const { return tables_.end(); }
    size_t size() const { return tables_.size(); }
    RelMap::const_iterator rels_lower_bound(const String &key) const {
        return rels_.lower_bound(key);
    };
    RelMap::const_iterator rels_upper_bound(const String &key) const {
        return rels_.upper_bound(key);
    };
    Relations::const_iterator rel_begin() const { return relations_.begin(); }
    Relations::const_iterator rel_end() const { return relations_.end(); }
    size_t rel_count() const { return relations_.size(); }
    void set_table(const Table &table_meta_data);
    const Table &get_table(const String &name) const;
    const Table &find_table_by_class(const String &class_name) const;
    void add_relation(const Relation &rel);
    void fill_fkeys();
    void check();
    const Relation *find_relation(const String &class1,
                            const String &relation_name = _T(""),
                            const String &class2 = _T(""),
                            int prop_side = 0) const;
private:
    void CheckForeignKey(const String &table, const String &fk_table, const String &fk_field);
    void set_absolute_depths(const std::map<String, int> &depths);
    void fill_unique_tables(std::set<String> &unique_tables);
    void zero_depths(const std::set<String> &unique_tables, std::map<String, int> &depths);
    void fill_map_tree_by_meta(const std::set<String> &unique_tables, StrMap &tree_map);
    void traverse_children(const StrMap &parent_child, std::map<String, int> &depths);
    Map tables_;
    RelMap rels_;
    Relations relations_;
};

const String mk_xml_name(const String &name, const String &xml_name);

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__META_DATA__INCLUDED
