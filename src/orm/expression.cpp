// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#define YBORM_SOURCE

#include "orm/expression.h"
#include "orm/schema.h"
#include "orm/sql_driver.h"
#include "orm/data_object.h"

using namespace std;

namespace Yb {

ExpressionBackend::~ExpressionBackend() {}

Expression::Expression() {}

Expression::Expression(const String &sql)
    : sql_(sql)
{}

Expression::Expression(const Column &col)
    : backend_(ExprBEPtr(new ColumnExprBackend(col.table().name(), col.name(),
        String())))
{}

Expression::Expression(ExprBEPtr backend)
    : backend_(backend)
{}

YBORM_DECL const String
Expression::generate_sql(Values *params, int *count) const
{
    if (backend_.get())
        return backend_->generate_sql(params, count);
    return sql_;
}

YBORM_DECL bool
is_number_or_object_name(const String &s) {
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

YBORM_DECL bool
is_string_constant(const String &s) {
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

YBORM_DECL bool
is_in_parentheses(const String &s) {
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

YBORM_DECL const String
sql_parentheses_as_needed(const String &s)
{
    if (is_number_or_object_name(s) || is_string_constant(s) || is_in_parentheses(s)
            || s == _T("?"))
        return s;
    return _T("(") + s + _T(")");
}

YBORM_DECL const String
sql_prefix(const String &s, const String &prefix)
{
    if (str_empty(prefix))
        return s;
    return prefix + _T(".") + s;
}

YBORM_DECL const String
sql_alias(const String &s, const String &alias)
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
ColumnExprBackend::generate_sql(Values *params, int *count) const
{
    String r;
    if (!str_empty(col_name_))
        r = sql_prefix(col_name_, tbl_name_);
    else
        r += sql_parentheses_as_needed(
                expr_.generate_sql(params, count));
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
    return dynamic_cast<ColumnExprBackend *>(backend_.get())->alias();
}

ConstExprBackend::ConstExprBackend(const Value &x): value_(x) {}

const String
ConstExprBackend::generate_sql(Values *params, int *count) const
{
    if (params) {
        params->push_back(value_);
        if (!count)
            return _T("?");
        int n = *count;
        ++*count;
        return _T(":") + to_string(n);
    }
    return value_.sql_str();
}

ConstExpr::ConstExpr()
    : Expression(ExprBEPtr(new ConstExprBackend(Value())))
{}

ConstExpr::ConstExpr(const Value &x)
    : Expression(ExprBEPtr(new ConstExprBackend(x)))
{}

const Value &
ConstExpr::const_value() const {
    return dynamic_cast<ConstExprBackend *>(backend_.get())->const_value();
}

UnaryOpExprBackend::UnaryOpExprBackend(
        bool prefix, const String &op, const Expression &expr)
    : prefix_(prefix)
    , op_(op)
    , expr_(expr)
{}

const String
UnaryOpExprBackend::generate_sql(Values *params, int *count) const
{
    if (prefix_)
        return op_ + _T(" ") + sql_parentheses_as_needed(
                expr_.generate_sql(params, count));
    return sql_parentheses_as_needed(
            expr_.generate_sql(params, count)) + _T(" ") + op_;
}

UnaryOpExpr::UnaryOpExpr(bool prefix, const String &op, const Expression &expr)
    : Expression(ExprBEPtr(new UnaryOpExprBackend(prefix, op, expr)))
{}

bool
UnaryOpExpr::prefix() const {
    return dynamic_cast<UnaryOpExprBackend *>(backend_.get())->prefix();
}

const String &
UnaryOpExpr::op() const {
    return dynamic_cast<UnaryOpExprBackend *>(backend_.get())->op();
}

const Expression &
UnaryOpExpr::expr() const {
    return dynamic_cast<UnaryOpExprBackend *>(backend_.get())->expr();
}

BinaryOpExprBackend::BinaryOpExprBackend(const Expression &expr1,
        const String &op, const Expression &expr2)
    : expr1_(expr1)
    , expr2_(expr2)
    , op_(op)
{}

const String
BinaryOpExprBackend::generate_sql(Values *params, int *count) const
{
    String sql1 = sql_parentheses_as_needed(
            expr1_.generate_sql(params, count));
    String sql2_nosubst = sql_parentheses_as_needed(
            expr2_.generate_sql(NULL));
    if (sql2_nosubst == _T("NULL")) {
        if (op_ == _T("="))
            return sql1 + _T(" IS NULL");
        if (op_ == _T("<>"))
            return sql1 + _T(" IS NOT NULL");
    }
    String sql2 = sql_parentheses_as_needed(
            expr2_.generate_sql(params, count));
    return sql1 + _T(" ") + op_ + _T(" ") + sql2;
}

BinaryOpExpr::BinaryOpExpr(const Expression &expr1,
        const String &op, const Expression &expr2)
    : Expression(ExprBEPtr(new BinaryOpExprBackend(expr1, op, expr2)))
{}

const String &
BinaryOpExpr::op() const {
    return dynamic_cast<BinaryOpExprBackend *>(backend_.get())->op();
}

const Expression &
BinaryOpExpr::expr1() const {
    return dynamic_cast<BinaryOpExprBackend *>(backend_.get())->expr1();
}

const Expression &
BinaryOpExpr::expr2() const {
    return dynamic_cast<BinaryOpExprBackend *>(backend_.get())->expr2();
}

const String
JoinExprBackend::generate_sql(Values *params, int *count) const
{
    String sql = sql_parentheses_as_needed(
            expr1_.generate_sql(params, count));
    sql += _T(" JOIN ");
    sql += sql_parentheses_as_needed(
            expr2_.generate_sql(params, count));
    sql += _T(" ON ");
    sql += sql_parentheses_as_needed(
            cond_.generate_sql(params, count));
    return sql;
}

JoinExpr::JoinExpr(const Expression &expr1,
        const Expression &expr2, const Expression &cond)
    : Expression(ExprBEPtr(new JoinExprBackend(expr1, expr2, cond)))
{}

const Expression &
JoinExpr::expr1() const {
    return dynamic_cast<JoinExprBackend *>(backend_.get())->expr1();
}

const Expression &
JoinExpr::expr2() const {
    return dynamic_cast<JoinExprBackend *>(backend_.get())->expr2();
}

const Expression &
JoinExpr::cond() const {
    return dynamic_cast<JoinExprBackend *>(backend_.get())->cond();
}

const String
ExpressionListBackend::generate_sql(Values *params, int *count) const
{
    String sql;
    for (size_t i = 0; i < items_.size(); ++i) {
        if (!str_empty(sql))
            sql += _T(", ");
        sql += sql_parentheses_as_needed(
                items_[i].generate_sql(params, count));
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

ExpressionList::ExpressionList(const Expression &expr1, const Expression &expr2)
    : Expression(ExprBEPtr(new ExpressionListBackend))
{
    append(expr1);
    append(expr2);
}

ExpressionList::ExpressionList(const Expression &expr1, const Expression &expr2,
        const Expression &expr3)
    : Expression(ExprBEPtr(new ExpressionListBackend))
{
    append(expr1);
    append(expr2);
    append(expr3);
}

void
ExpressionList::append(const Expression &expr) {
    dynamic_cast<ExpressionListBackend *>(backend_.get())->append(expr);
}

int
ExpressionList::size() const {
    return dynamic_cast<ExpressionListBackend *>(backend_.get())->size();
}

const Expression &
ExpressionList::item(int n) const {
    return dynamic_cast<ExpressionListBackend *>(backend_.get())->item(n);
}

const String
SelectExprBackend::generate_sql(Values *params, int *count) const
{
    String sql = _T("SELECT ");
    if (distinct_flag_)
        sql += _T("DISTINCT ");
    sql += select_expr_.generate_sql(params, count);
    if (!from_expr_.is_empty())
        sql += _T(" FROM ")
            + from_expr_.generate_sql(params, count);
    if (!where_expr_.is_empty())
        sql += _T(" WHERE ")
            + where_expr_.generate_sql(params, count);
    if (!group_by_expr_.is_empty())
        sql += _T(" GROUP BY ")
            + group_by_expr_.generate_sql(params, count);
    if (!having_expr_.is_empty()) {
        if (group_by_expr_.is_empty())
            throw BadSQLOperation(
                    _T("Trying to use HAVING without GROUP BY clause"));
        sql += _T(" HAVING ")
            + having_expr_.generate_sql(params, count);
    }
    if (!order_by_expr_.is_empty())
        sql += _T(" ORDER BY ")
            + order_by_expr_.generate_sql(params, count);
    if (!str_empty(lock_mode_))
        sql += _T(" FOR ") + lock_mode_;
    return sql;
}

SelectExpr::SelectExpr(const Expression &select_expr)
    : Expression(ExprBEPtr(new SelectExprBackend(select_expr)))
{}

SelectExpr &
SelectExpr::from_(const Expression &from_expr) {
    dynamic_cast<SelectExprBackend *>(backend_.get())->from_(from_expr);
    return *this;
}

SelectExpr &
SelectExpr::where_(const Expression &where_expr) {
    dynamic_cast<SelectExprBackend *>(backend_.get())->where_(where_expr);
    return *this;
}

SelectExpr &
SelectExpr::group_by_(const Expression &group_by_expr) {
    dynamic_cast<SelectExprBackend *>(backend_.get())->group_by_(group_by_expr);
    return *this;
}

SelectExpr &
SelectExpr::having_(const Expression &having_expr) {
    dynamic_cast<SelectExprBackend *>(backend_.get())->having_(having_expr);
    return *this;
}

SelectExpr &
SelectExpr::order_by_(const Expression &order_by_expr) {
    dynamic_cast<SelectExprBackend *>(backend_.get())->order_by_(order_by_expr);
    return *this;
}

SelectExpr &
SelectExpr::distinct(bool flag) {
    dynamic_cast<SelectExprBackend *>(backend_.get())->distinct(flag);
    return *this;
}

SelectExpr &
SelectExpr::with_lockmode(const String &lock_mode) {
    dynamic_cast<SelectExprBackend *>(backend_.get())->with_lockmode(lock_mode);
    return *this;
}

SelectExpr &
SelectExpr::for_update(bool flag) {
    return with_lockmode(flag? _T("UPDATE"): _T(""));
}

const Expression &
SelectExpr::select_expr() const {
    return dynamic_cast<SelectExprBackend *>(backend_.get())->select_expr();
}

const Expression &
SelectExpr::from_expr() const {
    return dynamic_cast<SelectExprBackend *>(backend_.get())->from_expr();
}

const Expression &
SelectExpr::where_expr() const {
    return dynamic_cast<SelectExprBackend *>(backend_.get())->where_expr();
}

const Expression &
SelectExpr::group_by_expr() const {
    return dynamic_cast<SelectExprBackend *>(backend_.get())->group_by_expr();
}

const Expression &
SelectExpr::having_expr() const {
    return dynamic_cast<SelectExprBackend *>(backend_.get())->having_expr();
}

const Expression &
SelectExpr::order_by_expr() const {
    return dynamic_cast<SelectExprBackend *>(backend_.get())->order_by_expr();
}

bool
SelectExpr::distinct_flag() const {
    return dynamic_cast<SelectExprBackend *>(backend_.get())->distinct_flag();
}

const String &
SelectExpr::lock_mode() const {
    return dynamic_cast<SelectExprBackend *>(backend_.get())->lock_mode();
}

bool
SelectExpr::for_update_flag() const {
    return lock_mode() == _T("UPDATE");
}

YBORM_DECL const Expression
operator ! (const Expression &a) {
    return Expression(ExprBEPtr(new UnaryOpExprBackend(true, _T("NOT"), a)));
}

YBORM_DECL const Expression
operator && (const Expression &a, const Expression &b) {
    if (a.is_empty())
        return b;
    if (b.is_empty())
        return a;
    return Expression(ExprBEPtr(new BinaryOpExprBackend(a, _T("AND"), b)));
}

YBORM_DECL const Expression
operator || (const Expression &a, const Expression &b) {
    if (a.is_empty())
        return b;
    if (b.is_empty())
        return a;
    return Expression(ExprBEPtr(new BinaryOpExprBackend(a, _T("OR"), b)));
}

YBORM_DECL const Expression
operator == (const Expression &a, const Expression &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(a, _T("="), b)));
}

YBORM_DECL const Expression
operator == (const Expression &a, const Value &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(a, _T("="), ConstExpr(b))));
}

YBORM_DECL const Expression
operator == (const Value &a, const Expression &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(ConstExpr(a), _T("="), b)));
}

