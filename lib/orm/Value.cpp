#include <time.h>
#include <stdio.h>
#include <iomanip>
#include <boost/lexical_cast.hpp>
#include <boost/thread/mutex.hpp>
#include <util/str_utils.hpp>
#include <util/nlogger.h>
#include <orm/Value.h>

using namespace std;
using namespace Yb::StrUtils;

namespace Yb {

static int extract_int(const Char *s, size_t len)
{
    int a = 0;
    for (; len > 0 && *s >= _T('0') && *s <= _T('9');
         a = a*10 + *s - _T('0'), --len, ++s);
    return len == 0? a: -1;
}

const DateTime mk_datetime(const String &s)
{
#if 0
    String::size_type pos = s.find(_T('T'));
    if (pos == String::npos)
        return boost::posix_time::time_from_string(s);
    String t(s);
    t[pos] = _T(' ');
    return boost::posix_time::time_from_string(t);
#else
    if (s.size() < 19 || s[4] != _T('-') || s[7] != _T('-') ||
        (s[10] != _T(' ') && s[10] != _T('T')) || s[13] != _T(':') ||
        s[16] != _T(':'))
    {
        throw ValueBadCast(s, _T("YYYY-MM-DD HH:MI:SS"));
    }
    const Char *cs = s.c_str();
    int year = extract_int(cs, 4), month = extract_int(cs + 5, 2),
        day = extract_int(cs + 8, 2), hours = extract_int(cs + 11, 2),
        minutes = extract_int(cs + 14, 2), seconds = extract_int(cs + 17, 2);
    if (year < 0 || month < 0 || day < 0
        || hours < 0 || minutes < 0 || seconds < 0)
    {
        throw ValueBadCast(s, _T("YYYY-MM-DD HH:MI:SS"));
    }
    return mk_datetime(year, month, day, hours, minutes, seconds);
#endif
}

const String ptime2str(const boost::posix_time::ptime &t)
{
#if 0
    Char buf[80];
    stprintf(buf, _T("%04d-%02d-%02dT%02d:%02d:%02d"),
        (int)t.date().year(), (int)t.date().month(),
        (int)t.date().day(), (int)t.time_of_day().hours(),
        (int)t.time_of_day().minutes(), (int)t.time_of_day().seconds());
    return String(buf);
#else
    OStringStream o;
    o << setfill(_T('0'))
        << setw(4) << t.date().year()
        << _T('-') << setw(2) << (int)t.date().month()
        << _T('-') << setw(2) << t.date().day()
        << _T('T') << setw(2) << t.time_of_day().hours()
        << _T(':') << setw(2) << t.time_of_day().minutes()
        << _T(':') << setw(2) << t.time_of_day().seconds();
    return o.str();
#endif
}

struct ValueHolderBase {
    virtual ~ValueHolderBase() {}
};

template <class T>
struct ValueHolder: public ValueHolderBase {
    T d_;
    ValueHolder(const T &d): d_(d) {}
};

template <class T>
const T &typed_value(boost::shared_ptr<ValueHolderBase> data)
{
    return reinterpret_cast<ValueHolder<T> * > (data.get())->d_;
}

Value::Value()
    : type_(INVALID)
{}

Value::Value(LongInt x)
    : type_(LONGINT)
    , data_(new ValueHolder<LongInt>(x))
{}

Value::Value(const Char *x)
    : type_(STRING)
    , data_(new ValueHolder<String>(String(x)))
{}

Value::Value(const String &x)
    : type_(STRING)
    , data_(new ValueHolder<String>(x))
{}

Value::Value(const Decimal &x)
    : type_(DECIMAL)
    , data_(new ValueHolder<Decimal>(x))
{}

Value::Value(const DateTime &x)
    : type_(DATETIME)
    , data_(new ValueHolder<DateTime>(x))
{}

LongInt
Value::as_longint() const
{
    if (type_ == LONGINT)
        return typed_value<LongInt>(data_);
    String s = as_string();
    try {
        return boost::lexical_cast<LongInt>(s);
    }
    catch (const boost::bad_lexical_cast &) {
        throw ValueBadCast(s, _T("LongInt"));
    }
}

const Decimal
Value::as_decimal() const
{
    if (type_ == DECIMAL)
        return typed_value<Decimal>(data_);
    String s = as_string();
    try {
        return Decimal(s);
    }
    catch (const Decimal::exception &) {
        double f = 0.0;
        try {
            f = boost::lexical_cast<double>(s);
        }
        catch (const boost::bad_lexical_cast &) {
            throw ValueBadCast(s, _T("Decimal"));
        }
        OStringStream o;
        o << setprecision(10) << fixed << f;
        try {
            return Decimal(o.str());
        }
        catch (const Decimal::exception &) {
            throw ValueBadCast(s, _T("Decimal"));
        }
    }
}

const DateTime
Value::as_date_time() const
{
    if (type_ == DATETIME)
        return typed_value<DateTime>(data_);
    String s = as_string();
    try {
        time_t t = boost::lexical_cast<time_t>(s);
        tm stm;
        if (!localtime_safe(&t, &stm))
            throw ValueBadCast(s, _T("struct tm"));
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

const String
Value::as_string() const
{
    if (type_ == INVALID)
        throw ValueIsNull();
    if (type_ == STRING)
        return typed_value<String>(data_);
    if (type_ == DECIMAL)
        return to_string(typed_value<Decimal>(data_));
    if (type_ == LONGINT)
        return to_string(typed_value<LongInt>(data_));
    if (type_ == DATETIME)
        return to_string(typed_value<DateTime>(data_));
    throw ValueBadCast(_T("UnknownType"), _T("Value::as_string()"));
}

const String
Value::sql_str() const
{
    if (is_null())
        return _T("NULL");
    if (type_ == STRING)
        return quote(sql_string_escape(typed_value<String>(data_)));
    else if (type_ == DATETIME) {
        String t(to_string(typed_value<DateTime>(data_)));
        size_t pos = t.find(_T('T'));
        if (pos != String::npos)
            t[pos] = _T(' ');
        return _T("'") + t + _T("'");
    }
    return as_string();
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
    if (type_ == DECIMAL || x.type_ == DECIMAL)
    {
        return as_decimal().cmp(x.as_decimal());
    }
    if (type_ == LONGINT || x.type_ == LONGINT)
    {
        LongInt a = as_longint(), b = x.as_longint();
        return a < b? -1: (a > b? 1: 0);
    }
    if (type_ == DATETIME || x.type_ == DATETIME)
    {
        DateTime a = as_date_time(), b = x.as_date_time();
        return a < b? -1: (a > b? 1: 0);
    }
    return as_string().compare(x.as_string());
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
            throw ValueBadCast(as_string(), Value::get_type_name(type));
    }
}

const Char *
Value::get_type_name(int type)
{
    static const Char *type_names[] =
        { _T("Invalid"), _T("LongInt"), _T("String"), _T("Decimal"), _T("DateTime") };
    if (type < 0 || type >= sizeof(type_names)/sizeof(const Char *))
        return _T("UnknownType");
    return type_names[type];
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
