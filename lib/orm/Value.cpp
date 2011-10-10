#include <time.h>
#include <stdio.h>
#include <sstream>
#include <iomanip>
#include <boost/lexical_cast.hpp>
#include <boost/thread/mutex.hpp>
#include <util/str_utils.hpp>
#include "Value.h"
#include "MetaData.h"

using namespace std;
using namespace Yb::StrUtils;

namespace Yb {

struct tm *localtime_safe(const time_t *clock, struct tm *result)
{
    if (!clock || !result)
        return NULL;
#if defined(_MSC_VER)
    errno_t err = localtime_s(result, clock);
    if (err)
        return NULL;
#elif defined(__unix__)
    if (!localtime_r(clock, result))
        return NULL;
#else
    static boost::mutex m;
    boost::mutex::scoped_lock lock(m);
    struct tm *r = localtime(clock);
    if (!r)
        return NULL;
    memcpy(result, r, sizeof(*result));
#endif
    return result;
}

int extract_int(const char *s, size_t len)
{
    int a = 0;
    for (; len > 0 && *s >= '0' && *s <= '9';
            a = a*10 + *s - '0', --len, ++s);
    return len == 0? a: -1;
}

const DateTime mk_datetime(const string &s)
{
#if 0
    string::size_type pos = s.find('T');
    if (pos == string::npos)
        return boost::posix_time::time_from_string(s);
    string t(s);
    t[pos] = ' ';
    return boost::posix_time::time_from_string(t);
#else
    if (s.size() < 19 || s[4] != '-' || s[7] != '-' ||
        (s[10] != ' ' && s[10] != 'T') || s[13] != ':' ||
        s[16] != ':')
    {
        throw ValueBadCast(s, "YYYY-MM-DD HH:MI:SS");
    }
    const char *cs = s.c_str();
    int year = extract_int(cs, 4), month = extract_int(cs + 5, 2),
        day = extract_int(cs + 8, 2), hours = extract_int(cs + 11, 2),
        minutes = extract_int(cs + 14, 2), seconds = extract_int(cs + 17, 2);
    if (year < 0 || month < 0 || day < 0
        || hours < 0 || minutes < 0 || seconds < 0)
    {
        throw ValueBadCast(s, "YYYY-MM-DD HH:MI:SS");
    }
    return mk_datetime(year, month, day, hours, minutes, seconds);
#endif
}

const string ptime2str(const boost::posix_time::ptime &t)
{
#if 0
    stringstream o;
    o << setfill('0')
        << setw(4) << t.date().year()
        << '-' << setw(2) << (int)t.date().month()
        << '-' << setw(2) << t.date().day()
        << 'T' << setw(2) << t.time_of_day().hours()
        << ':' << setw(2) << t.time_of_day().minutes()
        << ':' << setw(2) << t.time_of_day().seconds();
    return o.str();
#else
    char buf[80];
    sprintf(buf, "%04d-%02d-%02dT%02d:%02d:%02d",
        (int)t.date().year(), (int)t.date().month(),
        (int)t.date().day(), (int)t.time_of_day().hours(),
        (int)t.time_of_day().minutes(), (int)t.time_of_day().seconds());
    return string(buf);
#endif
}

class ValueData
{
public:
    virtual const string to_str() const = 0;
    virtual const string to_sql_str() const = 0;
    virtual int cmp(const ValueData &v) const = 0;
    virtual ~ValueData() {}
};

template <typename T>
class ValueDataImpl: public ValueData
{
    T x_;
public:
    ValueDataImpl(T x = T()): x_(x) {}
    T get() const { return x_; }
    const string to_str() const { return to_string(x_); }
    const string to_sql_str() const { return to_str(); }
    int cmp(const ValueData &v) const {
        const T &y = dynamic_cast<const ValueDataImpl<T> &>(v).x_;
        return x_ == y? 0: (x_ < y? -1: 1);
    }
};

template <>
const string ValueDataImpl<string>::to_sql_str() const
{
    return quote(sql_string_escape(x_));
}

template <>
const string ValueDataImpl<DateTime>::to_sql_str() const
{
    string t(to_string(x_));
    size_t pos = t.find('T');
    if (pos != string::npos)
        t[pos] = ' ';
    return "'" + t + "'";
}

Value::Value()
    : type_(INVALID)
{}

Value::Value(LongInt x)
    : data_(new ValueDataImpl<LongInt>(x))
    , type_(LONGINT)
{}

Value::Value(const char *x)
    : data_(new ValueDataImpl<string>(x))
    , type_(STRING)
{}

Value::Value(const string &x)
    : data_(new ValueDataImpl<string>(x))
    , type_(STRING)
{}

Value::Value(const Decimal &x)
    : data_(new ValueDataImpl<Decimal>(x))
    , type_(DECIMAL)
{}

Value::Value(const DateTime &x)
    : data_(new ValueDataImpl<DateTime>(x))
    , type_(DATETIME)
{}

bool
Value::is_null() const
{
    return type_ == INVALID;
}

LongInt
Value::as_longint() const
{
    if (type_ != LONGINT) {
        try {
            return boost::lexical_cast<LongInt>(check_null().to_str());
        }
        catch (const boost::bad_lexical_cast &) {
            throw ValueBadCast(check_null().to_str(), "LongLong");
        }
    }
    const ValueDataImpl<LongInt> &d =
        dynamic_cast<const ValueDataImpl<LongInt> &>(check_null());
    return d.get();
}

const Decimal
Value::as_decimal() const
{
    if (type_ != DECIMAL) {
        string s = check_null().to_str();
        try {
            return Decimal(s);
        }
        catch (const Decimal::exception &) {
            double f = 0.0;
            try {
                f = boost::lexical_cast<double>(s);
            }
            catch (const boost::bad_lexical_cast &) {
                throw ValueBadCast(s, "Decimal");
            }
            ostringstream o;
            o << setprecision(10) << fixed << f;
            try {
                return Decimal(o.str());
            }
            catch (const Decimal::exception &) {
                throw ValueBadCast(s, "Decimal");
            }
        }
    }
    const ValueDataImpl<Decimal> &d =
        dynamic_cast<const ValueDataImpl<Decimal> &>(check_null());
    return d.get();
}

const DateTime
Value::as_date_time() const
{
    if (type_ != DATETIME) {
        string s = check_null().to_str();
        try {
            time_t t = boost::lexical_cast<time_t>(s);
            tm stm;
            if (!localtime_safe(&t, &stm))
                throw ValueBadCast(s, "struct tm");
            return DateTime(
                    boost::gregorian::date(
                        stm.tm_year + 1900, stm.tm_mon + 1, stm.tm_mday),
                    boost::posix_time::time_duration(
                        stm.tm_hour, stm.tm_min, stm.tm_sec));
        }
        catch (const boost::bad_lexical_cast &) {
            return mk_datetime(s);
        }
    }
    const ValueDataImpl<DateTime> &d =
        dynamic_cast<const ValueDataImpl<DateTime> &>(check_null());
    return d.get();
}

const string
Value::as_string() const
{
    return check_null().to_str();
}

const string
Value::sql_str() const
{
    if (is_null())
        return "NULL";
    return data_->to_sql_str();
}

int
Value::cmp(const Value &x) const
{
    if (is_null() && x.is_null())
        return 0;
    if (is_null())
        return -1;
    if (x.is_null())
        return 1;
    try {
        return data_->cmp(*x.data_);
    }
    catch (const std::bad_cast &) {
        if (get_type() == DECIMAL || x.get_type() == DECIMAL)
            return as_decimal().cmp(x.as_decimal());
        if (get_type() == LONGINT || x.get_type() == LONGINT) {
            LongInt a = as_longint(), b = x.as_longint();
            return a < b? -1: (a > b? 1: 0);
        }
        if (get_type() == DATETIME || x.get_type() == DATETIME) {
            DateTime a = as_date_time(), b = x.as_date_time();
            return a < b? -1: (a > b? 1: 0);
        }
        return as_string().compare(x.as_string());
    }
}

const Value
Value::get_typed_value(int type) const
{
    if (is_null())
        return *this;
    switch (type) {
        case Value::LONGINT:
            return as_longint();
        case Value::DECIMAL:
            return as_decimal();
        case Value::DATETIME:
            return as_date_time();
        case Value::STRING:
            return as_string();
        default:
            throw ValueBadCast(check_null().to_str(), Value::get_type_name(type));
    }
}

const ValueData &
Value::check_null() const
{
    if (is_null())
        throw ValueIsNull();
    return *data_;
}

const char *
Value::get_type_name(int type)
{
    static const char *type_names[] =
        { "Invalid", "LongInt", "String", "Decimal", "DateTime" };
    if (type < 0 || type >= sizeof(type_names)/sizeof(const char *))
        return "Unknown";
    return type_names[type];
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
