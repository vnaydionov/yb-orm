// -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#ifndef YB__ORM__DOMAIN_OBJ__INCLUDED
#define YB__ORM__DOMAIN_OBJ__INCLUDED

#include <stdexcept>
#include <orm/XMLNode.h>
#include <orm/DataObject.h>
#if defined(YB_USE_TUPLE)
#include <boost/tuple/tuple.hpp>
#endif

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
        : std::runtime_error("Trying to access index " +
            to_stdstring(pos) + 
            " that falls out of ManagedList of size " +
            to_stdstring(sz))
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
        YB_ASSERT(ro_->master_object() == shptr_get(master_));
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
    static char init_[16];
    void check_ptr() const {
        if (!shptr_get(d_))
            throw NoRawData();
    }
    DataObject::Ptr d_;
    DomainObject *owner_;
    String prop_name_;
protected:
    explicit DomainObject(const Table &table,
            DomainObject *owner, const String &prop_name)
        : owner_(owner)
        , prop_name_(prop_name)
    {}
public:
    DataObject::Ptr get_data_object(bool check = true) const
    {
        if (owner_) {
            owner_->check_ptr();
            return DataObject::get_master(owner_->d_, prop_name_);
        }
        if (check)
            check_ptr();
        return d_;
    }
    void set_data_object(DataObject::Ptr d)
    {
        if (owner_) {
            owner_->check_ptr();
            DomainObject master(d);
            owner_->link_to_master(master, prop_name_);
        }
        else
            d_ = d;
    }
    DomainObject()
        : owner_(NULL)
    {}
    explicit DomainObject(DataObject::Ptr d)
        : d_(d)
        , owner_(NULL)
    {}
    explicit DomainObject(const Table &table)
        : d_(DataObject::create_new(table))
        , owner_(NULL)
    {}
    explicit DomainObject(const Schema &schema, const String &table_name)
        : d_(DataObject::create_new(schema.table(table_name)))
        , owner_(NULL)
    {}
    explicit DomainObject(Session &session, const Key &key)
        : d_(session.get_lazy(key))
        , owner_(NULL)
    {}
    explicit DomainObject(Session &session, const String &tbl_name, LongInt id)
        : d_(session.get_lazy(session.schema().table(tbl_name).mk_key(id)))
        , owner_(NULL)
    {}
    DomainObject(const DomainObject &obj)
        : d_(obj.get_data_object())
        , owner_(NULL)
    {}
    virtual ~DomainObject() {}
    DomainObject &operator=(const DomainObject &obj)
    {
        if (this != &obj)
            set_data_object(obj.get_data_object());
        return *this;
    }
    void save(Session &session) { session.save(get_data_object()); }
    void detach(Session &session) { session.detach(get_data_object()); }
    bool is_empty() const { return !shptr_get(get_data_object(false)); }
    Session *get_session() const { return get_data_object()->session(); }
    const Value &get(const String &col_name) const {
        return get_data_object()->get(col_name);
    }
    void set(const String &col_name, const Value &value) {
        get_data_object()->set(col_name, value);
    }
    const Value &get(int col_num) const {
        return get_data_object()->get(col_num);
    }
    void set(int col_num, const Value &value) {
        get_data_object()->set(col_num, value);
    }
    const DomainObject get_master(const String &relation_name = _T("")) const {
        return DomainObject(
                DataObject::get_master(get_data_object(), relation_name));
    }
    RelationObject *get_slaves_ro(const String &relation_name = _T("")) const {
        return get_data_object()->get_slaves(relation_name);
    }
    ManagedList<DomainObject> get_slaves(const String &relation_name = _T(""))
        const
    {
        DataObject::Ptr p = get_data_object();
        return ManagedList<DomainObject>(p->get_slaves(relation_name), p);
    }
    void link_to_master(DomainObject &master,
            const String relation_name = _T(""))
    {
        DataObject::link_slave_to_master(get_data_object(),
                master.get_data_object(), relation_name);
    }
    void link_to_slave(DomainObject &slave,
            const String relation_name = _T(""))
    {
        DataObject::link_master_to_slave(get_data_object(),
                slave.get_data_object(), relation_name);
    }
    void delete_object() {
        get_data_object()->delete_object();
    }
    DataObject::Status status() const {
        return get_data_object()->status();
    }
    const Table &get_table() const {
        return get_data_object()->table();
    }
    int cmp(const DomainObject &x) const {
        DataObject::Ptr p1 = get_data_object(false), p2 = x.get_data_object(false);
        if (p1 == p2)
            return 0;
        if (!shptr_get(p1))
            return -1;
        if (!shptr_get(p2))
            return 1;
        if (p1->values() == p2->values())
            return 0;
        return p1->values() < p2->values()? -1: 1;
    }
    ElementTree::ElementPtr xmlize(int depth = 0, const String &alt_name = _T("")) const {
        Session *s = get_session();
        if (!s)
            throw NoSessionBaseGiven();
        return deep_xmlize(*s, get_data_object(), depth, alt_name);
    }
    static bool register_table_meta(Table::Ptr tbl);
    static bool register_relation_meta(Relation::Ptr rel);
    static void save_registered(Schema &schema);
};

