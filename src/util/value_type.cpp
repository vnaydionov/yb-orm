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
T__ &get_as(char *bytes) {
    return *reinterpret_cast<T__ *>(bytes);
}

template <class T__>
const T__ &get_as(const char *bytes) {
    return *reinterpret_cast<const T__ *>(bytes);
}

template <class T__>
void cons_as(char *bytes) {
    new (bytes) T__();
}

template <class T__>
void copy_as(char *bytes, const T__ &x) {
    new (bytes) T__(x);
}

template <class T__>
void des_as(char *bytes) {
    get_as<T__>(bytes).~T__();
}

ValueIsNull::ValueIsNull()
    : ValueError(_T("Trying to get value of null"))
{}

void
Value::init()
{
    switch (type_) {
    case STRING:
        cons_as<String>(bytes_);
        break;
    case DECIMAL:
        cons_as<Decimal>(bytes_);
        break;
    case DATETIME:
        cons_as<DateTime>(bytes_);
        break;
    case BLOB:
        cons_as<Blob>(bytes_);
        break;
    }
}

void
Value::destroy()
{
    switch (type_) {
    case STRING:
        des_as<String>(bytes_);
        break;
    case DECIMAL:
        des_as<Decimal>(bytes_);
        break;
    case DATETIME:
        des_as<DateTime>(bytes_);
        break;
    case BLOB:
        des_as<Blob>(bytes_);
        break;
    }
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
        get_as<int>(bytes_) = get_as<int>(other.bytes_);
        break;
    case LONGINT:
        get_as<LongInt>(bytes_) = get_as<LongInt>(other.bytes_);
        break;
    case STRING:
        get_as<String>(bytes_) = get_as<String>(other.bytes_);
        break;
    case DECIMAL:
        get_as<Decimal>(bytes_) = get_as<Decimal>(other.bytes_);
        break;
    case DATETIME:
        get_as<DateTime>(bytes_) = get_as<DateTime>(other.bytes_);
        break;
    case FLOAT:
        get_as<double>(bytes_) = get_as<double>(other.bytes_);
        break;
    case BLOB:
        get_as<Blob>(bytes_) = get_as<Blob>(other.bytes_);
        break;
    }
}

const int &
Value::read_as_integer() const { return get_as<int>(bytes_); }

const LongInt &
Value::read_as_longint() const { return get_as<LongInt>(bytes_); }

const String &
Value::read_as_string() const { return get_as<String>(bytes_); }

const Decimal &
Value::read_as_decimal() const { return get_as<Decimal>(bytes_); }

const DateTime &
Value::read_as_datetime() const { return get_as<DateTime>(bytes_); }

const double &
Value::read_as_float() const { return get_as<double>(bytes_); }

const Blob &
Value::read_as_blob() const { return get_as<Blob>(bytes_); }

Value::Value()
    : type_(INVALID)
{
    memset(bytes_, 0, sizeof(bytes_));
}

Value::Value(const int &x)
    : type_(INTEGER)
{
    memset(bytes_, 0, sizeof(bytes_));
    copy_as<int>(bytes_, x);
}

Value::Value(const LongInt &x)
    : type_(LONGINT)
{
    memset(bytes_, 0, sizeof(bytes_));
    copy_as<LongInt>(bytes_, x);
}

Value::Value(const double &x)
    : type_(FLOAT)
{
    memset(bytes_, 0, sizeof(bytes_));
    copy_as<double>(bytes_, x);
}

Value::Value(const Decimal &x)
    : type_(DECIMAL)
{
    memset(bytes_, 0, sizeof(bytes_));
    copy_as<Decimal>(bytes_, x);
}

Value::Value(const DateTime &x)
    : type_(DATETIME)
{
    memset(bytes_, 0, sizeof(bytes_));
    copy_as<DateTime>(bytes_, x);
}

Value::Value(const String &x)
    : type_(STRING)
{
    memset(bytes_, 0, sizeof(bytes_));
    copy_as<String>(bytes_, x);
}

Value::Value(const Char *x)
    : type_(x != NULL? STRING: INVALID)
{
    memset(bytes_, 0, sizeof(bytes_));
    if (x != NULL)
        copy_as<String>(bytes_, str_from_chars(x));
}

Value::Value(const Blob &x)
    : type_(BLOB)
{
    memset(bytes_, 0, sizeof(bytes_));
    copy_as<Blob>(bytes_, x);
}

