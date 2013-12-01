// -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#ifndef YB__ORM__DOMAIN_OBJECT__INCLUDED
#define YB__ORM__DOMAIN_OBJECT__INCLUDED

#include <iterator>
#include <stdexcept>
#include <orm/XMLizer.h>
#include <orm/DataObject.h>
#if defined(YB_USE_TUPLE)
#include <boost/tuple/tuple.hpp>
#endif

namespace Yb {

class NoDataObject: public ORMError {
public:
    NoDataObject()
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
class ManagedList;

class DomainObject: public XMLizable
{
    static char init_[16];
    void check_ptr() const;

    DataObject::Ptr d_;
    DomainObject *owner_;
    String prop_name_;
protected:
    explicit DomainObject(const Table &table,
            DomainObject *owner, const String &prop_name);
public:
    DataObject::Ptr get_data_object(bool check = true) const;
    void set_data_object(DataObject::Ptr d);
    DomainObject();
    explicit DomainObject(DataObject::Ptr d);
    explicit DomainObject(const Table &table);
    explicit DomainObject(const Schema &schema, const String &table_name);
    explicit DomainObject(Session &session, const Key &key);
    explicit DomainObject(Session &session, const String &tbl_name, LongInt id);
    DomainObject(const DomainObject &obj);
    virtual ~DomainObject();
    DomainObject &operator=(const DomainObject &obj);
    void save(Session &session);
    void detach(Session &session);
    bool is_empty() const;
    Session *get_session() const;
    const Value &get(const String &col_name) const;
    void set(const String &col_name, const Value &value);
    const Value &get(int col_num) const;
    void set(int col_num, const Value &value);
    const DomainObject get_master(const String &relation_name = _T("")) const;
    RelationObject *get_slaves_ro(const String &relation_name = _T("")) const;
    ManagedList<DomainObject> get_slaves(
            const String &relation_name = _T("")) const;
    void link_to_master(DomainObject &master,
            const String relation_name = _T(""));
    void link_to_slave(DomainObject &slave,
            const String relation_name = _T(""));
    void delete_object();
    DataObject::Status status() const;
    const Table &get_table() const;
    int cmp(const DomainObject &x) const;
    ElementTree::ElementPtr xmlize(
            int depth = 0, const String &alt_name = _T("")) const;
    static bool register_table_meta(Table::Ptr tbl);
    static bool register_relation_meta(Relation::Ptr rel);
    static void save_registered(Schema &schema);
};

typedef IntrusivePtr<DomainObject> DomainObjectPtr;

inline bool operator==(const DomainObject &a, const DomainObject &b)
{ return !a.cmp(b); }
inline bool operator!=(const DomainObject &a, const DomainObject &b)
{ return a.cmp(b); }
inline bool operator<(const DomainObject &a, const DomainObject &b)
{ return a.cmp(b)<0; }
inline bool operator>(const DomainObject &a, const DomainObject &b)
{ return a.cmp(b)>0; }
inline bool operator<=(const DomainObject &a, const DomainObject &b)
{ return !(a.cmp(b)>0); }
inline bool operator>=(const DomainObject &a, const DomainObject &b)
{ return !(a.cmp(b)<0); }

template <class T>
class ManagedList;

template <class U__, class V__>
class ManagedListIter: public std::iterator<std::forward_iterator_tag,
        V__, ptrdiff_t, V__ *, V__ & >
{
    U__ it_;
    mutable std::auto_ptr<V__> d_;
public:
    ManagedListIter() {}
    ManagedListIter(const ManagedListIter &obj): it_(obj.it_) {}
    ManagedListIter &operator=(const ManagedListIter &obj) {
        if (this != &obj) {
            it_ = obj.it_;
            d_.reset(NULL);
        }
        return *this;
    }
    ManagedListIter(U__ it): it_(it) {}
    bool operator==(const ManagedListIter &obj) const {
        return it_ == obj.it_;
    }
    bool operator!=(const ManagedListIter &obj) const {
        return it_ != obj.it_;
    }
    V__ &operator*() const {
        if (!d_.get())
            d_.reset(new V__(*it_));
        return *d_;
    }
    V__ *operator->() const {
        if (!d_.get())
            d_.reset(new V__(*it_));
        return d_.get();
    }
    ManagedListIter &operator++() { ++it_; d_.reset(NULL); return *this; }
    ManagedListIter &operator--() { --it_; d_.reset(NULL); return *this; }
    ManagedListIter operator++(int) { ManagedListIter t(*this); ++it_; d_.reset(NULL); return t; }
    ManagedListIter operator--(int) { ManagedListIter t(*this); --it_; d_.reset(NULL); return t; }
    void delete_object() {
        (*it_)->delete_object();
    }
};

template <class T>
class ManagedList {
    RelationObject *ro_;
    DataObject::Ptr master_;
    DomainObject *owner_;
    String prop_name_;

