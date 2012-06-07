#ifndef YB__ORM__FILTERS__INCLUDED
#define YB__ORM__FILTERS__INCLUDED

#include <vector>
#include <string>
#include <set>
#include <map>
#include <util/Utility.h>
#include <util/Exception.h>
#include <orm/Value.h>

namespace Yb {

typedef std::map<String, int> ParamNums;

class StrList
{
    String str_list_;
    template <typename T>
    static const String container_to_str(const T &cont)
    {
        String r;
        typename T::const_iterator it = cont.begin(), end = cont.end();
        if (it != end)
            r = *it;
        for (++it; it != end; ++it) {
            r += _T(", ");
            r += *it;
        }
        return r;
    }
public:
    StrList()
    {}
    StrList(const StringSet &fs)
        : str_list_(container_to_str<StringSet>(fs))
    {}
    StrList(const Strings &fl)
        : str_list_(container_to_str<Strings>(fl))
    {}
    template <typename T>
    StrList(const T &fl)
        : str_list_(container_to_str<T>(fl))
    {}
    StrList(const String &s)
        : str_list_(s)
    {}
    StrList(const wchar_t *s)
        : str_list_(WIDEN(fast_narrow(s).c_str()))
    {}
    StrList(const char *s)
        : str_list_(WIDEN(s))
    {}
    const String &get_str() const { return str_list_; }
};

class FilterBackend
{
public:
    virtual const String do_get_sql() const = 0;
    virtual const String do_collect_params_and_build_sql(
            Values &seq) const = 0;
    virtual ~FilterBackend();
};

class Filter
{
    SharedPtr<FilterBackend>::Type backend_;
    String sql_;
public:
    Filter();
    Filter(const String &sql);
    Filter(FilterBackend *backend);
    bool is_empty() const;
    const String get_sql() const;
    const String collect_params_and_build_sql(
            Values &seq) const;
    const FilterBackend *get_backend() const { return shptr_get(backend_); }
};

class FilterBackendCmp : public FilterBackend
{
    String name_, op_;
    Value value_;
public:
    FilterBackendCmp(const String &name, const String &op, const Value &value);
    const String do_get_sql() const;
    const String do_collect_params_and_build_sql(
            Values &seq) const;
    const String &get_name() const;
    const String &get_op() const;
    const Value &get_value() const;
};

inline const Filter filter_eq(const String &name, const Value &value)
{
    return Filter(new FilterBackendCmp(name, _T("="), value));
}
inline const Filter filter_ne(const String &name, const Value &value)
{
    return Filter(new FilterBackendCmp(name, _T("<>"), value));
}
inline const Filter filter_lt(const String &name, const Value &value)
{
    return Filter(new FilterBackendCmp(name, _T("<"), value));
}
inline const Filter filter_gt(const String &name, const Value &value)
{
    return Filter(new FilterBackendCmp(name, _T(">"), value));
}
inline const Filter filter_le(const String &name, const Value &value)
{
    return Filter(new FilterBackendCmp(name, _T("<="), value));
}
inline const Filter filter_ge(const String &name, const Value &value)
{
    return Filter(new FilterBackendCmp(name, _T(">="), value));
}

class FilterBackendAnd : public FilterBackend
{
    Filter f1_;
    Filter f2_;
public:
    FilterBackendAnd(const Filter &f1, const Filter &f2);
    const String do_get_sql() const;
    const String do_collect_params_and_build_sql(
            Values &seq) const;
};

class FilterBackendOr : public FilterBackend
{
    Filter f1_;
    Filter f2_;
public:
    FilterBackendOr(const Filter &f1, const Filter &f2);
    const String do_get_sql() const;
    const String do_collect_params_and_build_sql(
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

class FilterBackendByPK : public FilterBackend
{
    Key key_;
public:
    FilterBackendByPK(const Key &key) : key_(key) {}
    const String do_get_sql() const;
    const String do_collect_params_and_build_sql(
            Values &seq) const;
    const Key &get_key() const { return key_; }
};

class KeyFilter : public Filter
{
public:
    KeyFilter(const Key &key)
        : Filter(new FilterBackendByPK(key))
    {}
};

class ORMError : public BaseError
{
public:
    ORMError(const String &msg);
};

class ObjectNotFoundByKey : public ORMError
{
public:
    ObjectNotFoundByKey(const String &msg);
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__FILTERS__INCLUDED
