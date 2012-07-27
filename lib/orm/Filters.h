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


class ExpressionBackend
{
public:
    virtual const String do_get_sql() const = 0;
    virtual const String do_collect_params_and_build_sql(Values &seq) const = 0;
    virtual ~ExpressionBackend();
};

typedef SharedPtr<ExpressionBackend>::Type ExprBEPtr;

class Expression
{
protected:
    ExprBEPtr backend_;
    String sql_;
public:
    Expression();
    Expression(const String &sql);
    Expression(SharedPtr<ExpressionBackend>::Type backend);
    const String get_sql() const;
    const String collect_params_and_build_sql(Values &seq) const;
    bool is_empty() const { return str_empty(sql_) && !shptr_get(backend_); }
};

const String sql_parentheses_as_needed(const String &s);
const String sql_prefix(const String &s, const String &prefix);
const String sql_alias(const String &s, const String &alias);

class ColumnExprBackend: public ExpressionBackend
{
    Expression expr_;
    String tbl_name_, col_name_, alias_;
public:
    ColumnExprBackend(const Expression &expr, const String &alias);
    ColumnExprBackend(const String &tbl_name, const String &col_name,
            const String &alias);
    const String do_get_sql() const;
    const String do_collect_params_and_build_sql(Values &seq) const;
    const String &alias() const { return alias_; }
};

class ColumnExpr: public Expression
{
public:
    ColumnExpr(const Expression &expr, const String &alias = _T(""));
    ColumnExpr(const String &tbl_name, const String &col_name,
            const String &alias = _T(""));
    const String &alias() const;
};

class ConstExprBackend: public ExpressionBackend
{
    Value value_;
public:
    ConstExprBackend(const Value &x);
    const String do_get_sql() const;
    const String do_collect_params_and_build_sql(Values &seq) const;
    const Value &const_value() const { return value_; }
};

class ConstExpr: public Expression
{
public:
    ConstExpr();
    ConstExpr(const Value &x);
    const Value &const_value() const;
};

class BinaryOpExprBackend: public ExpressionBackend
{
    Expression expr1_, expr2_;
    String op_;
public:
    BinaryOpExprBackend(const Expression &expr1,
            const String &op, const Expression &expr2);
    const String do_get_sql() const;
    const String do_collect_params_and_build_sql(Values &seq) const;
    const String &op() const { return op_; }
    const Expression &expr1() const { return expr1_; }
    const Expression &expr2() const { return expr2_; }
};

class BinaryOpExpr: public Expression
{
public:
    BinaryOpExpr(const Expression &expr1,
            const String &op, const Expression &expr2);
    const String &op() const;
    const Expression &expr1() const;
    const Expression &expr2() const;
};

const Expression filter_eq(const String &name, const Value &value);
const Expression filter_ne(const String &name, const Value &value);
const Expression filter_lt(const String &name, const Value &value);
const Expression filter_gt(const String &name, const Value &value);
const Expression filter_le(const String &name, const Value &value);
const Expression filter_ge(const String &name, const Value &value);
const Expression operator && (const Expression &a, const Expression &b);
const Expression operator || (const Expression &a, const Expression &b);

class FilterBackendByPK: public ExpressionBackend
{
    Expression expr_;
    Key key_;
    static const Expression build_expr(const Key &key);
public:
    FilterBackendByPK(const Key &key);
    const String do_get_sql() const;
    const String do_collect_params_and_build_sql(Values &seq) const;
    const Key &key() const { return key_; }
};

class KeyFilter: public Expression
{
public:
    KeyFilter(const Key &key);
    const Key &key() const;
};

typedef Expression Filter;

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
