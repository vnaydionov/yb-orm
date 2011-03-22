
#include <time.h>
#include <sstream>
#include <iomanip>
#include <boost/lexical_cast.hpp>
#include <util/str_utils.hpp>
#include "Value.h"
#include "MetaData.h"
#include "Mapper.h"

using namespace std;
using namespace Yb::StrUtils;

namespace Yb {

class ValueData
{
public:
    virtual const string to_str() const = 0;
    virtual const string to_sql_str() const = 0;
    virtual bool eq(const ValueData &v) const = 0;
    virtual bool lt(const ValueData &v) const = 0;
    virtual ~ValueData() {}
};

class ValueDataLongLong : public ValueData
{
    long long x_;
public:
    ValueDataLongLong(long long x = 0LL) : x_(x) {}
    long long get() const { return x_; }
    const string to_str() const { return boost::lexical_cast<string>(x_); }
    const string to_sql_str() const { return to_str(); }
    bool eq(const ValueData &v) const
    {
        const ValueDataLongLong &d = dynamic_cast<const ValueDataLongLong &>(v);
        return x_ == d.x_;
    }
    bool lt(const ValueData &v) const
    {
        const ValueDataLongLong &d = dynamic_cast<const ValueDataLongLong &>(v);
        return x_ < d.x_;
    }
};

class ValueDataString : public ValueData
{
    string x_;
public:
    ValueDataString(const string &x = "") : x_(x) {}
    const string to_str() const { return x_; }
    const string to_sql_str() const
    {
        return quote(sql_string_escape(x_));
    }
    bool eq(const ValueData &v) const
    {
        const ValueDataString &d = dynamic_cast<const ValueDataString &>(v);
        return x_ == d.x_;
    }
    bool lt(const ValueData &v) const
    {
        const ValueDataString &d = dynamic_cast<const ValueDataString &>(v);
        return x_ < d.x_;
    }
};

class ValueDataDecimal : public ValueData
{
    decimal x_;
public:
    ValueDataDecimal(const decimal &x = decimal()) : x_(x) {}
    const decimal &get() const { return x_; }
    const string to_str() const { return x_.str(); }
    const string to_sql_str() const { return to_str(); }
    bool eq(const ValueData &v) const
    {
        const ValueDataDecimal &d = dynamic_cast<const ValueDataDecimal &>(v);
        return x_ == d.x_;
    }
    bool lt(const ValueData &v) const
    {
        const ValueDataDecimal &d = dynamic_cast<const ValueDataDecimal &>(v);
        return x_ < d.x_;
    }
};

class ValueDataDateTime : public ValueData
{
    boost::posix_time::ptime x_;
public:
    ValueDataDateTime(const boost::posix_time::ptime &x =
            boost::posix_time::not_a_date_time)
        : x_(x) {}
    const boost::posix_time::ptime &get() const { return x_; }
    const string to_str() const
    {
        string t(boost::posix_time::to_iso_extended_string(x_));
#if 0
        size_t found = t.find('T');
        if (found != string::npos)
            t[found] = ' ';
#endif
        return t;
    }
    const string to_sql_str() const
    {
        return "TO_DATE('" +
            boost::posix_time::to_iso_extended_string(x_)
            + "', 'YYYY-MM-DD\"T\"HH24:MI:SS')";
    }
    bool eq(const ValueData &v) const
    {
        const ValueDataDateTime &d = dynamic_cast<const ValueDataDateTime &>(v);
        return x_ == d.x_;
    }
    bool lt(const ValueData &v) const
    {
        const ValueDataDateTime &d = dynamic_cast<const ValueDataDateTime &>(v);
        return x_ < d.x_;
    }
};

class ValueDataPKID : public ValueData
{
    PKIDValue x_;
public:
    ValueDataPKID(const PKIDValue &x = PKIDValue())
        : x_(x) {}
    const PKIDValue &get() const { return x_; }
    const string to_str() const
    {
        return to_sql_str();
    }
    const string to_sql_str() const
    {
        if (x_.is_temp())
            return "NULL";
        return boost::lexical_cast<string>(x_.as_long_long());
    }
    bool eq(const ValueData &v) const
    {
        const ValueDataPKID &d = dynamic_cast<const ValueDataPKID &>(v);
        return x_.eq(d.x_);
    }
    bool lt(const ValueData &v) const
    {
        const ValueDataPKID &d = dynamic_cast<const ValueDataPKID &>(v);
        return x_.lt(d.x_);
    }
};

PKIDValue::PKIDValue()
    : table_(NULL)
    , key_(pair<string, long long>(string(), 0))
    , pkid_(0)
{}

PKIDValue::PKIDValue(const ORMapper::TableMetaData &table,
        boost::shared_ptr<ORMapper::PKIDRecord> temp)
    : table_(&table)
    , key_(pair<string, long long>("-" + table.get_name(), temp->pkid_))
    , pkid_(0)
    , temp_(temp)
{}

PKIDValue::PKIDValue(const ORMapper::TableMetaData &table, long long pkid)
    : table_(&table)
    , key_(pair<string, long long>("+" + table.get_name(), pkid))
    , pkid_(pkid)
{}

const ORMapper::TableMetaData &PKIDValue::get_table() const
{
    if (!table_)
        throw PKIDInvalid();
    return *table_;
}

bool PKIDValue::eq(const PKIDValue &x) const
{
    return key_ == x.key_;
}

bool PKIDValue::lt(const PKIDValue &x) const
{
    return key_ < x.key_;
}

bool PKIDValue::is_temp() const
{
    return temp_.get() && temp_->is_temp_;
}

long long PKIDValue::as_long_long() const
{
    if (!table_)
        throw PKIDInvalid();
    if (!temp_.get())
        return pkid_;
    if (temp_->is_temp_)
        throw PKIDNotSynced(table_->get_name(), temp_->pkid_);
    return temp_->pkid_;
}

void PKIDValue::sync(long long pkid) const
{
    if (!table_)
        throw PKIDInvalid();
    if (!is_temp())
        throw PKIDAlreadySynced(table_->get_name(),
                as_long_long());
    temp_->pkid_ = pkid;
    temp_->is_temp_ = false;
}

Value::Value()
    : type_(Invalid)
{}

Value::Value(long long x)
    : data_(new ValueDataLongLong(x))
    , type_(LongLong)
{}

Value::Value(const char *x)
    : data_(new ValueDataString(x))
    , type_(String)
{}

Value::Value(const string &x)
    : data_(new ValueDataString(x))
    , type_(String)
{}

Value::Value(const decimal &x)
    : data_(new ValueDataDecimal(x))
    , type_(Decimal)
{}

Value::Value(const boost::posix_time::ptime &x)
    : data_(new ValueDataDateTime(x))
    , type_(DateTime)
{}

Value::Value(const PKIDValue &x)
    : data_(new ValueDataPKID(x))
    , type_(PKID)
{}

bool
Value::is_null() const
{
    return type_ == Invalid;
}

long long
Value::as_long_long() const
{
    if (type_ != LongLong) {
        try {
            return boost::lexical_cast<long long>(check_null().to_str());
        }
        catch (const boost::bad_lexical_cast &) {
            throw ValueBadCast(check_null().to_str(), "LongLong");
        }
    }
    const ValueDataLongLong &d =
        dynamic_cast<const ValueDataLongLong &>(check_null());
    return d.get();
}

const decimal
Value::as_decimal() const
{
    if (type_ != Decimal) {
        string s = check_null().to_str();
        try {
            return decimal(s);
        }
        catch (const decimal::exception &) {
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
                return decimal(o.str());
            }
            catch (const decimal::exception &) {
                throw ValueBadCast(s, "Decimal");
            }
        }
    }
    const ValueDataDecimal &d =
        dynamic_cast<const ValueDataDecimal &>(check_null());
    return d.get();
}

