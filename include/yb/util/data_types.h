// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__UTIL__DATA_TYPES__INCLUDED
#define YB__UTIL__DATA_TYPES__INCLUDED

#include <algorithm>
#include <iterator>
#include <vector>
#include <set>
#include <map>
#include <util/Utility.h>
#include <util/String.h>
#include <util/Exception.h>
#include <util/decimal.h>
#include <sstream>

#if defined(YB_USE_WX)
#include <wx/datetime.h>
#elif defined(YB_USE_QT)
#include <QDateTime>
#else
#include <boost/date_time/posix_time/posix_time.hpp>
#endif

namespace Yb {

template <class T>
inline const std::string to_stdstring(const T &x)
{
    std::ostringstream out;
    out << x;
    return out.str();
}
#if defined(YB_STRING_NOSTD)
inline const std::string to_stdstring(const String &x) { return str2std(x); }
#endif
inline const std::string &to_stdstring(const std::string &x) { return x; }

template <class T>
inline const String to_string(const T &x) { return WIDEN(to_stdstring(x)); }
#if defined(YB_STRING_NOSTD)
inline const String to_string(const std::string &x) { return std2str(x); }
#endif
inline const String &to_string(const String &x) { return x; }

template <class T>
inline T &from_stdstring(const std::string &s, T &x)
{
    if (!s.size())
        throw ValueBadCast(WIDEN(s), _T("value error: can't cast an empty string"));
    std::istringstream inp(s);
    inp >> x;
    if (!inp.eof())
        throw ValueBadCast(WIDEN(s), _T("value error: extra characters left"));
    return x;
}
#if defined(YB_STRING_NOSTD)
inline String &from_stdstring(const std::string &s, String &x)
{
    x = std2str(s);
    return x;
}
#endif
inline std::string &from_stdstring(const std::string &s, std::string &x)
{
    x = s;
    return x;
}

template <class T>
inline T &from_string(const String &s, T &x)
{
    return from_stdstring(NARROW(s), x);
}
#if defined(YB_STRING_NOSTD)
inline std::string &from_string(const String &s, std::string &x)
{
    x = str2std(s);
    return x;
}
#endif
inline String &from_string(const String &s, String &x)
{
    x = s;
    return x;
}

#if defined(YB_USE_WX)
typedef wxDateTime DateTime;
inline const DateTime now() { return wxDateTime::UNow(); }
inline const DateTime dt_from_time_t(time_t t) { return wxDateTime(t); }
inline const DateTime dt_make(int year, int month, int day,
        int hour = 0, int minute = 0, int second = 0, int millisec = 0)
{
    return wxDateTime(day, wxDateTime::Month(month - 1), year,
        hour, minute, second, millisec);
}
inline int dt_year(const DateTime &dt) { return dt.GetYear(); }
inline int dt_month(const DateTime &dt) { return (int)dt.GetMonth() + 1; }
inline int dt_day(const DateTime &dt) { return dt.GetDay(); }
inline int dt_hour(const DateTime &dt) { return dt.GetHour(); }
inline int dt_minute(const DateTime &dt) { return dt.GetMinute(); }
inline int dt_second(const DateTime &dt) { return dt.GetSecond(); }
inline int dt_millisec(const DateTime &dt) { return dt.GetMillisecond(); }
inline const DateTime dt_add_seconds(const DateTime &dt, int seconds)
{
    return dt + wxTimeSpan::Seconds(seconds);
}
inline const String to_string(const DateTime &dt, bool msec = false)
{
    return dt.Format(
            msec? _T("%Y-%m-%dT%H:%M:%S.%l"): _T("%Y-%m-%dT%H:%M:%S"));
}
inline const std::string to_stdstring(const DateTime &dt, bool msec = false)
{
    return NARROW(to_string(dt, msec));
}
inline DateTime &from_string(const String &s, DateTime &x)
{
    x = wxDateTime(0, 0, 0);
    const wxChar *pos = x.ParseFormat(s.GetData(),
            s.Len() == 10? _T("%Y-%m-%d"): (
            s.Len() > 19?
            (s[10] == _T('T')? _T("%Y-%m-%dT%H:%M:%S.%l"):
                _T("%Y-%m-%d %H:%M:%S.%l")):
            (s.Len() > 10 && s[10] == _T('T')? _T("%Y-%m-%dT%H:%M:%S"):
                _T("%Y-%m-%d %H:%M:%S"))));
    if (!pos || *pos)
        throw ValueBadCast(s, _T("DateTime (YYYY-MM-DD HH:MI:SS)"));
    return x;
}
inline DateTime &from_stdstring(const std::string &s, DateTime &x)
{
    return from_string(WIDEN(s), x);
}
#elif defined(YB_USE_QT)
typedef QDateTime DateTime;
inline const DateTime now() { return QDateTime::currentDateTime(); }
inline const DateTime dt_from_time_t(time_t t)
{
    QDateTime q;
    q.setTime_t(t);
    return q;
}
inline const DateTime dt_make(int year, int month, int day,
        int hour = 0, int minute = 0, int second = 0, int millisec = 0)
{
    return QDateTime(QDate(year, month, day),
            QTime(hour, minute, second, millisec));
}
inline int dt_year(const DateTime &dt) { return dt.date().year(); }
inline int dt_month(const DateTime &dt) { return dt.date().month(); }
inline int dt_day(const DateTime &dt) { return dt.date().day(); }
inline int dt_hour(const DateTime &dt) { return dt.time().hour(); }
inline int dt_minute(const DateTime &dt) { return dt.time().minute(); }
inline int dt_second(const DateTime &dt) { return dt.time().second(); }
inline int dt_millisec(const DateTime &dt) { return dt.time().msec(); }
inline const DateTime dt_add_seconds(const DateTime &dt, int seconds)
{
    return dt.addSecs(seconds);
}
inline const String to_string(const DateTime &dt, bool msec = false)
{
    return dt.toString(
            msec? "yyyy-MM-dd'T'hh:mm:ss.zzz": "yyyy-MM-dd'T'hh:mm:ss");
}
inline const std::string to_stdstring(const DateTime &dt, bool msec = false)
{
    return NARROW(to_string(dt, msec));
}
inline DateTime &from_string(const String &s, DateTime &x)
{
    x = QDateTime::fromString(s,
            s.length() == 10? _T("yyyy-MM-dd"): (
            s.length() > 19?
            (s[10] == 'T'? "yyyy-MM-dd'T'hh:mm:ss.zzz":
                "yyyy-MM-dd hh:mm:ss.zzz"):
            (s.length() > 10 && s[10] == 'T'? "yyyy-MM-dd'T'hh:mm:ss":
                "yyyy-MM-dd hh:mm:ss")));
    if (!x.isValid())
        throw ValueBadCast(s, _T("DateTime (YYYY-MM-DD HH:MI:SS)"));
    return x;
}
inline DateTime &from_stdstring(const std::string &s, DateTime &x)
{
    return from_string(WIDEN(s), x);
}
#else
typedef boost::posix_time::ptime DateTime;
inline const DateTime now()
{
    return boost::posix_time::second_clock::local_time();
}
inline const DateTime dt_from_time_t(time_t t)
{
    return boost::posix_time::from_time_t(t);
}
inline const DateTime dt_make(int year, int month, int day,
        int hour = 0, int minute = 0, int second = 0, int millisec = 0)
{
    return boost::posix_time::ptime(
            boost::gregorian::date(year, month, day),
            boost::posix_time::time_duration(hour, minute, second) +
            boost::posix_time::milliseconds(millisec));
}
inline int dt_year(const DateTime &dt) { return dt.date().year(); }
inline int dt_month(const DateTime &dt) { return dt.date().month(); }
inline int dt_day(const DateTime &dt) { return dt.date().day(); }
inline int dt_hour(const DateTime &dt) { return dt.time_of_day().hours(); }
inline int dt_minute(const DateTime &dt) { return dt.time_of_day().minutes(); }
inline int dt_second(const DateTime &dt) { return dt.time_of_day().seconds(); }
inline int dt_millisec(const DateTime &dt)
{
    return (int)(dt.time_of_day().total_milliseconds() -
        dt.time_of_day().total_seconds() * 1000);
}
inline const DateTime dt_add_seconds(const DateTime &dt, int seconds)
{
    return dt + boost::posix_time::seconds(seconds);
}
const std::string to_stdstring(const DateTime &dt, bool msec = false);
inline const String to_string(const DateTime &dt, bool msec = false)
{
    return WIDEN(to_stdstring(dt, msec));
}
DateTime &from_stdstring(const std::string &s, DateTime &x);
inline DateTime &from_string(const String &s, DateTime &x)
{
    return from_stdstring(NARROW(s), x);
}
#endif

typedef ::decimal Decimal;

inline const String to_string(const Decimal &d) { return d.str(); }
inline const std::string to_stdstring(const Decimal &d)
{
    return NARROW(d.str());
}
inline Decimal &from_string(const String &s, Decimal &x)
{
    return x = Decimal(s);
}
inline Decimal &from_stdstring(const std::string &s, Decimal &x)
{
    return from_string(WIDEN(s), x);
}

typedef std::vector<String> Strings;
typedef std::set<String> StringSet;

//! Dict class template resembles "dict" type found in Python
template <class K__, class V__>
class Dict
{
public:
    typedef std::map<K__, V__> BaseMap;
    typedef typename BaseMap::iterator iterator;
    typedef typename BaseMap::const_iterator const_iterator;
    typedef std::vector<K__> Keys;
    Dict() {}
    Dict(const Dict &other): d_(other.d_) {}
    explicit Dict(const BaseMap &other): d_(other) {}
    Dict &operator =(const Dict &other) {
        if (this != &other)
            d_ = other.d_;
        return *this;
    }
    Dict &operator =(const BaseMap &other) {
        d_ = other;
        return *this;
    }
    bool operator ==(const Dict &other) const { return d_ == other.d_; }
    bool operator !=(const Dict &other) const { return d_ != other.d_; }
    bool operator <(const Dict &other) const { return d_ < other.d_; }
    bool operator >=(const Dict &other) const { return d_ >= other.d_; }
    bool operator >(const Dict &other) const { return d_ > other.d_; }
    bool operator <=(const Dict &other) const { return d_ <= other.d_; }
    iterator begin() { return d_.begin(); }
    const_iterator begin() const { return d_.begin(); }
    iterator end() { return d_.end(); }
    const_iterator end() const { return d_.end(); }
    iterator find(const K__ &key) { return d_.find(key); }
    const_iterator find(const K__ &key) const { return d_.find(key); }
    size_t size() const { return d_.size(); }
    bool empty() const { return d_.size() == 0; }
    bool has(const K__ &key) const {
        return d_.find(key) != d_.end();
    }
    const V__ &get(const K__ &key) const {
        typename BaseMap::const_iterator it = d_.find(key);
        if (d_.end() == it)
            throw KeyError(to_string(key));
        return it->second;
    }
    const V__ get(const K__ &key, const V__ &def_val) const {
        typename BaseMap::const_iterator it = d_.find(key);
        if (d_.end() == it)
            return def_val;
        return it->second;
    }
    template <class U__>
    const U__ get_as(const K__ &key) const {
        const V__ &r = get(key);
        String s = to_string(r);
        try {
            U__ val = U__();
            from_string(s, val);
            return val;
        }
        catch (const std::exception &) {
            throw ValueError(s);
        }
    }
    template <class U__>
    const U__ get_as(const K__ &key, const U__ &def_val) const {
        try {
            const V__ &r = get(key);
            U__ val = U__();
            from_string(to_string(r), val);
            return val;
        }
        catch (const std::exception &) {}
        return def_val;
    }
    bool empty_key(const K__ &key) const {
        if (!has(key))
            return true;
        return str_empty(to_string(get(key)));
    }
    const V__ &operator[] (const K__ &key) const { return get(key); }
    V__ &operator[] (const K__ &key) { return d_[key]; }
    void set(const K__ &key, const V__ &value) {
        (*this)[key] = value;
    }
    template <class U__>
    void set(const K__ &key, const U__ &value) {
        V__ value2 = V__();
        from_string(to_string(value), value2);
        (*this)[key] = value2;
    }
    const V__ pop(const K__ &key) {
        typename BaseMap::iterator it = d_.find(key);
        if (d_.end() == it)
            throw KeyError(to_string(key));
        V__ r = it->second;
        d_.erase(it);
        return r;
    }
    const V__ pop(const K__ &key, const V__ &def_val) {
        typename BaseMap::iterator it = d_.find(key);
        if (d_.end() == it)
            return def_val;
        V__ r = it->second;
        d_.erase(it);
        return r;
    }
    const Keys keys() const {
        Keys k(d_.size());
        const_iterator i = d_.begin(), e = d_.end();
        for (size_t c = 0; i != e; ++i, ++c)
            k[c] = i->first;
        return k;
    }
    Dict &update(const Dict &other) {
        const_iterator i = other.begin(), e = other.end();
        for (; i != e; ++i)
            (*this)[i->first] = i->second;
        return *this;
    }
    void swap(Dict &other) {
        d_.swap(other.d_);
    }
protected:
    BaseMap d_;
};

//! OrderedDict class template resembles "OrderedDict" type found in Python
template <class K__, class V__>
class OrderedDict
{
public:
    typedef std::pair<const K__ *, V__> BasePair;
    typedef std::vector<BasePair> BaseVector;
    typedef typename BaseVector::iterator iterator;
    typedef typename BaseVector::const_iterator const_iterator;
    typedef std::map<K__, size_t> PositionMap;
    typedef std::vector<K__> Keys;
    typedef std::map<K__, V__> SourceMap;
    typedef Dict<K__, V__> SourceDict;

