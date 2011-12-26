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
    const Value &get_default_value() const {
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

class Table: private noncopyable
{
    Table();
public:
    typedef boost::shared_ptr<Table> Ptr;
    Table(const String &name, const String &xml_name = _T(""),
        const String &class_name = _T(""));
    void set_schema(Schema *s) { schema_ = s; }
    Schema *get_schema() const { return schema_; }
    const Schema &schema() const {
        return *check_not_null(schema_, _T("get table's schema"));
    }
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
    const String &get_unique_pk() const;
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

typedef std::vector<Table::Ptr> Tables;

class Relation: private noncopyable
{
    Relation();
public:
    typedef boost::shared_ptr<Relation> Ptr;
    typedef std::map<String, String> AttrMap;
    enum { UNKNOWN = 0, ONE2MANY, MANY2MANY, PARENT2CHILD };
    enum { Restrict = 0, Nullify, Delete };
    Relation(int _type,
            const String &_side1, const AttrMap &_attr1,
            const String &_side2, const AttrMap &_attr2,
            int cascade_delete_action = Restrict)
        : type_(_type)
        , cascade_(cascade_delete_action)
        , side1_(_side1)
        , side2_(_side2)
        , attr1_(_attr1)
        , attr2_(_attr2)
        , table1_(NULL)
        , table2_(NULL)
    {}
    int type() const { return type_; }
    int cascade() const { return cascade_; }
    void set_cascade(int cascade_mode) { cascade_ = cascade_mode; }
    const String &side(int n) const { return n == 0? side1_: side2_; }
    bool has_attr(int n, const String &name) const;
    const String &attr(int n, const String &name) const;
    const AttrMap &attr_map(int n) const { return n == 0? attr1_: attr2_; }
    void set_tables(Table *table1, Table *table2) {
        table1_ = table1;
        table2_ = table2;
    }
    Table *get_table(int n) const { return n == 0? table1_: table2_; }
    const Table &table(int n) const {
        return *check_not_null(n == 0? table1_: table2_,
                _T("get relation's table"));
    }
    bool eq(const Relation &o) {
        return type_ == o.type_ && cascade_ == o.cascade_ 
            && side1_ == o.side1_ && side2_ == o.side2_
            && attr1_ == o.attr1_ && attr2_ == o.attr2_;
    }
private:
    int type_, cascade_;
    String side1_, side2_;
    AttrMap attr1_, attr2_;
    Table *table1_, *table2_;
};

typedef std::vector<Relation::Ptr> Relations;

class Schema
{
    friend class ::TestMetaData;
    typedef std::multimap<String, String> StrMap;
    Schema(const Schema &);
public:
    typedef std::map<String, Table::Ptr> TblMap;
    typedef std::multimap<String, Relation::Ptr> RelMap;
    typedef Relations RelVect;

    Schema() {}
    ~Schema();
    Schema &operator=(Schema &x);
    TblMap::const_iterator tbl_begin() const { return tables_.begin(); }
    TblMap::const_iterator tbl_end() const { return tables_.end(); }
    size_t tbl_count() const { return tables_.size(); }
    RelMap::const_iterator rels_lower_bound(const String &key) const {
        return rels_.lower_bound(key);
    };
    RelMap::const_iterator rels_upper_bound(const String &key) const {
        return rels_.upper_bound(key);
    };
    RelVect::const_iterator rel_begin() const { return relations_.begin(); }
    RelVect::const_iterator rel_end() const { return relations_.end(); }
    size_t rel_count() const { return relations_.size(); }
    void add_table(Table::Ptr table);
    const Table &table(const String &name) const;
    const Table &find_table_by_class(const String &class_name) const;
    void add_relation(Relation::Ptr rel);
    const Relation *find_relation(const String &class1,
                            const String &relation_name = _T(""),
                            const String &class2 = _T(""),
                            int prop_side = 0) const;
    void fill_fkeys();
    void check();
private:
    void clear_backrefs();
    void fix_backrefs();
    void check_foreign_key(const String &table, const String &fk_table, const String &fk_field);
    void set_absolute_depths(const std::map<String, int> &depths);
    void fill_unique_tables(std::set<String> &unique_tables);
    void zero_depths(const std::set<String> &unique_tables, std::map<String, int> &depths);
    void fill_map_tree_by_meta(const std::set<String> &unique_tables, StrMap &tree_map);
    void traverse_children(const StrMap &parent_child, std::map<String, int> &depths);
    TblMap tables_;
    RelMap rels_;
    RelVect relations_;
};

const String mk_xml_name(const String &name, const String &xml_name);

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__META_DATA__INCLUDED
