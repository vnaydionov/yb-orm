#ifndef YB__ORM__FILTERS__INCLUDED
#define YB__ORM__FILTERS__INCLUDED

#include <vector>
#include <string>
#include <set>
#include <map>
#include <boost/shared_ptr.hpp>
#include "Value.h"

namespace Yb {

typedef std::vector<std::string> FieldList;
typedef std::set<std::string> FieldSet;
typedef std::map<std::string, int> ParamNums;

class StrList
{
    std::string str_list_;
    template <typename T>
    static const std::string container_to_str(const T &cont)
    {
        std::string r;
        typename T::const_iterator it = cont.begin(), end = cont.end();
        if (it != end)
            r = *it;
        for (++it; it != end; ++it)
            r += ", " + *it;
        return r;
    }
public:
    StrList()
    {}
    StrList(const FieldSet &fs)
        : str_list_(container_to_str<FieldSet>(fs))
    {}
    StrList(const FieldList &fl)
        : str_list_(container_to_str<FieldList>(fl))
    {}
    template <typename T>
    StrList(const T &fl)
	: str_list_(container_to_str<T>(fl))
    {}
    StrList(const std::string &s)
        : str_list_(s)
    {}
    StrList(const char *s)
        : str_list_(s)
    {}
    const std::string &get_str() const { return str_list_; }
};

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

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__FILTERS__INCLUDED
