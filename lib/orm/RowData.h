#ifndef YB__ORM__ROW_DATA__INCLUDED
#define YB__ORM__ROW_DATA__INCLUDED

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <stdexcept>
#include "Value.h"
#include "MetaData.h"
#include "Filters.h"
#include "EngineBase.h"

class TestRowData;

namespace Yb {

class RowData;
typedef std::auto_ptr<RowData> RowDataPtr;
typedef std::vector<RowData> RowDataVector;
typedef std::auto_ptr<RowDataVector> RowDataVectorPtr;

class ORMError : public std::logic_error
{
public:
    ORMError(const std::string &msg);
};

class ObjectNotFoundByKey : public ORMError
{
public:
    ObjectNotFoundByKey(const std::string &msg);
};

class FieldNotFoundInFetchedRow : public ORMError
{
public:
    FieldNotFoundInFetchedRow(const std::string &table_name, const std::string &field_name);
};

class BadTypeCast : public ORMError
{
public:
    BadTypeCast(const std::string &table_name, const std::string &field_name,
            const std::string &str_value, const std::string &type);
};

class TableDoesNotMatchRow : public ORMError
{
public:
    TableDoesNotMatchRow(const std::string &table_name, const std::string &table_name_from_row);
};

class StringTooLong : public ORMError
{
public:
    StringTooLong(const std::string &table_name, const std::string &field_name,
                  int max_len, const std::string &value);
};

class DataSource
{
public:
    virtual RowDataPtr select_row(const RowData &key) = 0;
    virtual RowDataVectorPtr select_rows(
            const std::string &table_name, const Filter &filter, const StrList &order_by = StrList(),
            int max = -1, const std::string &table_alias = "") = 0;
    virtual void insert_rows(const std::string &table_name,
            const RowDataVector &rows) = 0;
    virtual void update_rows(const std::string &table_name,
            const RowDataVector &rows) = 0;
    virtual void delete_row(const RowData &row) = 0;
    virtual ~DataSource();
};

struct PKIDRecord;
class Mapper;

class RowData
{
    friend class ::TestRowData;
    friend class Mapper;
    struct Entry {
        bool init;
        Value value;
        Entry() : init(false) {}
        explicit Entry(const Value &v) : init(true), value(v) {}
    };
    enum PersistStatus { Undef, New, Ghost, Dirty, Sync, Deleted };
public:
    typedef std::map<std::string, Entry> Map;
    RowData();
    RowData(const TableMetaDataRegistry &reg, const std::string &table_name);
    const TableMetaData &get_table() const;
    const Value &get(const std::string &column_name) const;
    const PKIDValue get_id() const;
    void set(const std::string &column_name, const Value &value);
    void set_pk(LongInt pk);
    void set_new_pk(boost::shared_ptr<PKIDRecord> new_pk);
    void set_data_source(DataSource *ds) { ds_ = ds; }
    bool is_new() const { return status_ == New; }
    bool is_ghost() const { return status_ == Ghost; }
    bool is_dirty() const { return status_ == Dirty; }
    bool is_deleted() const { return status_ == Deleted; }
    void set_new() { status_= New; }
    void set_ghost() { status_= Ghost; }
    void set_sync() { status_ = Sync; }
    void set_deleted() { status_ = Deleted; }
    bool lt(const RowData &x, bool key_only = false) const;
    void load(void) const;
private:
    void load_if_ghost_and_if_non_key_field_requested(const ColumnMetaData &c) const;
    const Value get_typed_value(const ColumnMetaData &c, const Value &value);
private:
    const TableMetaDataRegistry *reg_;
    const TableMetaData *table_;
    DataSource *ds_;
    mutable PersistStatus status_;
    mutable Map values_;
};

inline bool operator==(const RowData &x, const RowData &y) { return !x.lt(y) && !y.lt(x); }
inline bool operator!=(const RowData &x, const RowData &y) { return x.lt(y) || y.lt(x); }

class RowData_key_less
{
public:
    bool operator()(const RowData &x, const RowData &y) const { return x.lt(y, true); }
};

class FilterBackendByKey : public FilterBackend
{
    RowData key_;
public:
    FilterBackendByKey(const RowData &key)
        : key_(key)
    {}
    const std::string do_get_sql() const;
    const std::string do_collect_params_and_build_sql(
            Values &seq) const;
    const RowData &get_key_data() const { return key_; }
};

class FilterByKey : public Filter
{
public:
    FilterByKey(const RowData &key)
        : Filter(new FilterBackendByKey(key))
    {}
};

typedef std::map<RowData, boost::shared_ptr<RowData>,
        RowData_key_less> RowSet;
typedef std::auto_ptr<std::vector<RowData * > > LoadedRows;

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__ROW_DATA__INCLUDED