    static RelationObject *load_if_needed(RelationObject *ro)
    {
        if (ro->master_object()->status() != DataObject::New &&
            ro->status() != RelationObject::Sync)
        {
            ro->lazy_load_slaves();
        }
        return ro;
    }
public:
    typedef ManagedListIter<RelationObject::SlaveObjects::iterator, T> iterator;
    typedef ManagedListIter<RelationObject::SlaveObjects::const_iterator, const T> const_iterator;

    ManagedList(): ro_(NULL), owner_(NULL) {}
    ManagedList(RelationObject *ro, DataObject::Ptr m)
        : ro_(ro), master_(m), owner_(NULL)
    {
        YB_ASSERT(ro_);
        YB_ASSERT(ro_->master_object() == shptr_get(master_));
    }
    ManagedList(DomainObject *owner, const String &prop_name)
        : ro_(NULL), owner_(owner), prop_name_(prop_name)
    {
        YB_ASSERT(owner_);
    }
    ManagedList(const ManagedList &obj)
        : ro_(obj.relation_object()), master_(obj.master()), owner_(NULL)
    {}
    ManagedList &operator=(const ManagedList &obj)
    {
        if (this != &obj) {
            YB_ASSERT(!owner_);
            ro_ = obj.relation_object();
            master_ = obj.master();
        }
        return *this;
    }
    RelationObject *relation_object(bool check = true) const {
        RelationObject *ro = ro_;
        if (owner_)
            ro = owner_->get_slaves_ro(prop_name_);
        if (check && !ro)
            throw NoDataObject();
        return ro;
    }
    DataObject::Ptr master() const {
        if (owner_)
            return owner_->get_data_object();
        return master_;
    }