Value::Value(const Value &other)
    : type_(INVALID)
{
    memset(bytes_, 0, sizeof(bytes_));
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

void
Value::swap(Value &other)
{
    if (this == &other)
        return;
    type_ ^= other.type_;
    other.type_ ^= type_;
    type_ ^= other.type_;
    char *p1 = &bytes_[0], *p2 = &other.bytes_[0];
    char *limit1 = p1 + sizeof(bytes_);
    for (; p1 != limit1; ++p1, ++p2) {
        *p1 ^= *p2;
        *p2 ^= *p1;
        *p1 ^= *p2;
    }
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
            get_as<int>(bytes_) = t;
        }
        break;
    case Value::LONGINT:
        {
            LongInt t = as_longint();
            destroy();
            type_ = type;
            get_as<LongInt>(bytes_) = t;
        }
        break;
    case Value::STRING:
        {
            String t = as_string();
            destroy();
            type_ = type;
            init();
            get_as<String>(bytes_) = t;
        }
        break;
    case Value::DECIMAL:
        {
            Decimal t = as_decimal();
            destroy();
            type_ = type;
            init();
            get_as<Decimal>(bytes_) = t;
        }
        break;
    case Value::DATETIME:
        {
            DateTime t = as_date_time();
            destroy();
            type_ = type;
            init();
            get_as<DateTime>(bytes_) = t;
        }
        break;
    case Value::FLOAT:
        {
            double t = as_float();
            destroy();
            type_ = type;
            get_as<double>(bytes_) = t;
        }
        break;
    case Value::BLOB:
        {
            Blob t = as_blob();
            destroy();
            type_ = type;
            init();
            get_as<Blob>(bytes_) = t;
        }
        break;
    }
}

int
Value::as_integer() const
{
    if (type_ == INTEGER)
        return get_as<int>(bytes_);
    if (type_ == LONGINT) {
        LongInt x = get_as<LongInt>(bytes_);
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
        return get_as<LongInt>(bytes_);
    if (type_ == INTEGER)
        return get_as<int>(bytes_);
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
        return get_as<Decimal>(bytes_);
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
        return get_as<DateTime>(bytes_);
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
        return get_as<double>(bytes_);
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
        return get_as<Blob>(bytes_);
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
        return to_string(get_as<int>(bytes_));
    case LONGINT:
        return to_string(get_as<LongInt>(bytes_));
    case STRING:
        return get_as<String>(bytes_);
    case DECIMAL:
        return to_string(get_as<Decimal>(bytes_));
    case DATETIME:
        return to_string(get_as<DateTime>(bytes_));
    case FLOAT:
        return to_string(get_as<double>(bytes_));
    case BLOB:
        {
            //Blob x = get_as<Blob>(bytes_);
            std::string s;
            const Blob &b = read_as_blob();
            std::copy(b.begin(), b.end(), std::back_inserter(s));
            return WIDEN(s);
        }
        //return to_string(get_as<Blob>(bytes_));
    }
    throw ValueBadCast(_T("UnknownType"), _T("Value::as_string()"));
}

const String
Value::sql_str() const
{
    if (is_null())
        return _T("NULL");
    if (type_ == STRING)
        return quote(sql_string_escape(get_as<String>(bytes_)));
    if (type_ == BLOB)
        return quote(sql_string_escape(as_string()));
    else if (type_ == DATETIME) {
        String t(to_string(get_as<DateTime>(bytes_)));
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
        return get_as<String>(bytes_).compare(get_as<String>(x.bytes_));
    if ((type_ == LONGINT || type_ == INTEGER) &&
        (x.type_ == LONGINT || x.type_ == INTEGER))
    {
        LongInt a, b;
        if (type_ == INTEGER)
            a = get_as<int>(bytes_);
        else
            a = get_as<LongInt>(bytes_);
        if (x.type_ == INTEGER)
            b = get_as<int>(x.bytes_);
        else
            b = get_as<LongInt>(x.bytes_);
        return a < b? -1: (a > b? 1: 0);
    }
    if (type_ == DATETIME && x.type_ == DATETIME)
    {
        const DateTime &a = get_as<DateTime>(bytes_), &b = get_as<DateTime>(x.bytes_);
        return a < b? -1: (a > b? 1: 0);
    }
    if (type_ == DECIMAL && x.type_ == DECIMAL)
        return get_as<Decimal>(bytes_).cmp(get_as<Decimal>(x.bytes_));
    if (type_ == FLOAT && x.type_ == FLOAT)
    {
        const double &a = get_as<double>(bytes_), &b = get_as<double>(x.bytes_);
        return a < b? -1: (a > b? 1: 0);
    }
    if (type_ == BLOB && x.type_ == BLOB)
    {
        const Blob &a = get_as<Blob>(bytes_), &b = get_as<Blob>(x.bytes_);
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
