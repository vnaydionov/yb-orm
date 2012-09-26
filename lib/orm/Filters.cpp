#include <orm/Filters.h>

using namespace std;

namespace Yb {

ExpressionBackend::~ExpressionBackend() {}

Expression::Expression() {}

Expression::Expression(const String &sql)
    : sql_(sql)
{}

Expression::Expression(SharedPtr<ExpressionBackend>::Type backend)
    : backend_(backend)
{}

const String
Expression::get_sql() const
{
    if (shptr_get(backend_))
        return backend_->do_get_sql();
    return sql_;
}

const String
Expression::collect_params_and_build_sql(Values &seq) const
{
    if (shptr_get(backend_))
        return backend_->do_collect_params_and_build_sql(seq);
    return sql_;
}

bool is_number_or_object_name(const String &s) {
    for (int i = 0; i < str_length(s); ++i) {
        int c = char_code(s[i]);
        if (!((c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') ||
                c == '_' || c == '#' || c == '$' || c == '.' ||
                c == ':'))
            return false;
    }
    return true;
}

bool is_string_constant(const String &s) {
    if (str_length(s) < 2 || char_code(s[0]) != '\'' ||
            char_code(s[(int)(str_length(s) - 1)]) != '\'')
        return false;
    bool seen_quot = false;
    for (int i = 1; i < str_length(s) - 1; ++i) {
        int c = char_code(s[i]);
        if (c == '\'')
            seen_quot = !seen_quot;
        else if (seen_quot)
            return false;
    }
    return !seen_quot;
}

bool is_in_parentheses(const String &s) {
    if (str_length(s) < 2 || char_code(s[0]) != '(' ||
            char_code(s[(int)(str_length(s) - 1)]) != ')')
        return false;
    int level = 0;
    bool seen_quot = false;
    for (int i = 1; i < str_length(s) - 1; ++i) {
        int c = char_code(s[i]);
        if (c == '\'')
            seen_quot = !seen_quot;
        else if (!seen_quot) {
            if (c == '(')
                ++level;
            else if (c == ')') {
                --level;
                if (level < 0)
                    return false;
            }
        }
    }
    return !seen_quot && level == 0;
}

const String sql_parentheses_as_needed(const String &s)
{
    if (is_number_or_object_name(s) || is_string_constant(s) || is_in_parentheses(s)
            || s == _T("?"))
        return s;
    return _T("(") + s + _T(")");
}

const String sql_prefix(const String &s, const String &prefix)
{
    if (str_empty(prefix))
        return s;
    return prefix + _T(".") + s;
}

const String sql_alias(const String &s, const String &alias)
{
    if (str_empty(alias))
        return s;
    return s + _T(" ") + alias;
}

ColumnExprBackend::ColumnExprBackend(const Expression &expr, const String &alias)
    : expr_(expr)
    , alias_(alias)
{}

ColumnExprBackend::ColumnExprBackend(const String &tbl_name, const String &col_name,
        const String &alias)
    : tbl_name_(tbl_name)
    , col_name_(col_name)
    , alias_(alias)
{}

const String
ColumnExprBackend::do_get_sql() const
{
    String r;
    if (!str_empty(col_name_))
        r = sql_prefix(col_name_, tbl_name_);
    else
        r += sql_parentheses_as_needed(expr_.get_sql());
    return sql_alias(r, alias_);
}

const String
ColumnExprBackend::do_collect_params_and_build_sql(Values &seq) const
{
    String r;
    if (!str_empty(col_name_))
        r = sql_prefix(col_name_, tbl_name_);
    else
        r += sql_parentheses_as_needed(
                expr_.collect_params_and_build_sql(seq));
    return sql_alias(r, alias_);
}

ColumnExpr::ColumnExpr(const Expression &expr, const String &alias)
    : Expression(ExprBEPtr(new ColumnExprBackend(expr, alias)))
{}

ColumnExpr::ColumnExpr(const String &tbl_name, const String &col_name,
        const String &alias)
    : Expression(ExprBEPtr(new ColumnExprBackend(tbl_name, col_name, alias)))
{}

const String &
ColumnExpr::alias() const {
    return dynamic_cast<ColumnExprBackend *>(shptr_get(backend_))->alias();
}

ConstExprBackend::ConstExprBackend(const Value &x): value_(x) {}

const String
ConstExprBackend::do_get_sql() const { return value_.sql_str(); }

const String
ConstExprBackend::do_collect_params_and_build_sql(Values &seq) const
{
    seq.push_back(value_);
    return _T("?");
}

ConstExpr::ConstExpr()
    : Expression(ExprBEPtr(new ConstExprBackend(Value())))
{}

ConstExpr::ConstExpr(const Value &x)
    : Expression(ExprBEPtr(new ConstExprBackend(x)))
{}

const Value &
ConstExpr::const_value() const {
    return dynamic_cast<ConstExprBackend *>(shptr_get(backend_))->const_value();
}

BinaryOpExprBackend::BinaryOpExprBackend(const Expression &expr1,
        const String &op, const Expression &expr2)
    : expr1_(expr1)
    , expr2_(expr2)
    , op_(op)
{}

const String
BinaryOpExprBackend::do_get_sql() const
{
    return sql_parentheses_as_needed(expr1_.get_sql())
        + _T(" ") + op_ + _T(" ")
        + sql_parentheses_as_needed(expr2_.get_sql());
}

const String
BinaryOpExprBackend::do_collect_params_and_build_sql(Values &seq) const
{
    String sql = sql_parentheses_as_needed(
            expr1_.collect_params_and_build_sql(seq));
    sql += _T(" ") + op_ + _T(" ");
    sql += sql_parentheses_as_needed(
            expr2_.collect_params_and_build_sql(seq));
    return sql;
}

BinaryOpExpr::BinaryOpExpr(const Expression &expr1,
        const String &op, const Expression &expr2)
    : Expression(ExprBEPtr(new BinaryOpExprBackend(expr1, op, expr2)))
{}

const String &
BinaryOpExpr::op() const {
    return dynamic_cast<BinaryOpExprBackend *>(shptr_get(backend_))->op();
}

const Expression &
BinaryOpExpr::expr1() const {
    return dynamic_cast<BinaryOpExprBackend *>(shptr_get(backend_))->expr1();
}

const Expression &
BinaryOpExpr::expr2() const {
    return dynamic_cast<BinaryOpExprBackend *>(shptr_get(backend_))->expr2();
}

const String
ExpressionListBackend::do_get_sql() const
{
    String sql;
    for (size_t i = 0; i < items_.size(); ++i) {
        if (!sql.empty())
            sql += _T(", ");
        sql += sql_parentheses_as_needed(items_[i].get_sql());
    }
    return sql;
}

const String
ExpressionListBackend::do_collect_params_and_build_sql(Values &seq) const
{
    String sql;
    for (size_t i = 0; i < items_.size(); ++i) {
        if (!sql.empty())
            sql += _T(", ");
        sql += sql_parentheses_as_needed(items_[i].collect_params_and_build_sql(seq));
    }
    return sql;
}

ExpressionList::ExpressionList()
    : Expression(ExprBEPtr(new ExpressionListBackend))
{}

ExpressionList::ExpressionList(const Expression &expr)
    : Expression(ExprBEPtr(new ExpressionListBackend))
{
    append(expr);
}

void
ExpressionList::append(const Expression &expr) {
    dynamic_cast<ExpressionListBackend *>(shptr_get(backend_))->append(expr);
}

int
ExpressionList::size() const {
    return dynamic_cast<ExpressionListBackend *>(shptr_get(backend_))->size();
}

const Expression &
ExpressionList::item(int n) const {
    return dynamic_cast<ExpressionListBackend *>(shptr_get(backend_))->item(n);
}

const Expression
filter_eq(const String &name, const Value &value)
{
    return Expression(ExprBEPtr(new BinaryOpExprBackend(ColumnExpr(_T(""), name),
                _T("="), ConstExpr(value))));
}

const Expression
filter_ne(const String &name, const Value &value)
{
    return Expression(ExprBEPtr(new BinaryOpExprBackend(ColumnExpr(_T(""), name),
                _T("<>"), ConstExpr(value))));
}

const Expression
filter_lt(const String &name, const Value &value)
{
    return Expression(ExprBEPtr(new BinaryOpExprBackend(ColumnExpr(_T(""), name),
                _T("<"), ConstExpr(value))));
}

const Expression
filter_gt(const String &name, const Value &value)
{
    return Expression(ExprBEPtr(new BinaryOpExprBackend(ColumnExpr(_T(""), name),
                _T(">"), ConstExpr(value))));
}

const Expression
filter_le(const String &name, const Value &value)
{
    return Expression(ExprBEPtr(new BinaryOpExprBackend(ColumnExpr(_T(""), name),
                _T("<="), ConstExpr(value))));
}

const Expression
filter_ge(const String &name, const Value &value)
{
    return Expression(ExprBEPtr(new BinaryOpExprBackend(ColumnExpr(_T(""), name),
                _T(">="), ConstExpr(value))));
}

const Expression
operator && (const Expression &a, const Expression &b)
{
    if (a.is_empty())
        return b;
    if (b.is_empty())
        return a;
    return Expression(ExprBEPtr(new BinaryOpExprBackend(a, _T("AND"), b)));
}

const Expression
operator || (const Expression &a, const Expression &b)
{
    if (a.is_empty())
        return b;
    if (b.is_empty())
        return a;
    return Expression(ExprBEPtr(new BinaryOpExprBackend(a, _T("OR"), b)));
}

const Expression
FilterBackendByPK::build_expr(const Key &key)
{
    Expression expr;
    ValueMap::const_iterator i = key.second.begin(),
        iend = key.second.end();
    for (; i != iend; ++i)
        expr = expr && BinaryOpExpr(
                ColumnExpr(key.first, i->first),
                _T("="), ConstExpr(i->second));
    return expr;
}

FilterBackendByPK::FilterBackendByPK(const Key &key)
    : expr_(build_expr(key))
    , key_(key)
{}

const String
FilterBackendByPK::do_get_sql() const { return expr_.get_sql(); }

const String
FilterBackendByPK::do_collect_params_and_build_sql(Values &seq) const
{
    return expr_.collect_params_and_build_sql(seq);
}

KeyFilter::KeyFilter(const Key &key)
    : Expression(ExprBEPtr(new FilterBackendByPK(key)))
{}

const Key &
KeyFilter::key() const
{
    return dynamic_cast<FilterBackendByPK *>(shptr_get(backend_))->key();
}

ORMError::ORMError(const String &msg)
    : BaseError(msg)
{}

ObjectNotFoundByKey::ObjectNotFoundByKey(const String &msg)
    : ORMError(msg)
{}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