template <class T, int ColNum>
class Property {
    DomainObject *pobj_;
public:
    Property(DomainObject *pobj = NULL)
        : pobj_(pobj)
    {}
    Property &operator=(const Property &prop)
    {
        if (this != &prop && (pobj_ || prop.pobj_)) {
            YB_ASSERT(pobj_ != NULL);
            YB_ASSERT(prop.pobj_ != NULL);
            pobj_->set(ColNum, prop.pobj_->get(ColNum));
        }
        return *this;
    }
    T operator=(const T &value)
    {
        YB_ASSERT(pobj_ != NULL);
        pobj_->set(ColNum, Value(value));
        return value;
    }
    template <class U>
    U as()
    {
        YB_ASSERT(pobj_ != NULL);
        Value v = pobj_->get(ColNum);
        YB_ASSERT(!v.is_null());
        U u;
        return from_variant(v, u);
    }
    T value() { return as<T>(); }
    operator T() { return as<T>(); }
    bool is_null()
    {
        YB_ASSERT(pobj_ != NULL);
        return pobj_->get(ColNum).is_null();
    }
};

template <class DObj>
class DomainObjHolder {
    std::auto_ptr<DObj> p_;
public:
    DomainObjHolder(Yb::DomainObject *owner, const Yb::String &prop_name)
        : p_(new DObj(owner, prop_name)) {}
    DomainObjHolder()
        : p_(new DObj) {}
    explicit DomainObjHolder(Yb::Session &session)
        : p_(new DObj(session)) {}
    explicit DomainObjHolder(Yb::DataObject::Ptr d)
        : p_(new DObj(d)) {}
    DomainObjHolder(Yb::Session &session, const Yb::Key &key)
        : p_(new DObj(session, key)) {}
    DomainObjHolder(Yb::Session &session, Yb::LongInt id)
        : p_(new DObj(session, id)) {}
    DomainObjHolder(const DomainObjHolder &other)
        : p_(new DObj(other.p_->get_data_object())) {}
    DomainObjHolder &operator=(const DomainObjHolder &other)
    {
        if (this != &other) {
            *p_ = *other.p_;
        }
        return *this;
    }
    bool operator==(const DomainObjHolder &other) const
    {
        return *p_ == *other.p_;
    }
    DObj *operator->() const { return p_.get(); }
};

template <class R>
class DomainResultSet;

template <class R>
struct QueryFunc;

#if defined(YB_USE_TUPLE)

template <class H>
boost::tuples::cons<H, boost::tuples::null_type>
row2tuple(const boost::tuples::cons<H, boost::tuples::null_type> &,
    const ObjectList &row, int pos)
{
    boost::tuples::null_type tail;
    return boost::tuples::cons<H, boost::tuples::null_type>(H(row[pos]), tail);
}

template <class H, class T>
boost::tuples::cons<H, T>
row2tuple(const boost::tuples::cons<H, T> &item,
    const ObjectList &row, int pos)
{
    return boost::tuples::cons<H, T>(H(row[pos]),
        row2tuple(item.get_tail(), row, pos + 1));
}

template <class T0, class T1, class T2, class T3, class T4,
          class T5, class T6, class T7, class T8, class T9>
