#include <time.h>
#include <sstream>
#include <iomanip>
#include <boost/lexical_cast.hpp>
#include <boost/thread/mutex.hpp>
#include <util/str_utils.hpp>
#include "Value.h"
#include "MetaData.h"
#include "Session.h"

using namespace std;
using namespace Yb::StrUtils;

#if defined(__WIN32__) || defined(_WIN32)
struct tm *localtime_r(const time_t *clock, struct tm *result)
{ 
    if (!clock || !result)
        return NULL;
    static boost::mutex m;
    boost::mutex::scoped_lock lock(m);
    memcpy(result, localtime(clock), sizeof(*result)); 
    return result; 
}
#endif

namespace Yb {

const DateTime mk_datetime(const string &s)
{
    string::size_type pos = s.find('T');
    if (pos == string::npos)
        return boost::posix_time::time_from_string(s);
    string t(s);
    t[pos] = ' ';
    return boost::posix_time::time_from_string(t);
}

class ValueData
{
public:
    virtual const string to_str() const = 0;
    virtual const string to_sql_str() const = 0;
    virtual bool eq(const ValueData &v) const = 0;
    virtual bool lt(const ValueData &v) const = 0;
    virtual ~ValueData() {}
};

template <typename T>
class ValueDataImpl: public ValueData
{
    T x_;
public:
    ValueDataImpl(T x = T()): x_(x) {}
    T get() const { return x_; }
    const string to_str() const { return to_string(x_); }
    const string to_sql_str() const { return to_str(); }
    bool eq(const ValueData &v) const {
        return x_ == dynamic_cast<const ValueDataImpl<T> &>(v).x_;
    }
    bool lt(const ValueData &v) const {
        return x_ < dynamic_cast<const ValueDataImpl<T> &>(v).x_;
    }
};

template <>
const string ValueDataImpl<string>::to_sql_str() const
{
    return quote(sql_string_escape(x_));
}

template <>
const string ValueDataImpl<DateTime>::to_sql_str() const
{
    string t(to_string(x_));
    size_t pos = t.find('T');
    if (pos != string::npos)
        t[pos] = ' ';
    return "'" + t + "'";
}

template <>
const string ValueDataImpl<PKIDValue>::to_sql_str() const
{
    if (x_.is_temp())
        return "NULL";
    return boost::lexical_cast<string>(x_.as_longint());
}

template <>
const string ValueDataImpl<PKIDValue>::to_str() const
{
    return to_sql_str();
}

PKIDValue::PKIDValue()
    : table_(NULL)
    , key_(pair<string, LongInt>(string(), 0))
    , pkid_(0)
{}

PKIDValue::PKIDValue(const TableMetaData &table,
        boost::shared_ptr<PKIDRecord> temp)
    : table_(&table)
    , key_(pair<string, LongInt>("-" + table.get_name(), temp->pkid_))
    , pkid_(0)
    , temp_(temp)
{}

PKIDValue::PKIDValue(const TableMetaData &table, LongInt pkid)
    : table_(&table)
    , key_(pair<string, LongInt>("+" + table.get_name(), pkid))
    , pkid_(pkid)
{}

const TableMetaData &PKIDValue::get_table() const
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

LongInt PKIDValue::as_longint() const
{
    if (!table_)
        throw PKIDInvalid();
    if (!temp_.get())
        return pkid_;
    if (temp_->is_temp_)
        throw PKIDNotSynced(table_->get_name(), temp_->pkid_);
    return temp_->pkid_;
}

void PKIDValue::sync(LongInt pkid) const
{
    if (!table_)
        throw PKIDInvalid();
    if (!is_temp())
        throw PKIDAlreadySynced(table_->get_name(), as_longint());
    temp_->pkid_ = pkid;
    temp_->is_temp_ = false;
}

Value::Value()
    : type_(INVALID)
{}

Value::Value(LongInt x)
    : data_(new ValueDataImpl<LongInt>(x))
    , type_(LONGINT)
{}

Value::Value(const char *x)
    : data_(new ValueDataImpl<string>(x))
    , type_(STRING)
{}

Value::Value(const string &x)
    : data_(new ValueDataImpl<string>(x))
    , type_(STRING)
{}

Value::Value(const Decimal &x)
    : data_(new ValueDataImpl<Decimal>(x))
    , type_(DECIMAL)
{}

Value::Value(const DateTime &x)
    : data_(new ValueDataImpl<DateTime>(x))
    , type_(DATETIME)
{}

Value::Value(const PKIDValue &x)
    : data_(new ValueDataImpl<PKIDValue>(x))
    , type_(PKID)
{}

bool
Value::is_null() const
{
    return type_ == INVALID;
}

LongInt
Value::as_longint() const
{
    if (type_ != LONGINT) {
        try {
            return boost::lexical_cast<LongInt>(check_null().to_str());
        }
        catch (const boost::bad_lexical_cast &) {
            throw ValueBadCast(check_null().to_str(), "LongLong");
        }
    }
    const ValueDataImpl<LongInt> &d =
        dynamic_cast<const ValueDataImpl<LongInt> &>(check_null());
    return d.get();
}

const Decimal
Value::as_decimal() const
{
    if (type_ != DECIMAL) {
        string s = check_null().to_str();
        try {
            return Decimal(s);
        }
        catch (const Decimal::exception &) {
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
                return Decimal(o.str());
            }
            catch (const Decimal::exception &) {
                throw ValueBadCast(s, "Decimal");
            }
        }
    }
    const ValueDataImpl<Decimal> &d =
        dynamic_cast<const ValueDataImpl<Decimal> &>(check_null());
    return d.get();
}

const DateTime
Value::as_date_time() const
{
    if (type_ != DATETIME) {
        string s = check_null().to_str();
        try {
            time_t t = boost::lexical_cast<time_t>(s);
            tm stm;
            localtime_r(&t, &stm);
            return DateTime(
                    boost::gregorian::date(
                        stm.tm_year + 1900, stm.tm_mon + 1, stm.tm_mday),
                    boost::posix_time::time_duration(
                        stm.tm_hour, stm.tm_min, stm.tm_sec));
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
    const ValueDataImpl<DateTime> &d =
        dynamic_cast<const ValueDataImpl<DateTime> &>(check_null());
    return d.get();
}

const PKIDValue
Value::as_pkid() const
{
    if (type_ != PKID)
        throw ValueBadCast(check_null().to_str(), "PKID");
    const ValueDataImpl<PKIDValue> &d =
        dynamic_cast<const ValueDataImpl<PKIDValue> &>(check_null());
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
        if (get_type() == DECIMAL || x.get_type() == DECIMAL)
            return as_decimal() == x.as_decimal();
        if (get_type() == LONGINT || x.get_type() == LONGINT)
            return as_longint() == x.as_longint();
        if (get_type() == DATETIME || x.get_type() == DATETIME)
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
        case Value::LONGINT:
            return as_longint();
        case Value::DECIMAL:
            return as_decimal();
        case Value::DATETIME:
            return as_date_time();
        case Value::STRING:
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
        { "Invalid", "LongInt", "String", "Decimal", "DateTime", "PKID" };
    if (type < 0 || type >= sizeof(type_names)/sizeof(const char *))
        return "Unknown";
    return type_names[type];
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