YBORM_DECL const Expression
operator == (const Expression &a, const Column &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        a, _T("="), ColumnExpr(b.table().name(), b.name()))));
}

YBORM_DECL const Expression
operator == (const Column &a, const Expression &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        ColumnExpr(a.table().name(), a.name()), _T("="), b)));
}

YBORM_DECL const Expression
operator == (const Column &a, const Column &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        ColumnExpr(a.table().name(), a.name()),
        _T("="), ColumnExpr(b.table().name(), b.name()))));
}

YBORM_DECL const Expression
operator == (const Column &a, const Value &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        ColumnExpr(a.table().name(), a.name()), _T("="), ConstExpr(b))));
}

YBORM_DECL const Expression
operator == (const Value &a, const Column &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        ConstExpr(a), _T("="), ColumnExpr(b.table().name(), b.name()))));
}

YBORM_DECL const Expression
operator != (const Expression &a, const Expression &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(a, _T("<>"), b)));
}

YBORM_DECL const Expression
operator != (const Expression &a, const Value &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(a, _T("<>"), ConstExpr(b))));
}

YBORM_DECL const Expression
operator != (const Value &a, const Expression &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(ConstExpr(a), _T("<>"), b)));
}

YBORM_DECL const Expression
operator != (const Expression &a, const Column &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        a, _T("<>"), ColumnExpr(b.table().name(), b.name()))));
}

