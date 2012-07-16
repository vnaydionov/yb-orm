// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__UTIL__DATA_TYPES__INCLUDED
#define YB__UTIL__DATA_TYPES__INCLUDED

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
typedef std::map<String, String> StringMap;

// a Python-like dict
class Dict
{
    StringMap dict_;
public:
    bool has(const String &key) const;
    const String &get(const String &key) const;
    const String &operator[] (const String &key) const { return get(key); }
    String &operator[] (const String &key) { return dict_[key]; }
    const String get(const String &key, const String &def_val) const;
    template <class T>
    const T get_as(const String &key) const
    {
        String &r = get(key);
        try {
            T val;
            from_string(r, val);
            return val;
        }
        catch (const std::exception &) {
            throw ValueError(r);
        }
    }
    template <class T>
    const T get_as(const String &key, const T &def_val) const
    {
        String r = get(key, _T(""));
        try {
            T val;
            from_string(r, val);
            return val;
        }
        catch (const std::exception &) {}
        return def_val;
    }
    bool empty(const String &key) const
    {
        return !has(key) || str_empty(get(key));
    }
    const String pop(const String &key, const String &def_val = _T(""));
    template <class T>
    void set(const String &key, const T &value)
    {
        dict_[key] = to_string(value);
    }
    size_t size() const { return dict_.size(); }
    const Strings keys() const;
    Dict() {}
    explicit Dict(const StringMap &dict): dict_(dict) {}
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__UTIL__DATA_TYPES__INCLUDED
