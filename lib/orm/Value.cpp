#include <time.h>
#include <stdio.h>
#include <iomanip>
#include <util/str_utils.hpp>
#include <orm/Value.h>

using namespace std;
using namespace Yb::StrUtils;

namespace Yb {

ValueHolderBase::~ValueHolderBase() {}

template <class T>
struct ValueHolder: public ValueHolderBase {
    T d_;
    ValueHolder(const T &d): d_(d) {}
};

template <class T>
const T &typed_value(SharedPtr<ValueHolderBase>::Type data)
{
    return reinterpret_cast<ValueHolder<T> * > (shptr_get(data))->d_;
}

Value::Value()
    : type_(INVALID)
{}

Value::Value(int x)
    : type_(INTEGER)
    , data_(new ValueHolder<int>(x))
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

int
Value::as_integer() const
{
    if (type_ == INTEGER)
        return typed_value<int>(data_);
    if (type_ == LONGINT) {
        LongInt x = typed_value<LongInt>(data_);
        LongInt m = 0xFFFF;
        m = (m << 16) | m;
        LongInt h = (x >> 32) & m;
        if (h && h != m)
            throw ValueBadCast(to_string(x) + _T("LL"),
                    _T("Integer"));
        return (int)x;
    }
    String s = as_string();
    try {
        int x;
        return from_string(s, x);
    }
    catch (const std::exception &) {
        throw ValueBadCast(s, _T("Integer"));
    }
}

LongInt
Value::as_longint() const
{
    if (type_ == LONGINT)
        return typed_value<LongInt>(data_);
    if (type_ == INTEGER)
        return typed_value<int>(data_);
    String s = as_string();
    try {
        LongInt x;
        return from_string(s, x);
    }
    catch (const std::exception &) {
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
            from_string(s, f);
        }
        catch (const std::exception &) {
            throw ValueBadCast(s, _T("Decimal"));
        }
        std::ostringstream o;
        o << setprecision(10) << fixed << f;
        try {
            return Decimal(WIDEN(o.str()));
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
        DateTime x;
        return from_string(s, x);
    }
    catch (const std::exception &) {
        time_t t;
        from_string(s, t);
        tm stm;
        if (!localtime_safe(&t, &stm))
            throw ValueBadCast(s, _T("struct tm"));
        return dt_make(stm.tm_year + 1900,
                stm.tm_mon + 1, stm.tm_mday,
                stm.tm_hour, stm.tm_min, stm.tm_sec);
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
    if (type_ == INTEGER)
        return to_string(typed_value<int>(data_));
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
        int pos = str_find(t, _T('T'));
        if (pos != -1)
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
    if (type_ == LONGINT || x.type_ == LONGINT || 
            type_ == INTEGER || x.type_ == INTEGER)
    {
        LongInt a, b;
        if (type_ == INTEGER)
            a = as_integer();
        else
            a = as_longint();
        if (x.type_ == INTEGER)
            b = x.as_integer();
        else
            b = x.as_longint();
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
        case Value::INTEGER:
            return as_integer();
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

const String
Value::get_type_name(int type)
{
    static const char *type_names[] =
        { "Invalid", "Integer", "LongInt",
          "String", "Decimal", "DateTime" };
    if (type < 0 || type >= sizeof(type_names)/sizeof(const char *))
        return WIDEN("UnknownType");
    return WIDEN(type_names[type]);
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