const boost::posix_time::ptime
Value::as_date_time() const
{
    if (type_ != DateTime) {
        string s = check_null().to_str();
        try {
            time_t t = boost::lexical_cast<time_t>(s);
            tm stm;
            localtime_r(&t, &stm);
            return boost::posix_time::ptime(
                    boost::gregorian::date(stm.tm_year + 1900, stm.tm_mon + 1, stm.tm_mday),
                    boost::posix_time::time_duration(stm.tm_hour, stm.tm_min, stm.tm_sec));
        }
        catch (const boost::bad_lexical_cast &) {
            string::size_type pos = s.find("T");
            if (pos != string::npos)
                s[pos] = ' ';
            try {
                return boost::posix_time::time_from_string(s);
            }
            catch (const boost::bad_lexical_cast &) {
                throw ValueBadCast(check_null().to_str(), "DateTime");
            }
        }
    }
    const ValueDataDateTime &d =
        dynamic_cast<const ValueDataDateTime &>(check_null());
    return d.get();
}

const PKIDValue
Value::as_pkid() const
{
    if (type_ != PKID)
        throw ValueBadCast(check_null().to_str(), "PKID");
    const ValueDataPKID &d =
        dynamic_cast<const ValueDataPKID &>(check_null());
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

bool
Value::eq(const Value &x) const
{
    if (is_null() && x.is_null())
        return true;
    if (is_null() || x.is_null())
        return false;
    try {
        return data_->eq(*x.data_);
    }
    catch (const std::bad_cast &) {
        if (get_type() == Decimal || x.get_type() == Decimal)
            return as_decimal() == x.as_decimal();
        if (get_type() == LongLong || x.get_type() == LongLong)
            return as_long_long() == x.as_long_long();
        if (get_type() == DateTime || x.get_type() == DateTime)
            return as_date_time() == x.as_date_time();
        return as_string() == x.as_string();
    }
}

bool
Value::lt(const Value &x) const
{
    if (is_null() && x.is_null())
        return false;
    if (is_null())
        return true;
    if (x.is_null())
        return false;
    return data_->lt(*x.data_);
}

const Value
Value::get_typed_value(int type) const
{
    if (is_null())
        return *this;
    switch (type) {
        case Value::LongLong:
            return as_long_long();
        case Value::Decimal:
            return as_decimal();
        case Value::DateTime:
            return as_date_time();
        case Value::String:
            return as_string();
        case Value::PKID:
            return as_pkid();
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
        { "Invalid", "LongLong", "String", "Decimal", "DateTime", "PKID" };
    if (type < 0 || type >= sizeof(type_names)/sizeof(const char *))
        return "Unknown";
    return type_names[type];
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et

