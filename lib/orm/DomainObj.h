// -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#ifndef YB__ORM__DOMAIN_OBJ__INCLUDED
#define YB__ORM__DOMAIN_OBJ__INCLUDED

#include <stdexcept>
#include <boost/shared_ptr.hpp>
#include "Session.h"
#include "XMLNode.h"
#include "DataObject.h"

namespace Yb {

class NoRawData: public ORMError {
public:
    NoRawData()
        : ORMError("No ROW data is associated with DomainObject")
    {}
};

class NoSessionBaseGiven: public ORMError {
public:
    NoSessionBaseGiven()
        : ORMError("No session given to the WeakObject")
    {}
};

class AlreadySavedInAnotherSession: public ORMError {
public:
    AlreadySavedInAnotherSession()
        : ORMError("Object has been already saved in some other session")
    {}
};

class CouldNotSaveEmptyObject: public ORMError {
public:
    CouldNotSaveEmptyObject()
        : ORMError("Attempt to save an empty object failed")
    {}
};

class OutOfManagedList: public std::runtime_error {
public:
    OutOfManagedList(int pos, int sz)
        : std::runtime_error("Trying to access index " +
            boost::lexical_cast<std::string>(pos) + 
            " that falls out of ManagedList of size " +
            boost::lexical_cast<std::string>(sz))
    {}
};

class InvalidIterator: public std::runtime_error {
public:
    InvalidIterator()
        : std::runtime_error("Trying to use an invalid iterator")
    {}
};

//class ManagedList;

class DomainObjectV2: public XMLizable
{
    DataObject::Ptr d_;
    void check_ptr() const {
        if (!d_.get())
            throw NoRawData();
    }
public:
    DomainObjectV2()
    {}
    DomainObjectV2(DataObject::Ptr d)
        : d_(d)
    {}
    DomainObjectV2(const Schema &schema, const std::string &table_name)
        : d_(DataObject::create_new(schema.get_table(table_name)))
    {}
    DomainObjectV2(SessionV2 &session, const Key &key)
        : d_(session.get_lazy(key))
    {}
    virtual ~DomainObjectV2() {}
    void save(SessionV2 &session) { check_ptr(); session.save(d_); }
    void detach(SessionV2 &session) { check_ptr(); session.detach(d_); }
    bool empty() const { return !d_.get(); }
    SessionV2 *session() const {
        check_ptr();
        return d_->session();
    }
    DataObject::Ptr data_object() { return d_; }
    const Value &get(const std::string &col_name) const {
        check_ptr();
        return d_->get(col_name);
    }
    void set(const std::string &col_name, const Value &value) {
        check_ptr();
        d_->set(col_name, value);
    }
    const DomainObjectV2 get_master(const std::string &relation_name = "") const {
        check_ptr();
        return DomainObjectV2(DataObject::get_master(d_, relation_name));
    }
    void link_to_master(DomainObjectV2 &master, const std::string relation_name = "") {
        check_ptr();
        master.check_ptr();
        DataObject::link_slave_to_master(d_, master.d_, relation_name);
    }
    void link_to_slave(DomainObjectV2 &slave, const std::string relation_name = "") {
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
    int cmp(const DomainObjectV2 &x) const {
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
    const XMLNode xmlize(int depth = 0, const std::string &alt_name = "") const {
        if (!session())
            throw NoSessionBaseGiven();
        return deep_xmlize(*session(), d_, depth, alt_name);
    }
};

class DomainObject: public XMLizable
{
    RowData *d_;
    boost::shared_ptr<RowData> new_d_;
    SessionBase *session_;
    RowData *ptr() const { return d_? d_: new_d_.get(); }
    RowData *checked_ptr() const {
        RowData *d = ptr();
        if (!d)
            throw NoRawData();
        return d;
    }
public:
    DomainObject()
        : d_(NULL)
        , session_(NULL)
    {}
    DomainObject(std::auto_ptr<RowData> data)
        : d_(NULL)
        , new_d_(data.release())
        , session_(NULL)
    {}
    DomainObject(const Schema &schema, const std::string &table_name)
        : d_(NULL)
        , new_d_(new RowData(schema, table_name))
        , session_(NULL)
    {}
    DomainObject(SessionBase &session, const RowData &key)
        : d_(session.find(key))
        , session_(&session)
    {}
    DomainObject(SessionBase &session, const std::string &table_name, LongInt id)
        : d_(session.find(mk_key(session.get_schema(), table_name, id)))
        , session_(&session)
    {}
    DomainObject(SessionBase &session, const std::string &table_name)
        : d_(session.create(table_name))
        , session_(&session)
    {}
    virtual ~DomainObject() {}
    void save(SessionBase &session) {
        if (session_) {
            if (session_ != &session)
                throw AlreadySavedInAnotherSession();
        }
        else {
            if (!new_d_.get())
                throw CouldNotSaveEmptyObject();
            d_ = session.register_as_new(*new_d_);
            new_d_.reset();
            session_ = &session;
        }
    }
    bool empty() const { return ptr() == NULL; }
    bool saved() const { return d_ != NULL; }
    SessionBase *get_session() const { return session_; }
    RowData &row_data() { return *checked_ptr(); }
    const RowData &row_data() const { return *checked_ptr(); }
    const Value &get_attr(const std::string &col_name) const
        { return row_data().get(col_name); }
    void set_attr(const std::string &col_name, const Value &value)
        { return row_data().set(col_name, value); }
    int cmp(const DomainObject &x) const {
        RowData *aptr = ptr(), *bptr = x.ptr();
        if (!aptr && !bptr)
            return 0;
        if (!aptr)
            return -1;
        if (!bptr)
            return 1;
        return aptr->cmp(*bptr, true);
    }
    const XMLNode xmlize(int depth = 0,
            const std::string &alt_name = "") const
    {
        if (!session_)
            throw NoSessionBaseGiven();
        return deep_xmlize(*session_, row_data(), depth, alt_name);
    }
};

template <class T>
class ManagedList {
    enum PersistStatus { New, Ghost, Dirty, Sync };
    typedef std::vector<boost::shared_ptr<DomainObject> > Vector;
    mutable Vector d_;
    mutable PersistStatus status_;
    SessionBase *session_;
    Filter filter_;

    void load() const {
        if (!session_)
            throw NoSessionBaseGiven();
        typename T::ListPtr result = T::find(*session_, filter_);
        typename T::List::const_iterator
            it = result->begin(), end = result->end();
        for (; it != end; ++it)
            d_.push_back(boost::shared_ptr<DomainObject> (new T(*it)));
    }

    void load_if_needed() const {
        if (status_ == Ghost) {
            load();
            status_ = Sync;
        }
    }

public:
    template <class V, class U>
    class Iter {
        friend class ManagedList;
        V &d_;
        int pos_, step_;
        Iter(V &d, int pos, int step)
            : d_(d)
            , pos_(pos)
            , step_(step)
        {}
        void check_pos() const {
            if (pos_ < 0 || pos_ >= d_.size())
                throw OutOfManagedList(pos_, d_.size());
        }
    public:
        Iter(const Iter &obj)
            : d_(obj.d_)
            , pos_(obj.pos_)
            , step_(obj.step_)
        {}
        Iter &operator=(const Iter &obj) {
            if (this != &obj) {
                d_ = obj.d_;
                pos_ = obj.pos_;
                step_ = obj.step_;
            }
        }
        bool operator==(const Iter &obj) const {
            return &d_ == &obj.d_ && pos_ == obj.pos_;
        }
        bool operator!=(const Iter &obj) const {
            return &d_ != &obj.d_ || pos_ != obj.pos_;
        }
        U &operator*() const {
            check_pos();
            return *dynamic_cast<U *> (d_[pos_].get());
        }
        U *operator->() const {
            check_pos();
            return dynamic_cast<U *> (d_[pos_].get());
        }
        Iter &operator++() { pos_ += step_; return *this; }
        Iter &operator--() { pos_ -= step_; return *this; }
        Iter operator++(int) { Iter t(*this); pos_ += step_; return t; }
        Iter operator--(int) { Iter t(*this); pos_ -= step_; return t; }
    };
    typedef Iter<Vector, T> iterator;
    typedef Iter<const Vector, const T> const_iterator;

    ManagedList(SessionBase *session, const Filter &filter)
        : session_(session)
        , filter_(filter)
        , status_(Ghost)
    {}

    ManagedList()
        : session_(NULL)
        , status_(New)
    {}

    iterator begin() {
        load_if_needed();
        return iterator(d_, 0, 1);
    }
    iterator end() {
        load_if_needed();
        return iterator(d_, d_.size(), 1);
    }
    iterator find(const T &x) {
        load_if_needed();
        for (size_t i = 0; i < d_.size(); ++i) {
            if (!d_[i]->row_data().cmp(
                    dynamic_cast<const DomainObject & >(x).row_data(), true))
                return iterator(d_, i, 1);
        }
        return end();
    }
    const_iterator begin() const {
        load_if_needed();
        return const_iterator(d_, 0, 1);
    }
    const_iterator end() const {
        load_if_needed();
        return const_iterator(d_, d_.size(), 1);
    }
    size_t size() const {
        load_if_needed();
        return d_.size();
    }
    iterator insert(const T &x) {
        load_if_needed();
        iterator it = find(x);
        if (it != end())
            return it;
        d_.push_back(boost::shared_ptr<DomainObject> (new T(x)));
        // do some magic
        return iterator(d_, d_.size() - 1, 1);
    }
    void erase(iterator it) {
        load_if_needed();
        if (&it.d_ != &d_)
            throw InvalidIterator();
        it.check_pos();
        // do some magic
        d_.erase(d_.begin() + it.pos_);
    }
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__DOMAIN_OBJ__INCLUDED