YBORM_DECL const Expression
operator != (const Column &a, const Expression &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        ColumnExpr(a.table().name(), a.name()), _T("<>"), b)));
}

YBORM_DECL const Expression
operator != (const Column &a, const Column &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        ColumnExpr(a.table().name(), a.name()),
        _T("<>"), ColumnExpr(b.table().name(), b.name()))));
}

YBORM_DECL const Expression
operator != (const Column &a, const Value &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        ColumnExpr(a.table().name(), a.name()), _T("<>"), ConstExpr(b))));
}

YBORM_DECL const Expression
operator != (const Value &a, const Column &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        ConstExpr(a), _T("<>"), ColumnExpr(b.table().name(), b.name()))));
}

YBORM_DECL const Expression
operator > (const Expression &a, const Expression &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(a, _T(">"), b)));
}

YBORM_DECL const Expression
operator > (const Expression &a, const Value &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(a, _T(">"), ConstExpr(b))));
}

YBORM_DECL const Expression
operator > (const Value &a, const Expression &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(ConstExpr(a), _T(">"), b)));
}

YBORM_DECL const Expression
operator > (const Expression &a, const Column &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        a, _T(">"), ColumnExpr(b.table().name(), b.name()))));
}

YBORM_DECL const Expression
operator > (const Column &a, const Expression &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        ColumnExpr(a.table().name(), a.name()), _T(">"), b)));
}

