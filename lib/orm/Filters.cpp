#include <orm/Filters.h>

using namespace std;

namespace Yb {

FilterBackend::~FilterBackend()
{}

Filter::Filter()
    : sql_(_T("1=1"))
{}

Filter::Filter(const String &sql)
    : sql_(sql)
{}

Filter::Filter(FilterBackend *backend)
    : backend_(backend)
{}

bool
Filter::is_empty() const
{
    return !shptr_get(backend_) && !sql_.compare(_T("1=1"));
}

const String
Filter::get_sql() const
{
    if (shptr_get(backend_))
        return backend_->do_get_sql();
    return sql_;
}

const String
Filter::collect_params_and_build_sql(Values &seq) const
{
    if (shptr_get(backend_))
        return backend_->do_collect_params_and_build_sql(seq);
    return sql_;
}

FilterBackendCmp::FilterBackendCmp(const String &name, const String &op, const Value &value)
    : name_(name)
    , op_(op)
    , value_(value)
{}

const String
FilterBackendCmp::do_get_sql() const
{
    return name_ + _T(" ") + op_ + _T(" ") + value_.sql_str();
}

const String
FilterBackendCmp::do_collect_params_and_build_sql(Values &seq) const
{
    seq.push_back(value_);
    return name_ + _T(" ") + op_ + _T(" ?");
}

const String &
FilterBackendCmp::get_name() const
{
    return name_;
}

const String &
FilterBackendCmp::get_op() const
{
    return op_;
}

const Value &
FilterBackendCmp::get_value() const
{
    return value_;
}

FilterBackendAnd::FilterBackendAnd(const Filter &f1, const Filter &f2)
    : f1_(f1)
    , f2_(f2)
{}

const String
FilterBackendAnd::do_get_sql() const
{
    return _T("(") + f1_.get_sql() + _T(") AND (") + f2_.get_sql() + _T(")");
}

const String
FilterBackendAnd::do_collect_params_and_build_sql(Values &seq) const
{
    String p1 = f1_.collect_params_and_build_sql(seq);
    String p2 = f2_.collect_params_and_build_sql(seq);
    return _T("(") + p1 + _T(") AND (") + p2 + _T(")");
}

FilterBackendOr::FilterBackendOr(const Filter &f1, const Filter &f2)
    : f1_(f1)
    , f2_(f2)
{}

const String
FilterBackendOr::do_get_sql() const
{
    return _T("(") + f1_.get_sql() + _T(") OR (") + f2_.get_sql() + _T(")");
}

const String
FilterBackendOr::do_collect_params_and_build_sql(Values &seq) const
{
    String p1 = f1_.collect_params_and_build_sql(seq);
    String p2 = f2_.collect_params_and_build_sql(seq);
    return _T("(") + p1 + _T(") OR (") + p2 + _T(")");
}

const String
FilterBackendByPK::do_get_sql() const
{
    std::ostringstream sql;
    sql << "1=1";
    ValueMap::const_iterator i = key_.second.begin(),
        iend = key_.second.end();
    for (; i != iend; ++i)
        sql << " AND " << NARROW(i->first) << " = " << NARROW(i->second.sql_str());
    return WIDEN(sql.str());
}

const String
FilterBackendByPK::do_collect_params_and_build_sql(Values &seq) const
{
    std::ostringstream sql;
    sql << "1=1";
    ValueMap::const_iterator i = key_.second.begin(),
        iend = key_.second.end();
    for (; i != iend; ++i) {
        sql << " AND " << NARROW(i->first) << " = ?";
        seq.push_back(i->second);
    }
    return WIDEN(sql.str());
}

ORMError::ORMError(const String &msg)
    : BaseError(msg)
{}

ObjectNotFoundByKey::ObjectNotFoundByKey(const String &msg)
    : ORMError(msg)
{}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
