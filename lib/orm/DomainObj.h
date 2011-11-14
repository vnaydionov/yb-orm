// -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#ifndef YB__ORM__DOMAIN_OBJ__INCLUDED
#define YB__ORM__DOMAIN_OBJ__INCLUDED

#include <stdexcept>
#include <boost/shared_ptr.hpp>
#include "XMLNode.h"
#include "DataObject.h"

namespace Yb {

class NoRawData: public ORMError {
public:
    NoRawData()
        : ORMError(_T("No ROW data is associated with DomainObject"))
    {}
};

class NoSessionBaseGiven: public ORMError {
public:
    NoSessionBaseGiven()
        : ORMError(_T("No session given to the WeakObject"))
    {}
};

class AlreadySavedInAnotherSession: public ORMError {
public:
    AlreadySavedInAnotherSession()
        : ORMError(_T("Object has been already saved in some other session"))
    {}
};

class CouldNotSaveEmptyObject: public ORMError {
public:
    CouldNotSaveEmptyObject()
        : ORMError(_T("Attempt to save an empty object failed"))
    {}
};

class OutOfManagedList: public std::runtime_error {
public:
    OutOfManagedList(int pos, int sz)
        : std::runtime_error(NARROW(_T("Trying to access index ") +
            boost::lexical_cast<String>(pos) + 
            _T(" that falls out of ManagedList of size ") +
            boost::lexical_cast<String>(sz)))
    {}
};

class InvalidIterator: public std::runtime_error {
public:
    InvalidIterator()
        : std::runtime_error(NARROW(_T("Trying to use an invalid iterator")))
    {}
};

template <class T>
class ManagedList {
    RelationObject *ro_;
    DataObject::Ptr master_;

    void check_ptr () const
    {
        if (!ro_)
            throw NoRawData();
    }
    void load_if_needed()
    {
        check_ptr();
        if (ro_->master_object()->status() != DataObject::New &&
            ro_->status() != RelationObject::Sync)
        {
            ro_->lazy_load_slaves();
        }
    }
public:
    template <class U, class V>
    class Iter {
        friend class ManagedList;
        U it_;
        mutable std::auto_ptr<V> d_;
        Iter(U it): it_(it) {}
    public:
        Iter(const Iter &obj): it_(obj.it_) {}
        Iter &operator=(const Iter &obj) {
            if (this != &obj) {
                it_ = obj.it_;
                d_.reset(NULL);
            }
            return *this;
        }
        bool operator==(const Iter &obj) const {
            return it_ == obj.it_;
        }
        bool operator!=(const Iter &obj) const {
            return it_ != obj.it_;
        }
        V &operator*() const {
            if (!d_.get())
                d_.reset(new V(*it_));
            return *d_;
        }
        V *operator->() const {
            if (!d_.get())
                d_.reset(new V(*it_));
            return d_.get();
        }
        Iter &operator++() { ++it_; d_.reset(NULL); return *this; }
        Iter &operator--() { --it_; d_.reset(NULL); return *this; }
        Iter operator++(int) { Iter t(*this); ++it_; d_.reset(NULL); return t; }
        Iter operator--(int) { Iter t(*this); --it_; d_.reset(NULL); return t; }
    };

    typedef Iter<RelationObject::SlaveObjects::iterator, T> iterator;
    typedef Iter<RelationObject::SlaveObjects::const_iterator, const T> const_iterator;

    ManagedList(): ro_(NULL) {}
    ManagedList(RelationObject *ro, DataObject::Ptr m): ro_(ro), master_(m) {
        YB_ASSERT(ro_);
        YB_ASSERT(ro_->master_object() == master_.get());
    }
    RelationObject *relation_object() const { return ro_; }
    DataObject::Ptr master() { return master_; }

