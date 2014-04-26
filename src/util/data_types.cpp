// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#define YBUTIL_SOURCE

#include "util/data_types.h"
#include <stdlib.h>
#if !defined(YB_USE_QT) && !defined(YB_USE_WX)
#include <iomanip>
#endif

namespace Yb {

YBUTIL_DECL double &from_stdstring(const std::string &s, double &x)
{
    char *end = NULL;
    x = strtod(s.c_str(), &end);
    if (!end || *end)
        throw ValueBadCast(WIDEN(s), _T("value error: extra characters left"));
    return x;
}

#if defined(YB_USE_WX)

YBUTIL_DECL const DateTime dt_make(int year, int month, int day,
        int hour, int minute, int second, int millisec)
{
    return wxDateTime(day, wxDateTime::Month(month - 1), year,
        hour, minute, second, millisec);
}

YBUTIL_DECL const DateTime dt_add_seconds(const DateTime &dt, int seconds)
{
    return dt + wxTimeSpan::Seconds(seconds);
}

YBUTIL_DECL const String to_string(const DateTime &dt, bool msec)
{
    return dt.Format(
            msec? _T("%Y-%m-%dT%H:%M:%S.%l"): _T("%Y-%m-%dT%H:%M:%S"));
}

YBUTIL_DECL const std::string to_stdstring(const DateTime &dt, bool msec)
{
    return NARROW(to_string(dt, msec));
}

YBUTIL_DECL DateTime &from_string(const String &s, DateTime &x)
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

YBUTIL_DECL DateTime &from_stdstring(const std::string &s, DateTime &x)
{
    return from_string(WIDEN(s), x);
}

#elif defined(YB_USE_QT)

YBUTIL_DECL const DateTime dt_from_time_t(time_t t)
{
    QDateTime q;
    q.setTime_t(t);
    return q;
}

YBUTIL_DECL const DateTime dt_make(int year, int month, int day,
        int hour, int minute, int second, int millisec)
{
    return QDateTime(QDate(year, month, day),
            QTime(hour, minute, second, millisec));
}

YBUTIL_DECL const DateTime dt_add_seconds(const DateTime &dt, int seconds)
{
    return dt.addSecs(seconds);
}

YBUTIL_DECL const String to_string(const DateTime &dt, bool msec)
{
    return dt.toString(
            msec? "yyyy-MM-dd'T'hh:mm:ss.zzz": "yyyy-MM-dd'T'hh:mm:ss");
}

YBUTIL_DECL const std::string to_stdstring(const DateTime &dt, bool msec)
{
    return NARROW(to_string(dt, msec));
}

YBUTIL_DECL DateTime &from_string(const String &s, DateTime &x)
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

YBUTIL_DECL DateTime &from_stdstring(const std::string &s, DateTime &x)
{
    return from_string(WIDEN(s), x);
}

#else

YBUTIL_DECL const DateTime dt_make(int year, int month, int day,
        int hour, int minute, int second, int millisec)
{
    return boost::posix_time::ptime(
            boost::gregorian::date(year, month, day),
            boost::posix_time::time_duration(hour, minute, second) +
            boost::posix_time::milliseconds(millisec));
}

YBUTIL_DECL int dt_millisec(const DateTime &dt)
{
    return (int)(dt.time_of_day().total_milliseconds() -
        dt.time_of_day().total_seconds() * 1000);
}

YBUTIL_DECL const DateTime dt_add_seconds(const DateTime &dt, int seconds)
{
    return dt + boost::posix_time::seconds(seconds);
}

YBUTIL_DECL const std::string to_stdstring(const DateTime &dt, bool msec)
{
    std::ostringstream out;
    out << std::setfill('0')
        << std::setw(4) << dt_year(dt)
        << '-' << std::setw(2) << dt_month(dt)
        << '-' << std::setw(2) << dt_day(dt)
        << 'T' << std::setw(2) << dt_hour(dt)
        << ':' << std::setw(2) << dt_minute(dt)
        << ':' << std::setw(2) << dt_second(dt);
    if (msec)
        out << '.' << std::setw(3) << dt_millisec(dt);
    return out.str();
}

static int extract_int(const char *s, int len)
{
    int a = 0;
    for (; len > 0 && *s >= '0' && *s <= '9';
         a = a*10 + *s - '0', --len, ++s);
    return len == 0? a: -1;
}

YBUTIL_DECL DateTime &from_stdstring(const std::string &s, DateTime &x)
{
#define DT_FORMAT_REX _T("DateTime YYYY-mm-dd([T ]HH:MM:SS(.XXX)?)?")
    const char *cs = s.c_str();
    if (s.size() < 10 || s[4] != '-' || s[7] != '-')
        throw ValueBadCast(WIDEN(s), DT_FORMAT_REX);
    int year = extract_int(cs, 4), month = extract_int(cs + 5, 2),
        day = extract_int(cs + 8, 2),
        hours = 0, minutes = 0, seconds = 0, millis = 0;
    if (s.size() > 10) {
        if (s.size() < 19 || (s[10] != ' ' && s[10] != 'T')
                || s[13] != ':' || s[16] != ':')
        {
            throw ValueBadCast(WIDEN(s), DT_FORMAT_REX);
        }
        hours = extract_int(cs + 11, 2);
        minutes = extract_int(cs + 14, 2);
        seconds = extract_int(cs + 17, 2);
        if (s.size() > 19) {
            if (s.size() < 23 || s[19] != '.')
                throw ValueBadCast(WIDEN(s), DT_FORMAT_REX);
            millis = extract_int(cs + 19, 3);
        }
    }
    if (year < 0 || month < 0 || day < 0
        || hours < 0 || minutes < 0 || seconds < 0 || millis < 0)
    {
        throw ValueBadCast(WIDEN(s), DT_FORMAT_REX);
    }
    x = dt_make(year, month, day, hours, minutes, seconds, millis);
    return x;
}

#endif // !defined(YB_USE_QT) && !defined(YB_USE_WX)

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
