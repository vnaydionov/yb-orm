
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
    PKIDAlreadySynced(const std::string &table_name, long long id)
        : logic_error("PKID is already synchronized: " + 
                table_name + "(" +
                boost::lexical_cast<std::string>(id) + ")")
    {}
};

class PKIDNotSynced: public std::logic_error
{
public:
    PKIDNotSynced(const std::string &table_name, long long id)
        : logic_error("PKID is temporary: " + table_name + "(" +
                boost::lexical_cast<std::string>(id) + ")")
    {}
};

namespace ORMapper {

class TableMetaData;

struct PKIDRecord
{
    const ORMapper::TableMetaData *table_;
    long long pkid_;
    bool is_temp_;
    PKIDRecord(const ORMapper::TableMetaData *table,
            long long pkid, bool is_temp)
        : table_(table)
        , pkid_(pkid)
        , is_temp_(is_temp)
    {}
};

} // namespace ORMapper

class PKIDValue
{
    const ORMapper::TableMetaData *table_;
    std::pair<std::string, long long> key_;
    mutable long long pkid_;
    mutable boost::shared_ptr<ORMapper::PKIDRecord> temp_;
public:
    PKIDValue();
    PKIDValue(const ORMapper::TableMetaData &table,
            boost::shared_ptr<ORMapper::PKIDRecord> temp);
    PKIDValue(const ORMapper::TableMetaData &table, long long pkid);
    const ORMapper::TableMetaData &get_table() const;
    const std::pair<std::string, long long> &get_key() const { return key_; }
    bool is_temp() const;
    long long as_long_long() const;
    void sync(long long pkid) const;
    bool eq(const PKIDValue &x) const;
    bool lt(const PKIDValue &x) const;
    bool operator == (const PKIDValue &x) const { return eq(x); }
    bool operator < (const PKIDValue &x) const { return lt(x); }
};

class ValueData;

class Value
{
public:
    enum Type { Invalid, LongLong, String, Decimal, DateTime, PKID };
    Value();
    Value(long long x);
    Value(const char *x);
    Value(const std::string &x);
    Value(const decimal &x);
    Value(const boost::posix_time::ptime &x);
    Value(const PKIDValue &x);
    bool is_null() const;
    long long as_long_long() const;
    const std::string as_string() const;
    const decimal as_decimal() const;
    const boost::posix_time::ptime as_date_time() const;
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

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et

#endif // YB__ORM__VALUE__INCLUDED

