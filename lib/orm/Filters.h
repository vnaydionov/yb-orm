
#ifndef YB__ORM__SQL_FILTERS__INCLUDED
#define YB__ORM__SQL_FILTERS__INCLUDED

#include <string>
#include <boost/shared_ptr.hpp>
#include "Value.h"

namespace Yb {
namespace SQL {

class FilterBackend
{
public:
    virtual const std::string do_get_sql() const = 0;
    virtual const std::string do_collect_params_and_build_sql(
            Values &seq) const = 0;
    virtual ~FilterBackend();
};

class Filter
{
    boost::shared_ptr<FilterBackend> backend_;
    std::string sql_;
public:
    Filter();
    Filter(const std::string &sql);
    Filter(FilterBackend *backend);
    const std::string get_sql() const;
    const std::string collect_params_and_build_sql(
            Values &seq) const;
    const FilterBackend *get_backend() const { return backend_.get(); }
};

class FilterBackendEq : public FilterBackend
{
    std::string name_;
    Value value_;
public:
    FilterBackendEq(const std::string &name, const Value &value);
    const std::string do_get_sql() const;
    const std::string do_collect_params_and_build_sql(
            Values &seq) const;
    const std::string &get_name() const;
    const Value &get_value() const;
};

inline const Filter filter_eq(const std::string &name, const Value &value)
{
    return Filter(new FilterBackendEq(name, value));
}

class FilterBackendAnd : public FilterBackend
{
    Filter f1_;
    Filter f2_;
public:
    FilterBackendAnd(const Filter &f1, const Filter &f2);
    const std::string do_get_sql() const;
    const std::string do_collect_params_and_build_sql(
            Values &seq) const;
};

class FilterBackendOr : public FilterBackend
{
    Filter f1_;
    Filter f2_;
public:
    FilterBackendOr(const Filter &f1, const Filter &f2);
    const std::string do_get_sql() const;
    const std::string do_collect_params_and_build_sql(
            Values &seq) const;
};

inline const Filter operator && (const Filter &a, const Filter &b)
{
    return Filter(new FilterBackendAnd(a, b));
}

inline const Filter operator || (const Filter &a, const Filter &b)
{
    return Filter(new FilterBackendOr(a, b));
}

} // namespace SQL
} // namespace Yb

// vim:ts=4:sts=4:sw=4:et

#endif // YB__ORM__SQL_FILTERS__INCLUDED