    // tree basic operations
    std::pair<iterator, bool> insert(const K__ &k, const V__ &v)
    {
        std::pair<typename PositionMap::iterator, bool> r =
            p_.insert(std::make_pair(k, 0));
        size_t pos;
        if (r.second) {     // new item inserted
            pos = a_.size();
            a_.push_back(std::make_pair(&r.first->first, v));
            r.first->second = pos;
        }
        else {              // item exists
            pos = r.first->second;
        }
        return std::make_pair(a_.begin() + pos, r.second);
    }
    bool pop_basic(const K__ &k, V__ *v)
    {
        typename PositionMap::iterator i = p_.find(k);
        if (p_.end() == i)
            return false;
        size_t pos = i->second;
        typename PositionMap::iterator j = p_.begin(), jend = p_.end();
        for (; j != jend; ++j)
            if (j->second > pos)
                --(j->second);
        p_.erase(i);
        using namespace std;
        if (v)
            swap(*v, a_[pos].second);
        a_.erase(a_.begin() + pos);
        return true;
    }
    size_t get_basic(const K__ &k, V__ *v) const
    {
        typename PositionMap::const_iterator i = p_.find(k);
        if (p_.end() == i)
            return a_.size();
        size_t pos = i->second;
        if (v)
            *v = a_[pos].second;
        return pos;
    }

