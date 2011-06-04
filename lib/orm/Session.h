#ifndef YB__ORM__SESSION__INCLUDED
#define YB__ORM__SESSION__INCLUDED

#include <memory>
#include <string>
#include <list>
#include <set>
#include <map>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include "RowData.h"

class TestSession;

namespace Yb {

class SessionBase
{
public:
    virtual ~SessionBase();
    virtual RowData *find(const RowData &key) = 0;
    virtual LoadedRows load_collection(
            const std::string &table_name, const Filter &filter, 
            const StrList &order_by = StrList(), int max = -1,
            const std::string &table_alias = "") = 0;
    virtual RowData *create(const std::string &table_name) = 0;
    virtual RowData *register_as_new(const RowData &row) = 0;
    virtual void flush() = 0;
    virtual const Schema &get_schema() = 0;
    virtual DataSource &get_ds() = 0;
};

class Session : private boost::noncopyable, public SessionBase
{
    friend class ::TestSession;
    typedef std::multimap<std::string, std::string> StrMap;
public:
    Session(const Schema &reg, std::auto_ptr<DataSource> ds);
    RowData *find(const RowData &key);
    LoadedRows load_collection(
            const std::string &table_name, const Filter &filter, 
            const StrList &order_by = StrList(), int max = -1,
            const std::string &table_alias = "");
    RowData *create(const std::string &table_name);
    RowData *register_as_new(const RowData &row);
    void flush();
    const Schema &get_schema();
    DataSource &get_ds();
private:
    void flush_new();
    void insert_new_to_table(const std::string &table_name);
    void mark_new_as_ghost();
    void get_unique_tables_for_insert(std::set<std::string> &unique_tables);
    void sort_tables(const std::set<std::string> &unique_tabs, std::list<std::string> &ordered_tables);
    boost::shared_ptr<PKIDRecord> create_temp_pkid(
            const Table &table);
private:
    const Schema &reg_;
    std::auto_ptr<DataSource> ds_;
    RowSet rows_;
    std::set<std::string> tables_for_insert_;
    LongInt new_id_;
    std::vector<boost::shared_ptr<PKIDRecord> > new_ids_;
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__SESSION__INCLUDED
