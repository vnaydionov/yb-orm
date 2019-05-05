// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#define YBUTIL_SOURCE

#include <time.h>
#include <stdio.h>
#include <iomanip>
#include "util/string_utils.h"
#include "util/value_type.h"

using namespace std;
using namespace Yb::StrUtils;

namespace Yb {

template <class T__>
static inline T__ &get_as(void *data) {
    return *reinterpret_cast<T__ *>(data);
}

template <class T__>
static inline const T__ &get_as(const void *data) {
    return *reinterpret_cast<const T__ *>(data);
}

const int &
Value::read_as_integer() const { return get_as<int>(&data_); }

const LongInt &
Value::read_as_longint() const { return get_as<LongInt>(&data_); }

const String &
Value::read_as_string() const { return *get_as<const String *>(&data_); }

const Decimal &
Value::read_as_decimal() const { return *get_as<const Decimal *>(&data_); }

const DateTime &
Value::read_as_datetime() const { return *get_as<const DateTime *>(&data_); }

const double &
Value::read_as_float() const { return get_as<double>(&data_); }

const Blob &
Value::read_as_blob() const { return *get_as<const Blob *>(&data_); }

ValueIsNull::ValueIsNull()
    : ValueError(_T("Trying to get value of null"))
{}

void
Value::init()
{
    switch (type_) {
    case STRING:
        get_as<String *>(&data_) = new String();
        break;
    case DECIMAL:
        get_as<Decimal *>(&data_) = new Decimal();
        break;
    case DATETIME:
        get_as<DateTime *>(&data_) = new DateTime();
        break;
    case BLOB:
        get_as<Blob *>(&data_) = new Blob();
        break;
    }
}

void
Value::destroy()
{
    switch (type_) {
    case STRING:
        delete get_as<String *>(&data_);
        break;
    case DECIMAL:
        delete get_as<Decimal *>(&data_);
        break;
    case DATETIME:
        delete get_as<DateTime *>(&data_);
        break;
    case BLOB:
        delete get_as<Blob *>(&data_);
        break;
    default:
        return;
    }
    get_as<void *>(&data_) = NULL;
}

void
Value::assign(const Value &other)
{
    if (type_ != other.type_) {
        destroy();
        type_ = other.type_;
        init();
    }
    switch (type_) {
    case INTEGER:
        get_as<int>(&data_) = other.read_as_integer();
        break;
    case LONGINT:
        get_as<LongInt>(&data_) = other.read_as_longint();
        break;
    case STRING:
        *get_as<String *>(&data_) = other.read_as_string();
        break;
    case DECIMAL:
        *get_as<Decimal *>(&data_) = other.read_as_decimal();
        break;
    case DATETIME:
        *get_as<DateTime *>(&data_) = other.read_as_datetime();
        break;
    case FLOAT:
        get_as<double>(&data_) = other.read_as_float();
        break;
    case BLOB:
        *get_as<Blob *>(&data_) = other.read_as_blob();
        break;
    }
}

Value::Value()
    : type_(INVALID)
    , data_(0)
{}

Value::Value(const int &x)
    : type_(INTEGER)
    , data_(0)
{
    get_as<int>(&data_) = x;
}

Value::Value(const LongInt &x)
    : type_(LONGINT)
    , data_(x)
{}

Value::Value(const double &x)
    : type_(FLOAT)
    , data_(0)
{
    get_as<double>(&data_) = x;
}

Value::Value(const Decimal &x)
    : type_(DECIMAL)
    , data_(0)
{
    get_as<Decimal *>(&data_) = new Decimal(x);
}

Value::Value(const DateTime &x)
    : type_(DATETIME)
    , data_(0)
{
    get_as<DateTime *>(&data_) = new DateTime(x);
}

Value::Value(const String &x)
    : type_(STRING)
    , data_(0)
{
    get_as<String *>(&data_) = new String(x);
}

Value::Value(const Char *x)
    : type_(x != NULL? STRING: INVALID)
    , data_(0)
{
    if (type_ == STRING)
        get_as<String *>(&data_) = new String(str_from_chars(x));
}

Value::Value(const Blob &x)
    : type_(BLOB)
    , data_(0)
{
    get_as<Blob *>(&data_) = new Blob(x);
}

Value::Value(const Value &other)
    : type_(INVALID)
    , data_(0)
{
    assign(other);
}

Value &
Value::operator=(const Value &other)
{
    if (this != &other)
        assign(other);
    return *this;
}

Value::~Value()
{
    destroy();
}

template <class Int__>
static inline void xor_swap(Int__ &a, Int__ &b) SWAP_NOEXCEPT
{
    a ^= b;
    b ^= a;
    a ^= b;
}

void
Value::swap(Value &other) SWAP_NOEXCEPT
{
    if (this == &other)
        return;
    xor_swap(type_, other.type_);
    xor_swap(data_, other.data_);
}

void
Value::fix_type(int type)
{
    if (type_ == type || type_ == INVALID)
        return;
    switch (type) {
    case Value::INVALID:
        {
            destroy();
            type_ = type;
        }
        break;
    case Value::INTEGER:
        {
            int t = as_integer();
            destroy();
            type_ = type;
            get_as<int>(&data_) = t;
        }
        break;
    case Value::LONGINT:
        {
            LongInt t = as_longint();
            destroy();
            type_ = type;
            get_as<LongInt>(&data_) = t;
        }
        break;
    case Value::STRING:
        {
            String t = as_string();
            destroy();
            type_ = type;
            init();
            *get_as<String *>(&data_) = t;
        }
        break;
    case Value::DECIMAL:
        {
            Decimal t = as_decimal();
            destroy();
            type_ = type;
            init();
            *get_as<Decimal *>(&data_) = t;
        }
        break;
    case Value::DATETIME:
        {
            DateTime t = as_date_time();
            destroy();
            type_ = type;
            init();
            *get_as<DateTime *>(&data_) = t;
        }
        break;
    case Value::FLOAT:
        {
            double t = as_float();
            destroy();
            type_ = type;
            get_as<double>(&data_) = t;
        }
        break;
    case Value::BLOB:
        {
            Blob t = as_blob();
            destroy();
            type_ = type;
            init();
            *get_as<Blob *>(&data_) = t;
        }
        break;
    }
}

int
Value::as_integer() const
{
    if (type_ == INTEGER)
        return read_as_integer();
    if (type_ == LONGINT) {
        const LongInt &x = read_as_longint();
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
        return read_as_longint();
    if (type_ == INTEGER)
        return read_as_integer();
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
        return read_as_decimal();
    String s = as_string();
    try {
        return Decimal(s);
    }
    catch (const DecimalException &) {
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
        catch (const DecimalException &) {
            throw ValueBadCast(s, _T("Decimal"));
        }
    }
}

const DateTime
Value::as_date_time() const
{
    if (type_ == DATETIME)
        return read_as_datetime();
    String s = as_string();
    try {
        if (s == _T("sysdate"))
            return now();
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

double
Value::as_float() const
{
    if (type_ == FLOAT)
        return read_as_float();
    String s = as_string();
    try {
        double x;
        return from_string(s, x);
    }
    catch (const std::exception &) {
        throw ValueBadCast(s, _T("Float"));
    }
}

const Blob
Value::as_blob() const
{
    if (type_ == BLOB)
        return read_as_blob();
    String s = as_string();
    try {
        Blob x;
        std::string t = NARROW(s);
        std::copy(t.begin(), t.end(), std::back_inserter(x));
        return x;
    }
    catch (const std::exception &) {
        throw ValueBadCast(s, _T("Blob"));
    }
}

const String
Value::as_string() const
{
    switch (type_) {
    case INVALID:
        throw ValueIsNull();
    case INTEGER:
        return to_string(read_as_integer());
    case LONGINT:
        return to_string(read_as_longint());
    case STRING:
        return read_as_string();
    case DECIMAL:
        return to_string(read_as_decimal());
    case DATETIME:
        return to_string(read_as_datetime());
    case FLOAT:
        return to_string(read_as_float());
    case BLOB:
        {
            std::string s;
            const Blob &b = read_as_blob();
            std::copy(b.begin(), b.end(), std::back_inserter(s));
            return WIDEN(s);
        }
    }
    throw ValueBadCast(_T("UnknownType"), _T("Value::as_string()"));
}

const String
Value::sql_str() const
{
    if (is_null())
        return _T("NULL");
    if (type_ == STRING)
        return quote(sql_string_escape(read_as_string()));
    if (type_ == BLOB)
        return quote(sql_string_escape(as_string()));
    else if (type_ == DATETIME) {
        String t(to_string(read_as_datetime()));
        int pos = str_find(t, _T('T'));
        if (pos != -1)
            t[pos] = _T(' ');
        return _T("'") + t + _T("'");
    }
    return as_string();
}

const Value
Value::nvl(const Value &def_value) const
{
    return is_null()? def_value: *this;
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
    if (type_ == STRING && x.type_ == STRING)
        return read_as_string().compare(x.read_as_string());
    if ((type_ == LONGINT || type_ == INTEGER) &&
        (x.type_ == LONGINT || x.type_ == INTEGER))
    {
        LongInt a, b;
        if (type_ == INTEGER)
            a = read_as_integer();
        else
            a = read_as_longint();
        if (x.type_ == INTEGER)
            b = x.read_as_integer();
        else
            b = x.read_as_longint();
        return a < b? -1: (a > b? 1: 0);
    }
    if (type_ == DATETIME && x.type_ == DATETIME)
    {
        const DateTime &a = read_as_datetime(), &b = x.read_as_datetime();
        return a < b? -1: (a > b? 1: 0);
    }
    if (type_ == DECIMAL && x.type_ == DECIMAL)
        return read_as_decimal().cmp(x.read_as_decimal());
    if (type_ == FLOAT && x.type_ == FLOAT)
    {
        const double &a = read_as_float(), &b = x.read_as_float();
        return a < b? -1: (a > b? 1: 0);
    }
    if (type_ == BLOB && x.type_ == BLOB)
    {
        const Blob &a = read_as_blob(), &b = x.read_as_blob();
        return a < b? -1: (a > b? 1: 0);
    }
    return as_string().compare(x.as_string());
}

const String
Value::get_type_name(int type)
{
    static const char *type_names[] =
        { "Invalid", "Integer", "LongInt",
          "String", "Decimal", "DateTime", "Float", "Blob" };
    if (type < 0 || type >= sizeof(type_names)/sizeof(const char *))
        return WIDEN("UnknownType");
    return WIDEN(type_names[type]);
}

YBUTIL_DECL int
key_cmp(const Key &x, const Key &y)
{
    int r;
    r = (int)!!x.table - (int)!!y.table;
    if (r || !x.table)
        return r;
    r = CharBuf<Char>::x_strcmp(str_data(*x.table),
                                str_data(*y.table));
    if (r)
        return r;
    r = (int)!!x.id_name - (int)!!y.id_name;
    if (r)
        return r;
    if (x.id_name) {
        r = CharBuf<Char>::x_strcmp(str_data(*x.id_name),
                                    str_data(*y.id_name));
        if (r)
            return r;
        r = (int)!x.id_is_null - (int)!y.id_is_null;
        if (r || x.id_is_null)
            return r;
        LongInt lr = x.id_value - y.id_value;
        if (lr)
            return lr < 0? -1: 1;
        return 0;
    }
    r = (int)x.fields.size() - (int)y.fields.size();
    if (r)
        return r < 0? -1: 1;
    for (size_t i = 0; i < x.fields.size(); ++i) {
        r = CharBuf<Char>::x_strcmp(str_data(*x.fields[i].first),
                                    str_data(*y.fields[i].first));
        if (r)
            return r;
        r = x.fields[i].second.cmp(y.fields[i].second);
        if (r)
            return r;
    }
    return 0;
}

YBUTIL_DECL bool
empty_key(const Key &key)
{
    if (!key.table)
        return true;
    if (key.id_name)
        return key.id_is_null;
    ValueMap::const_iterator i = key.fields.begin(), iend = key.fields.end();
    if (i == iend)
        return true;
    for (; i != iend; ++i)
        if (i->second.is_null() || (
                    i->second.get_type() == Value::STRING &&
                    str_empty(i->second.read_as_string())))
            return true;
    return false;
}

YBUTIL_DECL const String
key2str(const Key &key)
{
    if (!key.table)
        return _T("Key()");
    String r;
    r.reserve(80);
    if (key.id_name) {
        r += _T("Key('");
        r += *key.table;
        r += _T("', {'");
        r += *key.id_name;
        r += _T("': ");
        if (key.id_is_null)
            r += _T("NULL");
        else
            r += to_string(key.id_value);
        r += _T("})");
    }
    else {
        r += _T("Key('");
        r += *key.table;
        r += _T("', {");
        ValueMap::const_iterator i = key.fields.begin(),
                                 iend = key.fields.end();
        for (bool first = true; i != iend; ++i, first = false) {
            if (!first)
                r += _T(", ");
            r += _T("'");
            r += *i->first;
            r += _T("': ");
            r += i->second.sql_str();
        }
        r += _T("})");
    }
    return r;
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