    // begin, end, find
    iterator begin() { return a_.begin(); }
    const_iterator begin() const { return a_.begin(); }
    iterator end() { return a_.end(); }
    const_iterator end() const { return a_.end(); }
    iterator find(const K__ &key) {
        size_t pos = get_basic(key, NULL);
        if (pos == a_.size())
            return a_.end();
        return a_.begin() + pos;
    }
    const_iterator find(const K__ &key) const {
        size_t pos = get_basic(key, NULL);
        if (pos == a_.size())
            return a_.end();
        return a_.begin() + pos;
    }

    // ctors, assignment
    OrderedDict() {}
    OrderedDict(const OrderedDict &other)
        : a_(other.a_), p_(other.p_)
    {
        fix_pointers();
    }
    explicit OrderedDict(const SourceMap &other_map) {
        typename SourceMap::const_iterator i = other_map.begin(),
                 iend = other_map.end();
        a_.reserve(other_map.size());
        for (; i != iend; ++i)
            set(i->first, i->second);
    }
    explicit OrderedDict(const SourceDict &other_dict) {
        typename SourceDict::const_iterator i = other_dict.begin(),
                 iend = other_dict.end();
        a_.reserve(other_dict.size());
        for (; i != iend; ++i)
            set(i->first, i->second);
    }
    OrderedDict &operator =(const OrderedDict &other) {
        if (this != &other) {
            a_ = other.a_;
            p_ = other.p_;
            fix_pointers();
        }
        return *this;
    }