YBORM_DECL const Expression
operator > (const Column &a, const Column &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        ColumnExpr(a.table().name(), a.name()),
        _T(">"), ColumnExpr(b.table().name(), b.name()))));
}

YBORM_DECL const Expression
operator > (const Column &a, const Value &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        ColumnExpr(a.table().name(), a.name()), _T(">"), ConstExpr(b))));
}

YBORM_DECL const Expression
operator > (const Value &a, const Column &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        ConstExpr(a), _T(">"), ColumnExpr(b.table().name(), b.name()))));
}

YBORM_DECL const Expression
operator < (const Expression &a, const Expression &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(a, _T("<"), b)));
}

YBORM_DECL const Expression
operator < (const Expression &a, const Value &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(a, _T("<"), ConstExpr(b))));
}

YBORM_DECL const Expression
operator < (const Value &a, const Expression &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(ConstExpr(a), _T("<"), b)));
}

YBORM_DECL const Expression
operator < (const Expression &a, const Column &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        a, _T("<"), ColumnExpr(b.table().name(), b.name()))));
}

YBORM_DECL const Expression
operator < (const Column &a, const Expression &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        ColumnExpr(a.table().name(), a.name()), _T("<"), b)));
}

YBORM_DECL const Expression
operator < (const Column &a, const Column &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        ColumnExpr(a.table().name(), a.name()),
        _T("<"), ColumnExpr(b.table().name(), b.name()))));
}

YBORM_DECL const Expression
operator < (const Column &a, const Value &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        ColumnExpr(a.table().name(), a.name()), _T("<"), ConstExpr(b))));
}

YBORM_DECL const Expression
operator < (const Value &a, const Column &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        ConstExpr(a), _T("<"), ColumnExpr(b.table().name(), b.name()))));
}

YBORM_DECL const Expression
operator >= (const Expression &a, const Expression &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(a, _T(">="), b)));
}

