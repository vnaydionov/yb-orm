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

inline bool operator==(const Value &x, const Value &y) { return !x.cmp(y); }
inline bool operator!=(const Value &x, const Value &y) { return x.cmp(y) != 0; }
inline bool operator<(const Value &x, const Value &y) { return x.cmp(y) < 0; }
inline bool operator>=(const Value &x, const Value &y) { return x.cmp(y) >= 0; }
inline bool operator>(const Value &x, const Value &y) { return x.cmp(y) > 0; }
inline bool operator<=(const Value &x, const Value &y) { return x.cmp(y) <= 0; }

typedef std::vector<Value> Values;
typedef std::map<String, Value> ValueMap;
typedef std::pair<String, ValueMap> Key;
typedef std::vector<Key> Keys;

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__VALUE__INCLUDED
