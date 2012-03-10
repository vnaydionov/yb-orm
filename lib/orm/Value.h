// -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#ifndef YB__ORM__VALUE__INCLUDED
#define YB__ORM__VALUE__INCLUDED

// Value class should not be implemented upon boost::any because of massive
// copying of Value objects here and there.

#include <string>
#include <vector>
#include <set>
#include <map>
#include <stdexcept>
#include <utility>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <util/decimal.h>
#include <util/UnicodeSupport.h>

namespace Yb {

#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef __int64 LongInt;
#else
typedef long long LongInt;
#endif

typedef ::decimal Decimal;
typedef ::boost::posix_time::ptime DateTime;

inline const DateTime now()
{
    return boost::posix_time::second_clock::local_time();
}

inline const DateTime mk_datetime(time_t t)
{
    return boost::posix_time::from_time_t(t);
}

const DateTime mk_datetime(const String &s);

inline const DateTime mk_datetime(int year, int month, int day,
        int hour = 0, int minute = 0, int second = 0)
{
    return DateTime(boost::gregorian::date(year, month, day),
            boost::posix_time::time_duration(hour, minute, second));
}

const String ptime2str(const boost::posix_time::ptime &t);

template <typename T>
inline const String to_string(const T &x)
{
    return boost::lexical_cast<String>(x);
}

inline const String to_string(const Decimal &x)
{
    return x.str();
}

inline const String to_string(const DateTime &x)
{
    return ptime2str(x);
}

class ValueError : public std::logic_error
{
public:
    ValueError(const String &msg) :
        std::logic_error(NARROW(msg))
    {}
};

class ValueIsNull : public ValueError
{
public:
    ValueIsNull()
        :ValueError(_T("Trying to get value of null"))
    {}
};

class ValueBadCast : public ValueError
{
public:
    ValueBadCast(const String &value, const String &type)
        :ValueError(_T("Can't cast value '") + value + _T("' to type ") + type)
    {}
};

struct ValueHolderBase;

class Value
{
public:
    enum Type { INVALID, INTEGER, LONGINT, STRING, DECIMAL, DATETIME };
    Value();
    Value(int x);
    Value(LongInt x);
    Value(const Char *x);
    Value(const String &x);
    Value(const Decimal &x);
    Value(const DateTime &x);
    bool is_null() const { return type_ == INVALID; }
    int as_integer() const;
    LongInt as_longint() const;
    const String as_string() const;
    const Decimal as_decimal() const;
    const DateTime as_date_time() const;
    const String sql_str() const;
    const Value nvl(const Value &def_value) const { return is_null()? def_value: *this; }
    int cmp(const Value &x) const;
    int get_type() const { return type_; }
    const Value get_typed_value(int type) const;
    static const Char *get_type_name(int type);
private:
    int type_;
    boost::shared_ptr<ValueHolderBase> data_;
};

inline bool operator==(const Value &x, const Value &y) { return !x.cmp(y); }
inline bool operator!=(const Value &x, const Value &y) { return x.cmp(y) != 0; }
inline bool operator<(const Value &x, const Value &y) { return x.cmp(y) < 0; }
inline bool operator>=(const Value &x, const Value &y) { return x.cmp(y) >= 0; }
inline bool operator>(const Value &x, const Value &y) { return x.cmp(y) > 0; }
inline bool operator<=(const Value &x, const Value &y) { return x.cmp(y) <= 0; }

typedef std::vector<String> Strings;
typedef std::set<String> StringSet;
typedef std::vector<Value> Values;
typedef std::map<String, Value> ValueMap;
typedef std::pair<String, ValueMap> Key;
typedef std::vector<Key> Keys;

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__VALUE__INCLUDED