YBORM_DECL const Expression
operator >= (const Expression &a, const Value &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(a, _T(">="), ConstExpr(b))));
}

YBORM_DECL const Expression
operator >= (const Value &a, const Expression &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(ConstExpr(a), _T(">="), b)));
}

YBORM_DECL const Expression
operator >= (const Expression &a, const Column &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        a, _T(">="), ColumnExpr(b.table().name(), b.name()))));
}

YBORM_DECL const Expression
operator >= (const Column &a, const Expression &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        ColumnExpr(a.table().name(), a.name()), _T(">="), b)));
}

YBORM_DECL const Expression
operator >= (const Column &a, const Column &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        ColumnExpr(a.table().name(), a.name()),
        _T(">="), ColumnExpr(b.table().name(), b.name()))));
}

YBORM_DECL const Expression
operator >= (const Column &a, const Value &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        ColumnExpr(a.table().name(), a.name()), _T(">="), ConstExpr(b))));
}

YBORM_DECL const Expression
operator >= (const Value &a, const Column &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        ConstExpr(a), _T(">="), ColumnExpr(b.table().name(), b.name()))));
}

YBORM_DECL const Expression
operator <= (const Expression &a, const Expression &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(a, _T("<="), b)));
}

YBORM_DECL const Expression
operator <= (const Expression &a, const Value &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(a, _T("<="), ConstExpr(b))));
}

YBORM_DECL const Expression
operator <= (const Value &a, const Expression &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(ConstExpr(a), _T("<="), b)));
}

YBORM_DECL const Expression
operator <= (const Expression &a, const Column &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        a, _T("<="), ColumnExpr(b.table().name(), b.name()))));
}

YBORM_DECL const Expression
operator <= (const Column &a, const Expression &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        ColumnExpr(a.table().name(), a.name()), _T("<="), b)));
}

YBORM_DECL const Expression
operator <= (const Column &a, const Column &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        ColumnExpr(a.table().name(), a.name()),
        _T("<="), ColumnExpr(b.table().name(), b.name()))));
}

YBORM_DECL const Expression
operator <= (const Column &a, const Value &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        ColumnExpr(a.table().name(), a.name()), _T("<="), ConstExpr(b))));
}

YBORM_DECL const Expression
operator <= (const Value &a, const Column &b) {
    return Expression(ExprBEPtr(new BinaryOpExprBackend(
        ConstExpr(a), _T("<="), ColumnExpr(b.table().name(), b.name()))));
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
FilterBackendByPK::generate_sql(Values *params, int *count) const {
    return expr_.generate_sql(params, count);
}

KeyFilter::KeyFilter(const Key &key)
    : Expression(ExprBEPtr(new FilterBackendByPK(key)))
{}

const Key &
KeyFilter::key() const
{
    return dynamic_cast<FilterBackendByPK *>(backend_.get())->key();
}

YBORM_DECL void
find_all_tables(const Expression &expr, Strings &tables)
{
    if (expr.backend()) {
        ExpressionListBackend *list_expr =
            dynamic_cast<ExpressionListBackend *> (expr.backend());
        if (list_expr) {
            int n = list_expr->size();
            for (int i = 0; i < n; ++i)
                find_all_tables(list_expr->item(i), tables);
            return;
        }
        JoinExprBackend *join_expr =
            dynamic_cast<JoinExprBackend *> (expr.backend());
        if (join_expr) {
            find_all_tables(join_expr->expr1(), tables);
            find_all_tables(join_expr->expr2(), tables);
            return;
        }
    }
    String sql = expr.get_sql();
    if (is_number_or_object_name(sql))
        tables.push_back(sql);
    else
        throw ORMError(_T("Not a table expression"));
}

YBORM_DECL SelectExpr
make_select(const Schema &schema, const Expression &from_where,
        const Expression &filter, const Expression &order_by,
        bool for_update_flag)
{
    Strings tables;
    find_all_tables(from_where, tables);
    ExpressionList cols;
    Strings::const_iterator i = tables.begin(), iend = tables.end();
    for (; i != iend; ++i) {
        const Table &table = schema[*i];
        for (size_t j = 0; j < table.size(); ++j)
            cols << ColumnExpr(table.name(), table[j].name());
    }
    SelectExpr q(cols);
    q.from_(from_where).where_(filter).order_by_(order_by)
        .for_update(for_update_flag);
    return q;
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
