#ifndef YB__ORM__DATA_OBJ__INCLUDED
#define YB__ORM__DATA_OBJ__INCLUDED

#include <stdexcept>
#include <boost/shared_ptr.hpp>
#include "Mapper.h"
#include "XMLNode.h"

namespace Yb {

class NoRawData : public std::logic_error
{
public:
    NoRawData()
        : logic_error("No ROW data is associated with DataObject")
    {}
};

class NoMapperGiven : public std::logic_error
{
public:
    NoMapperGiven()
        : logic_error("No mapper given to the WeakObject")
    {}
};

class DataObject
{
public:
    virtual ~DataObject()
    {}

    virtual const XMLNode auto_xmlize(Mapper &mapper, int deep) const = 0;
    const Value &get(const std::string &column_name) const
    {
        return get_row_data().get(column_name);
    }
    
    void set(const std::string &column_name, const Value &value)
    {
        return get_row_data().set(column_name, value);
    }
    RowData &get_row_data()
    {
        RowData *d = ptr();
        if (!d)
            throw NoRawData();
        return *d;
    }
    const RowData &get_row_data() const
    {
        const RowData *d = ptr();
        if (!d)
            throw NoRawData();
        return *d;
    }
private:
    virtual RowData *ptr() const = 0;
};

class StrongObject : public DataObject
{
    RowData *d_;
    RowData *ptr() const { return d_; }
    static const RowData mk_key(Mapper &mapper,
            const std::string &table_name, long long id)
    {
        RowData key(mapper.get_meta_data_registry(), table_name);
        const TableMetaData &table = key.get_table();
        key.set(table.get_synth_pk(), Value(id));
        return key;
    }
public:
    StrongObject()
        : d_(NULL)
    {}
    StrongObject(Mapper &mapper, const RowData &key)
        : d_(mapper.find(key))
    {}
    StrongObject(Mapper &mapper, const std::string &table_name, long long id)
        : d_(mapper.find(mk_key(mapper, table_name, id)))
    {}
    StrongObject(Mapper &mapper, const std::string &table_name)
        : d_(mapper.create(table_name))
    {}

    virtual const XMLNode auto_xmlize(Mapper &mapper, int deep = 0) const
    {    
        return deep_xmlize(mapper, get_row_data(), deep);
    }
};

class WeakObject : public DataObject
{
    RowData *d_;
    boost::shared_ptr<RowData> new_d_;
    Mapper *mapper_;
    RowData *ptr() const { return d_? d_: new_d_.get(); }
    static const RowData mk_key(Mapper &mapper,
            const std::string &table_name, long long id)
    {
        RowData key(mapper.get_meta_data_registry(), table_name);
        const TableMetaData &table = key.get_table();
        key.set(table.get_unique_pk(), Value(id));
        return key;
    }
public:
    WeakObject()
        : d_(NULL)
        , mapper_(NULL)
    {}
    WeakObject(Mapper &mapper, const RowData &key)
        : d_(mapper.find(key))
        , mapper_(NULL)
    {}
    WeakObject(Mapper &mapper, const std::string &table_name, long long id)
        : d_(mapper.find(mk_key(mapper, table_name, id)))
        , mapper_(NULL)
    {}
    WeakObject(Mapper &mapper, const std::string &table_name)
        : d_(NULL)
        , new_d_(new RowData(mapper.get_meta_data_registry(), table_name))
        , mapper_(&mapper)
    {}
    void register_in_mapper()
    {
        if (!mapper_)
            throw NoMapperGiven();
        d_ = mapper_->register_as_new(*new_d_);
        new_d_.reset();
        mapper_ = NULL;
    }
    
    virtual const XMLNode auto_xmlize(Mapper &mapper, int deep = 0) const
    {    
        return deep_xmlize(mapper, get_row_data(), deep);
    }
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__DATA_OBJ__INCLUDED
