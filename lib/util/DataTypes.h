// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__UTIL__DATA_TYPES__INCLUDED
#define YB__UTIL__DATA_TYPES__INCLUDED

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
    bool eq(const Dict &other) const { return d_ == other.d_; }
    bool lt(const Dict &other) const { return d_ < other.d_; }
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

template <class K__, class V__>
bool operator ==(const Dict<K__, V__> &a, const Dict<K__, V__> &b) {
    return a.eq(b);
}
template <class K__, class V__>
bool operator !=(const Dict<K__, V__> &a, const Dict<K__, V__> &b) {
    return !a.eq(b);
}
template <class K__, class V__>
bool operator <(const Dict<K__, V__> &a, const Dict<K__, V__> &b) {
    return a.lt(b);
}
template <class K__, class V__>
bool operator >=(const Dict<K__, V__> &a, const Dict<K__, V__> &b) {
    return !a.lt(b);
}
template <class K__, class V__>
bool operator >(const Dict<K__, V__> &a, const Dict<K__, V__> &b) {
    return b.lt(a);
}
template <class K__, class V__>
bool operator <=(const Dict<K__, V__> &a, const Dict<K__, V__> &b) {
    return !b.lt(a);
}

//! OrderedDict class template resembles "OrderedDict" type found in Python
template <class K__, class V__>
class OrderedDict: public Dict<K__, V__>
{
public:
    typedef Dict<K__, V__> BaseDict;
    typedef std::map<K__, V__> BaseMap;
    typedef typename BaseMap::iterator iterator;
    typedef typename BaseMap::const_iterator const_iterator;
    typedef std::vector<K__> Keys;
    OrderedDict() {}
    OrderedDict(const OrderedDict &other)
        : BaseDict(other.d_), k_(other.k_) {}
    explicit OrderedDict(const BaseMap &other)
        : BaseDict(other)
    {
        update_keys();
    }
    OrderedDict &operator =(const OrderedDict &other) {
        if (this != &other) {
            this->d_ = other.d_;
            k_ = other.k_;
        }
        return *this;
    }
    OrderedDict &operator =(const BaseMap &other) {
        this->d_ = other;
        update_keys();
        return *this;
    }
    bool eq(const OrderedDict &other) const {
        return this->d_ == other.d_ && k_ == other.k_;
    }
    bool lt(const OrderedDict &other) const {
        if (k_ == other.k_) {
            typename Keys::const_iterator i = k_.begin(), e = k_.end();
            for (; i != e; ++i) {
                if (BaseDict::get(*i) < 
                        reinterpret_cast<const BaseDict & >(other)
                        .get(*i))
                    return true;
                if (BaseDict::get(*i) >
                        reinterpret_cast<const BaseDict & >(other)
                        .get(*i))
                    break;
            }
            return false;
        }
        return k_ < other.k_;
    }
    const V__ &operator[] (const K__ &key) const { return BaseDict::get(key); }
    const V__ &get(size_t i) const {
        if (i >= this->d_.size())
            throw KeyError(_T("index out of bounds"));
        return BaseDict::get(k_[i]);
    }
    const V__ &operator[] (size_t i) const { return get(i); }
    V__ &operator[] (const K__ &key) {
        V__ val = V__();
        std::pair<typename BaseMap::iterator, bool> r
            = this->d_.insert(std::make_pair(key, val));
        if (r.second)
            k_.push_back(key);
        return r.first->second;
    }
    void set(const K__ &key, const V__ &value) {
        (*this)[key] = value;
    }
    template <class U__>
    void set(const K__ &key, const U__ &value) {
        V__ value2 = V__();
        from_string(to_string(value), value2);
        (*this)[key] = value2;
    }
    V__ &operator[] (size_t i) {
        if (i >= this->size())
            throw KeyError(_T("index out of bounds"));
        return (*this)[k_[i]];
    }
    const V__ pop(const K__ &key) {
        typename BaseMap::iterator it = this->d_.find(key);
        if (this->d_.end() == it)
            throw KeyError(key);
        V__ r = it->second;
        this->d_.erase(it);
        k_.erase(std::find(k_.begin(), k_.end(), key));
        return r;
    }
    const V__ pop(const K__ &key, const V__ &def_val) {
        typename BaseMap::iterator it = this->d_.find(key);
        if (this->d_.end() == it)
            return def_val;
        V__ r = it->second;
        this->d_.erase(it);
        k_.erase(std::find(k_.begin(), k_.end(), key));
        return r;
    }
    const V__ pop(size_t i) {
        if (i >= this->size())
            throw KeyError(_T("index out of bounds"));
        typename BaseMap::iterator it = this->d_.find(k_[i]);
        if (this->d_.end() == it)
            throw KeyError(k_[i]);
        V__ r = it->second;
        this->d_.erase(it);
        k_.erase(k_.begin() + i);
        return r;
    }
    const Keys &keys() const { return k_; }
    void swap(OrderedDict &other) {
        this->d_.swap(other.d_);
        k_.swap(other.k_);
    }
protected:
    void update_keys() {
        Keys k(this->d_.size());
        typename BaseMap::const_iterator i = this->d_.begin(), e = this->d_.end();
        for (size_t c = 0; i != e; ++i, ++c)
            k[c] = i->first;
        k_.swap(k);
    }
    Keys k_;
};

template <class K__, class V__>
bool operator ==(const OrderedDict<K__, V__> &a, const OrderedDict<K__, V__> &b) {
    return a.eq(b);
}
template <class K__, class V__>
bool operator !=(const OrderedDict<K__, V__> &a, const OrderedDict<K__, V__> &b) {
    return !a.eq(b);
}
template <class K__, class V__>
bool operator <(const OrderedDict<K__, V__> &a, const OrderedDict<K__, V__> &b) {
    return a.lt(b);
}
template <class K__, class V__>
bool operator >=(const OrderedDict<K__, V__> &a, const OrderedDict<K__, V__> &b) {
    return !a.lt(b);
}
template <class K__, class V__>
bool operator >(const OrderedDict<K__, V__> &a, const OrderedDict<K__, V__> &b) {
    return b.lt(a);
}
template <class K__, class V__>
bool operator <=(const OrderedDict<K__, V__> &a, const OrderedDict<K__, V__> &b) {
    return !b.lt(a);
}

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
    a.swap(b);
}

} // end of namespace std

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__UTIL__DATA_TYPES__INCLUDED
