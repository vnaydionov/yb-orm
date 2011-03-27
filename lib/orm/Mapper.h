#ifndef YB__ORM__MAPPER__INCLUDED
#define YB__ORM__MAPPER__INCLUDED

#include <string>
#include <list>
#include <set>
#include <map>
#include <boost/shared_ptr.hpp>
#include "RowData.h"

class TestTableMapper;

namespace Yb {

class Mapper
{
public:
    virtual ~Mapper();
    virtual RowData *find(const RowData &key) = 0;
    virtual LoadedRows load_collection(
            const std::string &table_name, const Filter &filter, 
            const StrList &order_by = StrList(), int max = -1,
            const std::string &table_alias = "") = 0;
    virtual RowData *create(const std::string &table_name) = 0;
    virtual RowData *register_as_new(const RowData &row) = 0;
    virtual void flush() = 0;
    virtual const TableMetaDataRegistry &get_meta_data_registry() = 0;
};

class TableMapper : public Mapper
{
    friend class ::TestTableMapper;
    typedef std::multimap<std::string, std::string> StrMap;
public:
    TableMapper(const TableMetaDataRegistry &reg, DataSource &ds);
    RowData *find(const RowData &key);
    LoadedRows load_collection(
            const std::string &table_name, const Filter &filter, 
            const StrList &order_by = StrList(), int max = -1,
            const std::string &table_alias = "");
    RowData *create(const std::string &table_name);
    RowData *register_as_new(const RowData &row);
    void flush();
    const TableMetaDataRegistry &get_meta_data_registry();
private:
    void flush_new();
    void insert_new_to_table(const std::string &table_name);
    void mark_new_as_ghost();
    void get_unique_tables_for_insert(std::set<std::string> &unique_tables);
    void sort_tables(const std::set<std::string> &unique_tabs, std::list<std::string> &ordered_tables);
    boost::shared_ptr<PKIDRecord> create_temp_pkid(
            const TableMetaData &table);
private:
    const TableMetaDataRegistry &reg_;
    DataSource &ds_;
    RowSet rows_;
    std::set<std::string> tables_for_insert_;
    LongInt new_id_;
    std::vector<boost::shared_ptr<PKIDRecord> > new_ids_;
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__MAPPER__INCLUDED
