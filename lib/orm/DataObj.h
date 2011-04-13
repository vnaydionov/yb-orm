#ifndef YB__ORM__DATA_OBJ__INCLUDED
#define YB__ORM__DATA_OBJ__INCLUDED

#include <stdexcept>
#include <boost/shared_ptr.hpp>
#include "Session.h"
#include "XMLNode.h"

namespace Yb {

class NoRawData : public std::logic_error
{
public:
    NoRawData()
        : logic_error("No ROW data is associated with DataObject")
    {}
};

class NoSessionBaseGiven : public std::logic_error
{
public:
    NoSessionBaseGiven()
        : logic_error("No session given to the WeakObject")
    {}
};

class DataObject
{
public:
    virtual ~DataObject()
    {}

    virtual const XMLNode auto_xmlize(SessionBase &session, int deep) const = 0;
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
    static const RowData mk_key(SessionBase &session,
            const std::string &table_name, LongInt id)
    {
        RowData key(session.get_meta_data_registry(), table_name);
        const TableMetaData &table = key.get_table();
        key.set(table.get_synth_pk(), Value(id));
        return key;
    }
public:
    StrongObject()
        : d_(NULL)
    {}
    StrongObject(SessionBase &session, const RowData &key)
        : d_(session.find(key))
    {}
    StrongObject(SessionBase &session, const std::string &table_name, LongInt id)
        : d_(session.find(mk_key(session, table_name, id)))
    {}
    StrongObject(SessionBase &session, const std::string &table_name)
        : d_(session.create(table_name))
    {}

    virtual const XMLNode auto_xmlize(SessionBase &session, int deep = 0) const
    {    
        return deep_xmlize(session, get_row_data(), deep);
    }
};

class WeakObject : public DataObject
{
    RowData *d_;
    boost::shared_ptr<RowData> new_d_;
    SessionBase *session_;
    RowData *ptr() const { return d_? d_: new_d_.get(); }
    static const RowData mk_key(SessionBase &session,
            const std::string &table_name, LongInt id)
    {
        RowData key(session.get_meta_data_registry(), table_name);
        const TableMetaData &table = key.get_table();
        key.set(table.get_unique_pk(), Value(id));
        return key;
    }
public:
    WeakObject()
        : d_(NULL)
        , session_(NULL)
    {}
    WeakObject(SessionBase &session, const RowData &key)
        : d_(session.find(key))
        , session_(NULL)
    {}
    WeakObject(SessionBase &session, const std::string &table_name, LongInt id)
        : d_(session.find(mk_key(session, table_name, id)))
        , session_(NULL)
    {}
    WeakObject(SessionBase &session, const std::string &table_name)
        : d_(NULL)
        , new_d_(new RowData(session.get_meta_data_registry(), table_name))
        , session_(&session)
    {}
    void register_in_session()
    {
        if (!session_)
            throw NoSessionBaseGiven();
        d_ = session_->register_as_new(*new_d_);
        new_d_.reset();
        session_ = NULL;
    }
    
    virtual const XMLNode auto_xmlize(SessionBase &session, int deep = 0) const
    {    
        return deep_xmlize(session, get_row_data(), deep);
    }
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__DATA_OBJ__INCLUDED
