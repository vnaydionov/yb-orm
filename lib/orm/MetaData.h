#ifndef YB__ORM__META_DATA__INCLUDED
#define YB__ORM__META_DATA__INCLUDED

#include <string>
#include <map>
#include <set>
#include <stdexcept>
#include <list>
#include "orm/Value.h"

class TestMetaData;

namespace Yb {

class MetaDataError : public std::logic_error
{
public:
    MetaDataError(const std::string &msg);
};

class BadColumnName : public MetaDataError
{
public:
    BadColumnName(const std::string &table, const std::string &column);
};

class ColumnNotFoundInMetaData : public MetaDataError
{
public:
    ColumnNotFoundInMetaData(const std::string &table, const std::string &column);
};

class TableWithoutColumns : public MetaDataError
{
public:
    TableWithoutColumns(const std::string &table);
};

class BadTableName : public MetaDataError
{
public:
    BadTableName(const std::string &table);
};

class TableNotFoundInMetaData : public MetaDataError
{
public:
    TableNotFoundInMetaData(const std::string &table);
};

class RowNotLinkedToTable: public MetaDataError
{
public:
    RowNotLinkedToTable();
};

class ReadOnlyColumn : public MetaDataError
{
public:
    ReadOnlyColumn(const std::string &table, const std::string &column);
};

class NotSuitableForAutoCreating : public MetaDataError
{
public:
    NotSuitableForAutoCreating(const std::string &table);
};

class IntegrityCheckFailed: public MetaDataError
{
public:
    IntegrityCheckFailed(const std::string &what)
        : MetaDataError(what)
    {}
};

class AmbiguousPK : public MetaDataError
{
public:
    AmbiguousPK(const std::string &table)
        : MetaDataError(std::string("Ambiguous primary key for table '"  + table + "'"))
    {}
};

class ColumnMetaData
{
public:
    enum { PK = 1, RO = 2, NULLABLE = 4 };
    ColumnMetaData(const std::string &name = "",
            int type = 0, size_t size = 0, int flags = 0,
            const std::string &fk_table = "", const std::string &fk = "",
            const std::string &xml_name = "");

    void set_default_value(const Value &value) {
	    default_value_ = value;
    }
    const Value get_default_value() const {
	    return default_value_;
    }
    const std::string &get_name() const { return name_; }
    int get_type() const { return type_; }
    size_t get_size() const { return size_; }
    int get_flags() const { return flags_; }
    bool is_pk() const { return flags_ & PK; }
    bool is_ro() const { return flags_ & RO; }
    bool is_nullable() const { return flags_ & NULLABLE; }
    const std::string &get_fk_table_name() const { return fk_table_name_; }
    const std::string &get_fk_name() const { return fk_name_; }
    bool has_fk() const { return !fk_table_name_.empty() && !fk_name_.empty(); }
    const std::string &get_xml_name() const { return xml_name_; }
private:
    std::string name_;
    int type_;
    size_t size_;
    int flags_;
    std::string fk_table_name_;
    std::string fk_name_;
    std::string xml_name_;
    Value default_value_;
};

class TableMetaData
{
public:
    const std::string get_unique_pk() const;
    typedef std::map<std::string, ColumnMetaData> Map;
    TableMetaData(const std::string &name = "", const std::string &xml_name = "");
    const std::string &get_name() const { return name_; }
    const std::string &get_xml_name() const { return xml_name_; }
    Map::const_iterator begin() const { return cols_.begin(); }
    Map::const_iterator end() const { return cols_.end(); }
    size_t size() const { return cols_.size(); }
    const ColumnMetaData &get_column(const std::string &name) const;
    const std::string get_seq_name() const;
    bool get_autoinc() const;
    const std::string find_synth_pk() const;
    const std::string get_synth_pk() const;
    int get_depth() const { return depth_; }
    void set_column(const ColumnMetaData &column);
    void set_seq_name(const std::string &seq_name);
    void set_autoinc(bool autoinc) { autoinc_ = autoinc; }
    void set_name(const std::string &name) { name_ = name; }
    void set_xml_name(const std::string &xml_name) { xml_name_ = xml_name; }
    void set_depth(int depth) { depth_ = depth; }
    void set_db_type(const std::string &db_type) { db_type_ = db_type; }
private:
    std::string name_;
    std::string xml_name_;
    std::string seq_name_;
    bool autoinc_;
    Map cols_;
    int depth_;
    std::string db_type_;
};

class TableMetaDataRegistry
{
    friend class ::TestMetaData;
    typedef std::multimap<std::string, std::string> StrMap;    
public:
    typedef std::map<std::string, TableMetaData> Map;
    Map::const_iterator begin() const { return tables_.begin(); }
    Map::const_iterator end() const { return tables_.end(); }
    size_t size() const { return tables_.size(); }
    void set_table(const TableMetaData &table_meta_data);
    const TableMetaData &get_table(const std::string &name) const;
    const std::string &get_db_type() const { return db_type_; }
    void set_db_type(const std::string &db_type) { db_type_ = db_type; }
    void check();
private:
    void CheckForeignKey(const std::string &table, const std::string &fk_table, const std::string &fk_field);
    void set_absolute_depths(const std::map<std::string, int> &depths);
    void fill_unique_tables(std::set<std::string> &unique_tables);
    void zero_depths(const std::set<std::string> &unique_tables, std::map<std::string, int> &depths);
    void fill_map_tree_by_meta(const std::set<std::string> &unique_tables, StrMap &tree_map);
    void traverse_children(const StrMap &parent_child, std::map<std::string, int> &depths);
    Map tables_;
    std::string db_type_;
};

const std::string mk_xml_name(const std::string &name, const std::string &xml_name);

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__META_DATA__INCLUDED
