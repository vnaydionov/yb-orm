#include "Filters.h"

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

const String
Filter::get_sql() const
{
    if (backend_.get())
        return backend_->do_get_sql();
    return sql_;
}

const String
Filter::collect_params_and_build_sql(Values &seq) const
{
    if (backend_.get())
        return backend_->do_collect_params_and_build_sql(seq);
    return sql_;
}

FilterBackendEq::FilterBackendEq(const String &name, const Value &value)
    : name_(name)
    , value_(value)
{}

const String
FilterBackendEq::do_get_sql() const
{
    return name_ + _T(" = ") + value_.sql_str();
}

const String
FilterBackendEq::do_collect_params_and_build_sql(Values &seq) const
{
    seq.push_back(value_);
    return name_ + _T(" = ?");
}

const String &
FilterBackendEq::get_name() const
{
    return name_;
}

const Value &
FilterBackendEq::get_value() const
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
    OStringStream sql;
    sql << _T("1=1");
    ValueMap::const_iterator i = key_.second.begin(),
        iend = key_.second.end();
    for (; i != iend; ++i)
        sql << _T(" AND ") << i->first << _T(" = ") << i->second.sql_str();
    return sql.str();
}

const String
FilterBackendByPK::do_collect_params_and_build_sql(Values &seq) const
{
    OStringStream sql;
    sql << _T("1=1");
    ValueMap::const_iterator i = key_.second.begin(),
        iend = key_.second.end();
    for (; i != iend; ++i) {
        sql << _T(" AND ") << i->first << _T(" = ?");
        seq.push_back(i->second);
    }
    return sql.str();
}

ORMError::ORMError(const String &msg)
    : BaseError(msg)
{}

ObjectNotFoundByKey::ObjectNotFoundByKey(const String &msg)
    : ORMError(msg)
{}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
