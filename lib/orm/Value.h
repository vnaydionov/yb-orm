#ifndef YB__ORM__VALUE__INCLUDED
#define YB__ORM__VALUE__INCLUDED

#include <string>
#include <vector>
#include <stdexcept>
#include <utility>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <util/decimal.h>

#ifdef __WIN32__
struct tm *localtime_r(const time_t *clock, struct tm *result);
#endif

namespace Yb {

#ifdef _MSC_VER
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

const DateTime mk_datetime(const std::string &s);

inline const DateTime mk_datetime(int year, int month, int day,
        int hour = 0, int minute = 0, int second = 0)
{
    return DateTime(boost::gregorian::date(year, month, day),
            boost::posix_time::time_duration(hour, minute, second));
}

template <typename T>
inline std::string to_string(const T &x)
{
    return boost::lexical_cast<std::string>(x);
}

inline std::string to_string(const Decimal &x)
{
    return x.str();
}

inline std::string to_string(const DateTime &x)
{
    return boost::posix_time::to_iso_extended_string(x);
}

class ValueIsNull : public std::logic_error
{
public:
    ValueIsNull()
        :logic_error("Trying to get value of null")
    {}
};

class ValueBadCast : public std::logic_error
{
public:
    ValueBadCast(const std::string &value, const std::string &type)
        :logic_error("Can't cast value '" + value + "' to type " + type)
    {}
};

class PKIDInvalid: public std::logic_error
{
public:
    PKIDInvalid(): logic_error("Invalid PKID object") {}
};

class PKIDAlreadySynced: public std::logic_error
{
public:
    PKIDAlreadySynced(const std::string &table_name, LongInt id)
        : logic_error("PKID is already synchronized: " + 
                table_name + "(" +
                boost::lexical_cast<std::string>(id) + ")")
    {}
};

class PKIDNotSynced: public std::logic_error
{
public:
    PKIDNotSynced(const std::string &table_name, LongInt id)
        : logic_error("PKID is temporary: " + table_name + "(" +
                boost::lexical_cast<std::string>(id) + ")")
    {}
};

class TableMetaData;

struct PKIDRecord
{
    const TableMetaData *table_;
    LongInt pkid_;
    bool is_temp_;
    PKIDRecord(const TableMetaData *table,
            LongInt pkid, bool is_temp)
        : table_(table)
        , pkid_(pkid)
        , is_temp_(is_temp)
    {}
};

class PKIDValue
{
    const TableMetaData *table_;
    std::pair<std::string, LongInt> key_;
    mutable LongInt pkid_;
    mutable boost::shared_ptr<PKIDRecord> temp_;
public:
    PKIDValue();
    PKIDValue(const TableMetaData &table,
            boost::shared_ptr<PKIDRecord> temp);
    PKIDValue(const TableMetaData &table, LongInt pkid);
    const TableMetaData &get_table() const;
    const std::pair<std::string, LongInt> &get_key() const { return key_; }
    bool is_temp() const;
    LongInt as_longint() const;
    void sync(LongInt pkid) const;
    bool eq(const PKIDValue &x) const;
    bool lt(const PKIDValue &x) const;
    bool operator == (const PKIDValue &x) const { return eq(x); }
    bool operator < (const PKIDValue &x) const { return lt(x); }
};

inline std::string to_string(const PKIDValue &x)
{
    return boost::lexical_cast<std::string>(x.as_longint());
}

class ValueData;

class Value
{
public:
    enum Type { INVALID, LONGINT, STRING, DECIMAL, DATETIME, PKID };
    Value();
    Value(LongInt x);
    Value(const char *x);
    Value(const std::string &x);
    Value(const Decimal &x);
    Value(const DateTime &x);
    Value(const PKIDValue &x);
    bool is_null() const;
    LongInt as_longint() const;
    const std::string as_string() const;
    const Decimal as_decimal() const;
    const DateTime as_date_time() const;
    const PKIDValue as_pkid() const;
    const std::string sql_str() const;
    const Value nvl(const Value &def_value) const { return is_null()? def_value: *this; }
    bool eq(const Value &x) const;
    bool lt(const Value &x) const;
    int get_type() const { return type_; }
    const Value get_typed_value(int type) const;
    static const char *get_type_name(int type);
private:
    const ValueData &check_null() const;
private:
    boost::shared_ptr<ValueData> data_;
    Type type_;
};

inline bool operator==(const Value &x, const Value &y) { return x.eq(y); }
inline bool operator!=(const Value &x, const Value &y) { return !x.eq(y); }
inline bool operator<(const Value &x, const Value &y) { return x.lt(y); }
inline bool operator>=(const Value &x, const Value &y) { return !x.lt(y); }
inline bool operator>(const Value &x, const Value &y) { return y.lt(x); }
inline bool operator<=(const Value &x, const Value &y) { return !y.lt(x); }

typedef std::vector<Value> Values;
typedef std::vector<std::string> Names;

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__VALUE__INCLUDED