    // comparison
    bool eq(const OrderedDict &other) const {
        if (a_.size() != other.a_.size())
            return false;
        typename BaseVector::const_iterator i = a_.begin(), iend = a_.end(),
                 j = other.a_.begin(), jend = other.a_.end();
        for (; i != iend; ++i, ++j)
            if (*i->first != *j->first || i->second != j->second)
                return false;
        return true;
    }
    bool lt(const OrderedDict &other) const {
        if (p_ < other.p_)
            return true;
        if (p_ > other.p_)
            return false;
        typename BaseVector::const_iterator i = a_.begin(), iend = a_.end(),
                 j = other.a_.begin(), jend = other.a_.end();
        for (; i != iend; ++i, ++j) {
            if (i->second < j->second)
                return true;
            if (i->second > j->second)
                return false;
        }
        return false;
    }
    bool operator ==(const OrderedDict &other) const { return eq(other); }
    bool operator != (const OrderedDict &other) const { return !eq(other); }
    bool operator <(const OrderedDict &other) const { return lt(other); }
    bool operator >=(const OrderedDict &other) const { return !lt(other); }
    bool operator >(const OrderedDict &other) const { return other.lt(*this); }
    bool operator <=(const OrderedDict &other) const { return !other.lt(*this); }

    // indexing
    const BasePair &item(const K__ &key) const {
        size_t pos = get_basic(key, NULL);
        if (pos == a_.size())
            throw KeyError(to_string(key));
        return a_[pos];
    }
    BasePair &item(const K__ &key) {
        V__ v = V__();
        std::pair<iterator, bool> r = insert(key, v);
        return *r.first;
    }
    const BasePair &item(size_t i) const {
        check_bounds(i);
        return a_[i];
    }
    BasePair &item(size_t i) {
        check_bounds(i);
        return a_[i];
    }
    const V__ &operator [] (const K__ &key) const { return item(key).second; }
    V__ &operator [] (const K__ &key) { return item(key).second; }
    const V__ &operator [] (size_t i) const { return item(i).second; }
    V__ &operator [] (size_t i) { return item(i).second; }

