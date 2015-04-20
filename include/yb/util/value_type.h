// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__UTIL__VALUE_TYPE__INCLUDED
#define YB__UTIL__VALUE_TYPE__INCLUDED

#include <string.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <utility>
#include "util_config.h"
#include "data_types.h"
#include "nlogger.h"
#if defined(YB_USE_TUPLE)
#include <boost/tuple/tuple.hpp>
#endif
#if defined(YB_USE_STDTUPLE)
#include <tuple>
#endif

namespace Yb {

class YBUTIL_DECL ValueIsNull: public ValueError
{
public:
    ValueIsNull();
};

template <class T__> struct ValueTraits {};

#define YB_MAX(x, y) ((x) > (y)? (x): (y))
#define YB_NULL ::Yb::Value()

//! Variant data type for communication to the database layer
/** Value class objects can hold NULL values.  Copying such objects
 * is cheap because of shared pointer.
 * Value class also supports casting to several strict types.
 *
 * @remark Value class should not be implemented upon boost::any because of massive
 * copying of Value objects here and there.
 */
class YBUTIL_DECL Value
{
    void init();
    void destroy();
    void assign(const Value &other);

public:
    enum Type {
        INVALID = 0, INTEGER, LONGINT, STRING, DECIMAL, DATETIME, FLOAT, BLOB
    };
    Value();
    Value(const int &x);
    Value(const LongInt &x);
    Value(const double &x);
    Value(const Decimal &x);
    Value(const DateTime &x);
    Value(const String &x);
    Value(const Char *x);
    Value(const Blob &x);
    Value(const Value &other);
    Value &operator=(const Value &other);
    ~Value();
    void swap(Value &other);
    void fix_type(int type);

    int as_integer() const;
    LongInt as_longint() const;
    const String as_string() const;
    const Decimal as_decimal() const;
    const DateTime as_date_time() const;
    double as_float() const;
    const Blob as_blob() const;
    const String sql_str() const;
    const Value nvl(const Value &def_value) const;
    int cmp(const Value &x) const;
    bool is_null() const { return type_ == INVALID; }
    int get_type() const { return type_; }
    static const String get_type_name(int type);

    // raw read with no checks and no conversions
    const int &read_as_integer() const;
    const LongInt &read_as_longint() const;
    const double &read_as_float() const;
    const Decimal &read_as_decimal() const;
    const DateTime &read_as_datetime() const;
    const String &read_as_string() const;
    const Blob &read_as_blob() const;

    template <class T__>
    const T__ &read_as() const {
        YB_ASSERT((int)type_ == (int)ValueTraits<T__>::TYPE_CODE);
        return ValueTraits<T__>::read(*this);
    }

private:
    int type_;
    char bytes_[YB_MAX(sizeof(int), YB_MAX(sizeof(LongInt),
                YB_MAX(sizeof(String), YB_MAX(sizeof(Decimal),
                YB_MAX(sizeof(DateTime), YB_MAX(sizeof(double), sizeof(Blob)))))))];
};

template <> struct ValueTraits<int> {
    enum { TYPE_CODE = (int)Value::INTEGER };
    static const int &read(const Value &x)
        { return x.read_as_integer(); }
    static int &from_variant(const Value &x, int &t) {
        t = x.as_integer();
        return t;
    }
};
template <> struct ValueTraits<LongInt> {
    enum { TYPE_CODE = (int)Value::LONGINT };
    static const LongInt &read(const Value &x)
        { return x.read_as_longint(); }
    static LongInt &from_variant(const Value &x, LongInt &t) {
        t = x.as_longint();
        return t;
    }
};
template <> struct ValueTraits<String> {
    enum { TYPE_CODE = (int)Value::STRING };
    static const String &read(const Value &x)
        { return x.read_as_string(); }
    static String &from_variant(const Value &x, String &t) {
        t = x.as_string();
        return t;
    }
};
template <> struct ValueTraits<Decimal> {
    enum { TYPE_CODE = (int)Value::DECIMAL };
    static const Decimal &read(const Value &x)
        { return x.read_as_decimal(); }
    static Decimal &from_variant(const Value &x, Decimal &t) {
        t = x.as_decimal();
        return t;
    }
};
template <> struct ValueTraits<DateTime> {
    enum { TYPE_CODE = (int)Value::DATETIME };
    static const DateTime &read(const Value &x)
        { return x.read_as_datetime(); }
    static DateTime &from_variant(const Value &x, DateTime &t) {
        t = x.as_date_time();
        return t;
    }
};
template <> struct ValueTraits<double> {
    enum { TYPE_CODE = (int)Value::FLOAT };
    static const double &read(const Value &x)
        { return x.read_as_float(); }
    static double &from_variant(const Value &x, double &t) {
        t = x.as_float();
        return t;
    }
};

template <> struct ValueTraits<Blob> {
    enum { TYPE_CODE = (int)Value::BLOB };
    static const Blob &read(const Value &x)
        { return x.read_as_blob(); }
    static Blob &from_variant(const Value &x, Blob &t) {
        t = x.as_blob();
        return t;
    }
};

//! @name Overloaded operations for Value class
//! @{
inline bool operator==(const Value &x, const Value &y) { return !x.cmp(y); }
inline bool operator!=(const Value &x, const Value &y) { return x.cmp(y) != 0; }
inline bool operator<(const Value &x, const Value &y) { return x.cmp(y) < 0; }
inline bool operator>=(const Value &x, const Value &y) { return x.cmp(y) >= 0; }
inline bool operator>(const Value &x, const Value &y) { return x.cmp(y) > 0; }
inline bool operator<=(const Value &x, const Value &y) { return x.cmp(y) <= 0; }
//! @}

typedef std::vector<Value> Values;
typedef const Values *RowDataPtr;
typedef std::vector<RowDataPtr> RowsData;
typedef std::vector<std::pair<String, Value> > ValueMap;
typedef std::pair<String, ValueMap> Key;
typedef std::vector<Key> Keys;

YBUTIL_DECL bool empty_key(const Key &key);

//! @name Casting from variant typed to certain type
//! @{
template <class T__>
inline T__ &from_variant(const Value &x, T__ &t) {
    return ValueTraits<T__>::from_variant(x, t);
}
//! @}

#if defined(YB_USE_TUPLE)
inline void tuple_values(const boost::tuples::null_type &, Values &values)
{}

template <class H, class T>
inline void tuple_values(const boost::tuples::cons<H, T> &item, Values &values)
{
    values.push_back(Value(item.get_head()));
    tuple_values(item.get_tail(), values);
}
#endif // defined(YB_USE_TUPLE)

#if defined(YB_USE_STDTUPLE)
template <int I, class T>
inline typename std::enable_if<I == std::tuple_size<T>::value, void>::type
stdtuple_values(const T &t, Values &values)
{}

template <int I, class T>
inline typename std::enable_if<I != std::tuple_size<T>::value, void>::type
stdtuple_values(const T &t, Values &values)
{
    values.push_back(Value(std::get<I>(t)));
    stdtuple_values<I + 1, T>(t, values);
}
#endif // defined(YB_USE_STDTUPLE)

} // namespace Yb

namespace std {

template<>
inline void swap(::Yb::Value &x, ::Yb::Value &y) { x.swap(y); }

} // namespace std

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__UTIL__VALUE_TYPE__INCLUDED