class DomainResultSet<boost::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9> >
    : public ResultSetBase<boost::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9> >
{
    typedef boost::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9> R;
    DataObjectResultSet rs_;
    std::auto_ptr<DataObjectResultSet::iterator> it_;

    bool fetch(R &row) {
        if (!it_.get())
            it_.reset(
                    new DataObjectResultSet::iterator(rs_.begin()));
        if (rs_.end() == *it_)
            return false;
        typename R::inherited tuple;
        row = row2tuple(tuple, **it_, 0);
        ++*it_;
        return true;
    }
    DomainResultSet();
public:
    DomainResultSet(const DataObjectResultSet &rs)
        : rs_(rs)
    {}
    DomainResultSet(const DomainResultSet &obj)
        : rs_(obj.rs_)
    {
        YB_ASSERT(!obj.it_.get());
    }
};

template <class H>
void tuple_tables(const boost::tuples::cons<H, boost::tuples::null_type> &, Strings &tables)
{
    tables.push_back(H::get_table_name());
}

template <class H, class T>
void tuple_tables(const boost::tuples::cons<H, T> &item, Strings &tables)
{
    tables.push_back(H::get_table_name());
    tuple_tables(item.get_tail(), tables);
}

template <class T0, class T1, class T2, class T3, class T4,
          class T5, class T6, class T7, class T8, class T9>
struct QueryFunc<boost::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9> > {
    typedef boost::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9> R;
    typedef DomainResultSet<R> RS;
    static RS query(Session &session, const Filter &filter,
         const StrList &order_by, int max)
    {
        Strings tables;
        typename R::inherited tuple;
        tuple_tables(tuple, tables);
        return RS(session.load_collection(tables, filter, order_by, max));
    }
};
#endif // defined(YB_USE_TUPLE)

template <class R>
class DomainResultSet: public ResultSetBase<R>
{
    DataObjectResultSet rs_;
    std::auto_ptr<DataObjectResultSet::iterator> it_;

    bool fetch(R &row) {
        if (!it_.get())
            it_.reset(
                    new DataObjectResultSet::iterator(rs_.begin()));
        if (rs_.end() == *it_)
            return false;
        row = R((**it_)[0]);
        ++*it_;
        return true;
    }
    DomainResultSet();
public:
    DomainResultSet(const DataObjectResultSet &rs)
        : rs_(rs)
    {}
    DomainResultSet(const DomainResultSet &obj)
        : rs_(obj.rs_)
    {
        YB_ASSERT(!obj.it_.get());
    }
};

template <class R>
struct QueryFunc {
    typedef DomainResultSet<R> RS;
    static RS query(Session &session, const Filter &filter,
         const StrList &order_by, int max)
    {
        Strings tables(1);
        tables[0] = R::get_table_name();
        return RS(session.load_collection(tables, filter, order_by, max));
    }
};

template <class R>
class QueryObj {
    typedef QueryFunc<R> QF;
    Session &session_;
    Filter filter_;
    StrList order_;
    int max_;
public:
    QueryObj(Session &session, const Filter &filter = Filter(),
            const StrList &order = StrList(), int max = -1)
        : session_(session)
        , filter_(filter)
        , order_(order)
        , max_(max)
    {}
    QueryObj filter_by(const Filter &filter) {
        QueryObj q(*this);
        if (q.filter_.is_empty())
            q.filter_ = filter;
        else
            q.filter_ = q.filter_ && filter;
        return q;
    }
    QueryObj order_by(const StrList &order) {
        QueryObj q(*this);
        q.order_ = order;
        return q;
    }
    QueryObj max_rows(int max) {
        QueryObj q(*this);
        q.max_ = max;
        return q;
    }
    DomainResultSet<R> all() {
        return QF::query(session_, filter_, order_, max_);
    }
};

template <class R>
QueryObj<R> query(Session &session, const Filter &filter = Filter(),
    const StrList &order = StrList(), int max = -1)
{
    return QueryObj<R>(session, filter, order, max);
}

template <class Obj>
class DomainMetaDataCreator {
public:
    DomainMetaDataCreator(Tables &tbls, Relations &rels) {
        Obj::create_tables_meta(tbls);
        Obj::create_relations_meta(rels);
        Tables::iterator i = tbls.begin(), iend = tbls.end();
        for (; i != iend; ++i)
            DomainObject::register_table_meta(*i);
        Relations::iterator j = rels.begin(), jend = rels.end();
        for (; j != jend; ++j)
            DomainObject::register_relation_meta(*j);
    }
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__DOMAIN_OBJ__INCLUDED