    iterator begin() {
        return iterator(load_if_needed(relation_object())
                ->slave_objects().begin());
    }
    iterator end() {
        return iterator(load_if_needed(relation_object())
                ->slave_objects().end());
    }
    iterator find(const T &x) {
        typename RelationObject::SlaveObjects::iterator it
            = load_if_needed(relation_object())->find(shptr_get(x.get_data_object()));
        return iterator(it);
    }
    const_iterator begin() const {
        return const_iterator
            (((const RelationObject::SlaveObjects &)
              (load_if_needed(relation_object())->slave_objects())).begin());
    }
    const_iterator end() const {
        return const_iterator
            (((const RelationObject::SlaveObjects &)
              (load_if_needed(relation_object())->slave_objects())).end());
    }
    size_t size() const {
        RelationObject *ro = relation_object();
        if (ro->status() == RelationObject::Sync ||
                ro->master_object()->status() == DataObject::New)
            return ro->slave_objects().size();
        ///
        return ro->count_slaves();
    }
    iterator insert(const T &x) {
        RelationObject *ro = relation_object();
        load_if_needed(ro); // ??
        iterator it = find(x);
        if (it != end())
            return it;
        DataObject::link_master_to_slave(master(), x.get_data_object(),
                                         ro->relation_info().attr(0, _T("property")));
        return find(x);
    }
    void erase(iterator it) {
        RelationObject *ro = relation_object();
        load_if_needed(ro); // ??
        //(*it.it_)->delete_object();
        it.delete_object();
    }
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
        const Value &v = pobj_->get(ColNum);
        YB_ASSERT(!v.is_null());
        U u;
        return from_variant(v, u);
    }
    const T &value() {
        YB_ASSERT(pobj_ != NULL);
        const Value &v = pobj_->get(ColNum);
        YB_ASSERT(!v.is_null());
        return v.read_as<T>();
    }
    operator const T & () {
        return value();
    }
    operator Value ()
    {
        YB_ASSERT(pobj_ != NULL);
        return pobj_->get(ColNum);
    }
    bool is_null()
    {
        YB_ASSERT(pobj_ != NULL);
        return pobj_->get(ColNum).is_null();
    }
};

template <class DObj>
class DomainObjHolder {
    mutable std::auto_ptr<DObj> p_;
    Yb::DomainObject *owner_;
    const Yb::String prop_name_;
    typedef ManagedList<DObj> SlaveList;
    mutable std::auto_ptr<SlaveList> slaves_;
    bool one2one_;
public:
    DomainObjHolder(Yb::DomainObject *owner, const Yb::String &prop_name,
            bool one2one = false)
        : owner_(owner)
        , prop_name_(prop_name)
        , one2one_(one2one)
    {}
    DomainObjHolder()
        : p_(new DObj), owner_(NULL), one2one_(false) {}
    explicit DomainObjHolder(Yb::Session &session)
        : p_(new DObj(session)), owner_(NULL), one2one_(false) {}
    explicit DomainObjHolder(Yb::DataObject::Ptr d)
        : p_(new DObj(d)), owner_(NULL), one2one_(false) {}
    explicit DomainObjHolder(DObj &d)
        : p_(new DObj(d.get_data_object())), owner_(NULL), one2one_(false) {}
    DomainObjHolder(Yb::Session &session, const Yb::Key &key)
        : p_(new DObj(session, key)), owner_(NULL), one2one_(false) {}
    DomainObjHolder(Yb::Session &session, Yb::LongInt id)
        : p_(new DObj(session, id)), owner_(NULL), one2one_(false) {}
    DomainObjHolder(const DomainObjHolder &other)
        : p_(new DObj(other._get_p()->get_data_object()))
        , owner_(NULL), one2one_(false) {}
    DomainObjHolder &operator=(const DomainObjHolder &other)
    {
        if (this != &other) {
            if (one2one_) {
                if (!slaves_.get())
                    slaves_.reset(new SlaveList(owner_, prop_name_));
                p_.reset(new DObj(*other));
                slaves_->insert(*p_);
            }
            else
                *_get_p() = *other._get_p();
        }
        return *this;
    }
    bool operator==(const DomainObjHolder &other) const
    {
        return *_get_p() == *other._get_p();
    }
    DObj *_get_p() const {
        if (!p_.get() && owner_) {
            if (one2one_) {
                slaves_.reset(new SlaveList(owner_, prop_name_));
                p_.reset(new DObj(*slaves_->begin()));
            }
            else
                p_.reset(new DObj(owner_, prop_name_));
        }
        return p_.get();
    }
    DObj &operator * () const { return *_get_p(); }
    DObj *operator->() const { return _get_p(); }
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
    static void list_tables(Strings &tables)
    {
        typename R::inherited tuple;
        tuple_tables(tuple, tables);
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
    static void list_tables(Strings &tables)
    {
        tables.push_back(R::get_table_name());
    }
};

template <class R>
class QueryObj {
    typedef QueryFunc<R> QF;
    Session *session_;
    Expression filter_, order_;
    bool for_update_;
public:
    QueryObj(Session &session, const Expression &filter = Expression(),
            const Expression &order = Expression(), bool for_update = false)
        : session_(&session)
        , filter_(filter)
        , order_(order)
        , for_update_(for_update)
    {}
    QueryObj filter_by(const Expression &filter) {
        QueryObj q(*this);
        if (q.filter_.is_empty())
            q.filter_ = filter;
        else
            q.filter_ = q.filter_ && filter;
        return q;
    }
    QueryObj order_by(const Expression &order) {
        QueryObj q(*this);
        q.order_ = order;
        return q;
    }
    QueryObj for_update() {
        QueryObj q(*this);
        q.for_update_ = true;
        return q;
    }
    SelectExpr get_select() {
        Strings tables;
        QF::list_tables(tables);
        return make_select(session_->schema(),
                session_->schema().join_expr(tables),
                filter_, order_, for_update_);
    }
    DomainResultSet<R> all() {
        Strings tables;
        QF::list_tables(tables);
        return DomainResultSet<R>(session_->load_collection(
                    session_->schema().join_expr(tables),
                    filter_, order_, for_update_));
    }
    R one() {
        Strings tables;
        QF::list_tables(tables);
        DomainResultSet<R> r = session_->load_collection(
                    session_->schema().join_expr(tables),
                    filter_, order_, for_update_);
        typename DomainResultSet<R>::iterator it = r.begin();
        if (it == r.end())
            throw NoDataFound("No data");
        R result = *it;
        if (++it != r.end())
            throw NoDataFound("More than one row");
        return result;
    }
    LongInt count() {
        SelectExpr select(Expression(_T("COUNT(*) CNT")));
        select.from_(ColumnExpr(get_select(), _T("X")));
        SqlResultSet rs = session_->engine()->select_iter(select);
        Row r = *rs.begin(); 
        return r[0].second.as_longint();
    }
};

template <class R>
QueryObj<R> query(Session &session, const Expression &filter = Expression(),
    const Expression &order = Expression())
{
    return QueryObj<R>(session, filter, order);
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
#endif // YB__ORM__DOMAIN_OBJECT__INCLUDED
