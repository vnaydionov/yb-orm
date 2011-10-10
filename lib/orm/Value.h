// -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#ifndef YB__ORM__VALUE__INCLUDED
#define YB__ORM__VALUE__INCLUDED

#include <string>
#include <vector>
#include <set>
#include <map>
#include <stdexcept>
#include <utility>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <util/decimal.h>

namespace Yb {

#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef __int64 LongInt;
#else
typedef long long LongInt;
#endif

struct tm *localtime_safe(const time_t *clock, struct tm *result);

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

const DateTime mk_datetime(const std::string &s);

inline const DateTime mk_datetime(int year, int month, int day,
        int hour = 0, int minute = 0, int second = 0)
{
    return DateTime(boost::gregorian::date(year, month, day),
            boost::posix_time::time_duration(hour, minute, second));
}

const std::string ptime2str(const boost::posix_time::ptime &t);

template <typename T>
inline const std::string to_string(const T &x)
{
    return boost::lexical_cast<std::string>(x);
}

inline const std::string to_string(const Decimal &x)
{
    return x.str();
}

inline const std::string to_string(const DateTime &x)
{
    return ptime2str(x);
}

class ValueError : public std::logic_error
{
public:
    ValueError(const std::string &msg) :
        std::logic_error(msg)
    {}
};

class ValueIsNull : public ValueError
{
public:
    ValueIsNull()
        :ValueError("Trying to get value of null")
    {}
};

class ValueBadCast : public ValueError
{
public:
    ValueBadCast(const std::string &value, const std::string &type)
        :ValueError("Can't cast value '" + value + "' to type " + type)
    {}
};

class ValueData;

class Value
{
public:
    enum Type { INVALID, LONGINT, STRING, DECIMAL, DATETIME };
    Value();
    Value(LongInt x);
    Value(const char *x);
    Value(const std::string &x);
    Value(const Decimal &x);
    Value(const DateTime &x);
    bool is_null() const;
    LongInt as_longint() const;
    const std::string as_string() const;
    const Decimal as_decimal() const;
    const DateTime as_date_time() const;
    const std::string sql_str() const;
    const Value nvl(const Value &def_value) const { return is_null()? def_value: *this; }
    int cmp(const Value &x) const;
    int get_type() const { return type_; }
    const Value get_typed_value(int type) const;
    static const char *get_type_name(int type);
private:
    const ValueData &check_null() const;
private:
    boost::shared_ptr<ValueData> data_;
    Type type_;
};

inline bool operator==(const Value &x, const Value &y) { return !x.cmp(y); }
inline bool operator!=(const Value &x, const Value &y) { return x.cmp(y) != 0; }
inline bool operator<(const Value &x, const Value &y) { return x.cmp(y) < 0; }
inline bool operator>=(const Value &x, const Value &y) { return x.cmp(y) >= 0; }
inline bool operator>(const Value &x, const Value &y) { return x.cmp(y) > 0; }
inline bool operator<=(const Value &x, const Value &y) { return x.cmp(y) <= 0; }

typedef std::vector<std::string> Strings;
typedef std::set<std::string> StringSet;
typedef std::vector<Value> Values;
typedef std::map<std::string, Value> ValueMap;
typedef std::pair<std::string, ValueMap> Key;
typedef std::vector<Key> Keys;

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__VALUE__INCLUDED