    // misc accessors
    size_t size() const { return a_.size(); }
    bool empty() const { return a_.size() == 0; }
    bool has(const K__ &key) const {
        return p_.find(key) != p_.end();
    }
    const V__ &get(const K__ &key) const { return item(key).second; }
    const V__ &get(size_t i) const { return item(i).second; }
    const V__ get(const K__ &key, const V__ &def_val) {
        V__ v = def_val;
        get_basic(key, &v);
        return v;
    }
    template <class U__>
    const U__ get_as(const K__ &key) const {
        const V__ &r = get(key);
        String s = to_string(r);
        try {
            U__ val = U__();
            from_string(s, val);
            return val;
        }
        catch (const std::exception &) {
            throw ValueError(s);
        }
    }
    template <class U__>
    const U__ get_as(const K__ &key, const U__ &def_val) const {
        try {
            const V__ &r = get(key);
            U__ val = U__();
            from_string(to_string(r), val);
            return val;
        }
        catch (const std::exception &) {}
        return def_val;
    }
    bool empty_key(const K__ &key) const {
        size_t pos = get_basic(key, NULL);
        if (pos == a_.size())
            return true;
        return str_empty(to_string(get(pos)));
    }
    void set(const K__ &key, const V__ &value) {
        item(key).second = value;
    }
    template <class U__>
    void set(const K__ &key, const U__ &value) {
        V__ value2 = V__();
        from_string(to_string(value), value2);
        item(key).second = value2;
    }
    const V__ pop(const K__ &key) {
        V__ r = V__();
        if (!pop_basic(key, &r))
            throw KeyError(to_string(key));
        return r;
    }
    const V__ pop(const K__ &key, const V__ &def_val) {
        V__ r = def_val;
        pop_basic(key, &r);
        return r;
    }
    const V__ pop(size_t i) {
        check_bounds(i);
        V__ r = V__();
        pop_basic(*a_[i].first, &r);
        return r;
    }
    void get_keys(Keys &keys) const {
        Keys new_keys(a_.size());
        typename BaseVector::const_iterator i = a_.begin(), iend = a_.end();
        typename Keys::iterator j = new_keys.begin(), jend = new_keys.end();
        for (; i != iend; ++i, ++j)
            *j = *i->first;
        new_keys.swap(keys);
    }
    const Keys keys() const {
        Keys k;
        get_keys(k);
        return k;
    }
    OrderedDict &update(const OrderedDict &other) {
        const_iterator i = other.begin(), iend = other.end();
        for (; i != iend; ++i)
            (*this)[*i->first] = i->second;
        return *this;
    }
    void do_swap(OrderedDict &other) {
        a_.swap(other.a_);
        p_.swap(other.p_);
    }
private:
    // misc helpers
    void check_bounds(size_t i) const {
        if (i >= a_.size())
            throw KeyError(_T("index out of bounds"));
    }
    void fix_pointers() {
        typename BaseVector::iterator i = a_.begin(), iend = a_.end();
        for (; i != iend; ++i)
            i->first = &(p_.find(*i->first)->first);
    }

    // data members
    BaseVector a_;
    PositionMap p_;
};

typedef Dict<String, String> StringDict;
typedef OrderedDict<String, String> OrderedStringDict;

} // namespace Yb

namespace std {

template <class K__, class V__>
void swap(::Yb::Dict<K__, V__> &a, ::Yb::Dict<K__, V__> &b) {
    a.swap(b);
}

template <class K__, class V__>
void swap(::Yb::OrderedDict<K__, V__> &a, ::Yb::OrderedDict<K__, V__> &b) {
    a.do_swap(b);
}

} // end of namespace std

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__UTIL__DATA_TYPES__INCLUDED
