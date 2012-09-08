// -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#ifndef YB__ORM__VALUE__INCLUDED
#define YB__ORM__VALUE__INCLUDED

#include <string>
#include <vector>
#include <set>
#include <map>
#include <stdexcept>
#include <utility>
#include <util/DataTypes.h>
#include <util/nlogger.h>

namespace Yb {

class ValueIsNull : public ValueError
{
public:
    ValueIsNull()
        :ValueError(_T("Trying to get value of null"))
    {}
};

struct ValueHolderBase {
    virtual ~ValueHolderBase();
};

//! Variant data type for communication to the database layer
/** Value class objects can hold NULL values.  Copying such objects
 * is cheap because of shared pointer.
 * Value class also supports casting to several strict types.
 * 
 * @remark Value class should not be implemented upon boost::any because of massive
 * copying of Value objects here and there.
 */
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
    static const String get_type_name(int type);
private:
    int type_;
    SharedPtr<ValueHolderBase>::Type data_;
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
typedef std::map<String, Value> ValueMap;
typedef std::pair<String, ValueMap> Key;
typedef std::vector<Key> Keys;

//! @name Casting from variant typed to certain type
//! @{
template <class T>
inline T &from_variant(const Value &x, T &t) {
    return from_string(x.as_string(), t);
}

template <>
inline int &from_variant(const Value &x, int &t) {
    t = x.as_integer();
    return t;
}

template <>
inline LongInt &from_variant(const Value &x, LongInt &t) {
    t = x.as_longint();
    return t;
}

template <>
inline Decimal &from_variant(const Value &x, Decimal &t) {
    t = x.as_decimal();
    return t;
}

template <>
inline DateTime &from_variant(const Value &x, DateTime &t) {
    t = x.as_date_time();
    return t;
}
//! @}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__VALUE__INCLUDED