    iterator begin() {
        load_if_needed();
        return iterator(ro_->slave_objects().begin());
    }
    iterator end() {
        load_if_needed();
        return iterator(ro_->slave_objects().end());
    }
    iterator find(const T &x) {
        load_if_needed();
        RelationObject::SlaveObjects::iterator
            it = ro_->slave_objects().find(x.data_object());
        return iterator(it);
    }
    const_iterator begin() const {
        load_if_needed();
        return const_iterator
            (((const RelationObject::SlaveObjects &)(ro_->slave_objects())).begin());
    }
    const_iterator end() const {
        load_if_needed();
        return const_iterator
            (((const RelationObject::SlaveObjects &)(ro_->slave_objects())).end());
    }
    size_t size() const {
        check_ptr();
        if (ro_->status() == RelationObject::Sync ||
                ro_->master_object()->status() == DataObject::New)
            return ro_->slave_objects().size();
        ///
        return ro_->count_slaves();
    }
    iterator insert(const T &x) {
        load_if_needed(); // ??
        iterator it = find(x);
        if (it != end())
            return it;
        DataObject::link_master_to_slave(master_, x.data_object(),
                                         ro_->relation_info().attr(0, _T("property")));
        return find(x);
    }
    void erase(iterator it) {
        load_if_needed(); // ??
        (*it.it_)->delete_object();
    }
};

class DomainObject: public XMLizable
{
protected:
    DataObject::Ptr d_;
    void check_ptr() const {
        if (!d_.get())
            throw NoRawData();
    }
public:
    DomainObject()
    {}
    DomainObject(DataObject::Ptr d)
        : d_(d)
    {}
    DomainObject(const Schema &schema, const String &table_name)
        : d_(DataObject::create_new(schema.get_table(table_name)))
    {}
    DomainObject(Session &session, const Key &key)
        : d_(session.get_lazy(key))
    {}
    DomainObject(Session &session, const String &tbl_name, LongInt id)
        : d_(session.get_lazy(session.schema().get_table(tbl_name).mk_key(id)))
    {}
    virtual ~DomainObject() {}
    void save(Session &session) { check_ptr(); session.save(d_); }
    void detach(Session &session) { check_ptr(); session.detach(d_); }
    bool empty() const { return !d_.get(); }
    Session *session() const {
        check_ptr();
        return d_->session();
    }
    DataObject::Ptr data_object() const { return d_; }
    const Value &get(const String &col_name) const {
        check_ptr();
        return d_->get(col_name);
    }
    void set(const String &col_name, const Value &value) {
        check_ptr();
        d_->set(col_name, value);
    }
    const DomainObject get_master(const String &relation_name = _T("")) const {
        check_ptr();
        return DomainObject(DataObject::get_master(d_, relation_name));
    }
    RelationObject *get_slaves_ro(const String &relation_name = _T("")) const {
        check_ptr();
        return d_->get_slaves(relation_name);
    }
    ManagedList<DomainObject> get_slaves(const String &relation_name = _T("")) const {
        check_ptr();
        return ManagedList<DomainObject>(d_->get_slaves(relation_name), d_);
    }
    void link_to_master(DomainObject &master, const String relation_name = _T("")) {
        check_ptr();
        master.check_ptr();
        DataObject::link_slave_to_master(d_, master.d_, relation_name);
    }
    void link_to_slave(DomainObject &slave, const String relation_name = _T("")) {
        check_ptr();
        slave.check_ptr();
        DataObject::link_master_to_slave(d_, slave.d_, relation_name);
    }
    void delete_object() {
        check_ptr();
        d_->delete_object();
    }
    DataObject::Status status() const {
        check_ptr();
        return d_->status();
    }
    const Table &table() const {
        check_ptr();
        return d_->table();
    }
    int cmp(const DomainObject &x) const {
        if (d_ == x.d_)
            return 0;
        if (!d_.get())
            return -1;
        if (!x.d_.get())
            return 1;
        if (d_->values() == x.d_->values())
            return 0;
        return d_->values() < x.d_->values()? -1: 1;
    }
    const XMLNode xmlize(int depth = 0, const String &alt_name = _T("")) const {
        if (!session())
            throw NoSessionBaseGiven();
        return deep_xmlize(*session(), d_, depth, alt_name);
    }
};

template <class T>
class DomainObjectResultSet: public ResultSetBase<T>
{
    DataObjectResultSet rs_;
    std::auto_ptr<DataObjectResultSet::iterator> it_;

    bool fetch(T &row) {
        if (!it_.get())
            it_ = std::auto_ptr<DataObjectResultSet::iterator> 
                (new DataObjectResultSet::iterator(rs_.begin()));
        if (rs_.end() == *it_)
            return false;
        row = T((**it_)[0]);
        ++*it_;
        return true;
    }
    DomainObjectResultSet();
public:
    DomainObjectResultSet(const DataObjectResultSet &rs)
        : rs_(rs)
    {}
    DomainObjectResultSet(const DomainObjectResultSet &obj)
        : rs_(obj.rs_)
    {
        YB_ASSERT(!obj.it_.get());
    }
};

template <class T>
DomainObjectResultSet<T> query(Session &session, const Filter &filter,
        const StrList &order_by = StrList(), int max = -1)
{
    Strings tables(1);
    tables[0] = T::get_table_name();
    return DomainObjectResultSet<T>(session.load_collection(
            tables, filter, order_by, max));
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__DOMAIN_OBJ__INCLUDED
