// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#include <util/DataTypes.h>
#if !defined(YB_USE_QT) && !defined(YB_USE_WX)
#include <iomanip>
#endif

namespace Yb {

#if !defined(YB_USE_QT) && !defined(YB_USE_WX)
const std::string to_stdstring(const DateTime &dt, bool msec)
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

DateTime &from_stdstring(const std::string &s, DateTime &x)
{
    if (s.size() < 19 || s[4] != '-' || s[7] != '-' ||
        (s[10] != ' ' && s[10] != 'T') || s[13] != ':' ||
        s[16] != ':' || (s.size() > 19 && s[19] != '.'))
    {
        throw ValueBadCast(WIDEN(s), _T("DateTime (YYYY-MM-DD HH:MI:SS)"));
    }
    const char *cs = s.c_str();
    int year = extract_int(cs, 4), month = extract_int(cs + 5, 2),
        day = extract_int(cs + 8, 2), hours = extract_int(cs + 11, 2),
        minutes = extract_int(cs + 14, 2), seconds = extract_int(cs + 17, 2),
        millis = 0;
    if (s.size() > 19)
        millis = extract_int(cs + 19, 3);
    if (year < 0 || month < 0 || day < 0
        || hours < 0 || minutes < 0 || seconds < 0 || millis < 0)
    {
        throw ValueBadCast(WIDEN(s), _T("DateTime (YYYY-MM-DD HH:MI:SS)"));
    }
    x = dt_make(year, month, day, hours, minutes, seconds, millis);
    return x;
}
#endif

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
