
#include "Filters.h"

using namespace std;

namespace Yb {
namespace SQL {

FilterBackend::~FilterBackend()
{}

Filter::Filter()
    : sql_("1=1")
{}

Filter::Filter(const string &sql)
    : sql_(sql)
{}

Filter::Filter(FilterBackend *backend)
    : backend_(backend)
{}

const string
Filter::get_sql() const
{
    if (backend_.get())
        return backend_->do_get_sql();
    return sql_;
}

const string
Filter::collect_params_and_build_sql(Values &seq) const
{
    if (backend_.get())
        return backend_->do_collect_params_and_build_sql(seq);
    return sql_;
}

FilterBackendEq::FilterBackendEq(const string &name, const Value &value)
    : name_(name)
    , value_(value)
{}

const string
FilterBackendEq::do_get_sql() const
{
    return name_ + " = " + value_.sql_str();
}

const string
FilterBackendEq::do_collect_params_and_build_sql(Values &seq) const
{
    seq.push_back(value_);
    return name_ + " = ?";
}

const string &
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

const string
FilterBackendAnd::do_get_sql() const
{
    return "(" + f1_.get_sql() + ") AND (" + f2_.get_sql() + ")";
}

const string
FilterBackendAnd::do_collect_params_and_build_sql(Values &seq) const
{
    string p1 = f1_.collect_params_and_build_sql(seq);
    string p2 = f2_.collect_params_and_build_sql(seq);
    return "(" + p1 + ") AND (" + p2 + ")";
}

FilterBackendOr::FilterBackendOr(const Filter &f1, const Filter &f2)
    : f1_(f1)
    , f2_(f2)
{}

const string
FilterBackendOr::do_get_sql() const
{
    return "(" + f1_.get_sql() + ") OR (" + f2_.get_sql() + ")";
}

const string
FilterBackendOr::do_collect_params_and_build_sql(Values &seq) const
{
    string p1 = f1_.collect_params_and_build_sql(seq);
    string p2 = f2_.collect_params_and_build_sql(seq);
    return "(" + p1 + ") OR (" + p2 + ")";
}

} // namespace SQL
} // namespace Yb

