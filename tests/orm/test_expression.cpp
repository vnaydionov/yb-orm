#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
#include "orm/expression.h"
#include "orm/schema.h"

using namespace std;
using namespace Yb;

class TestExpression : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestExpression);
    CPPUNIT_TEST(testFilterAll);
    CPPUNIT_TEST(testFilterEq);
    CPPUNIT_TEST(testOperatorOr);
    CPPUNIT_TEST(testOperatorAnd);
    CPPUNIT_TEST(testLike);
    CPPUNIT_TEST(testIn);
    CPPUNIT_TEST(testCollectParams);
    CPPUNIT_TEST(testExprList);
#if defined(YB_USE_TUPLE)
    CPPUNIT_TEST(testExprListTuple);
#endif // defined(YB_USE_TUPLE)
#if defined(YB_USE_STDTUPLE)
    CPPUNIT_TEST(testExprListStdTuple);
#endif // defined(YB_USE_STDTUPLE)
    CPPUNIT_TEST(testSelectExpr);
    CPPUNIT_TEST(testFindAllTables);
    CPPUNIT_TEST(testParenth);
    CPPUNIT_TEST_SUITE_END();

public:
    void testFilterAll()
    {
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(Expression().get_sql()));
    }

    void testFilterEq()
    {
        CPPUNIT_ASSERT_EQUAL(string("ID = 1"), NARROW((Expression(_T("ID")) == 1).get_sql()));
    }

    void testOperatorOr()
    {
        CPPUNIT_ASSERT_EQUAL(string("(ID = 1) OR (A <= 'a')"),
                             NARROW((Expression(_T("ID")) == 1 ||
                                     Expression(_T("A")) <= String(_T("a"))).get_sql()));
    }

    void testOperatorAnd()
    {
        CPPUNIT_ASSERT_EQUAL(string("(ID = 1) AND (A <> 'a')"),
                             NARROW((Expression(_T("ID")) == 1 &&
                                     Expression(_T("A")) != String(_T("a"))).get_sql()));
    }

    void testLike()
    {
        Table t(_T("a"));
        t.add_column(Column(_T("x"), Value::STRING, 10, 0));
        ConstExpr e(Value(_T("abc%")));
        CPPUNIT_ASSERT_EQUAL(string("a.x LIKE 'abc%'"),
                             NARROW(t[_T("x")].like_(ConstExpr(Value(_T("abc%")))).get_sql()));
    }

    void testIn()
    {
        Table t(_T("a"));
        t.add_column(Column(_T("x"), Value::STRING, 10, 0));
        CPPUNIT_ASSERT_EQUAL(string("a.x IN ('a', 'bb')"),
                             NARROW(t[_T("x")].in_(ExpressionList(
                                         ConstExpr(Value(_T("a"))),
                                         ConstExpr(Value(_T("bb")))
                                        )).get_sql()));
#if defined(YB_USE_TUPLE)
        CPPUNIT_ASSERT_EQUAL(string("a.x IN ('a', 'bb')"),
                             NARROW(t[_T("x")].in_(boost::make_tuple(_T("a"), _T("bb"))).get_sql()));
#endif // defined(YB_USE_TUPLE)
#if defined(YB_USE_STDTUPLE)
        CPPUNIT_ASSERT_EQUAL(string("a.x IN ('a', 'bb')"),
                             NARROW(t[_T("x")].in_(std::make_tuple(_T("a"), _T("bb"))).get_sql()));
#endif // defined(YB_USE_STDTUPLE)
    }

    void testCollectParams()
    {
        Expression expr = Expression(_T("ID")) == 1 &&
            Expression(_T("A")) != String(_T("a"));
        SqlGeneratorOptions options1(NO_QUOTES, true, true);
        SqlGeneratorContext ctx1;
        String sql = expr.generate_sql(options1, &ctx1);
        CPPUNIT_ASSERT_EQUAL(string("(ID = ?) AND (A <> ?)"),
                NARROW(sql));
        SqlGeneratorOptions options2(NO_QUOTES, true, true, true);
        SqlGeneratorContext ctx2;
        sql = expr.generate_sql(options2, &ctx2);
        CPPUNIT_ASSERT_EQUAL(string("(ID = :1) AND (A <> :2)"),
                NARROW(sql));
        CPPUNIT_ASSERT_EQUAL(string("(ID = 1) AND (A <> 'a')"),
                NARROW(expr.get_sql()));
        CPPUNIT_ASSERT_EQUAL((size_t)2, ctx1.params_.size());
        CPPUNIT_ASSERT_EQUAL(1, ctx1.params_[0].as_integer());
        CPPUNIT_ASSERT_EQUAL(string("a"), NARROW(ctx1.params_[1].as_string()));
        CPPUNIT_ASSERT_EQUAL((size_t)2, ctx2.params_.size());
        CPPUNIT_ASSERT_EQUAL(1, ctx2.params_[0].as_integer());
        CPPUNIT_ASSERT_EQUAL(string("a"), NARROW(ctx2.params_[1].as_string()));
    }

    void testExprList()
    {
        ExpressionList expr(ConstExpr(1));
        expr << ConstExpr(2) << ConstExpr(String(_T("three")));
        CPPUNIT_ASSERT_EQUAL(string("1, 2, 'three'"),
                NARROW(expr.get_sql()));
    }

#if defined(YB_USE_TUPLE)
    void testExprListTuple()
    {
        ExpressionList expr(boost::make_tuple(1, 2.01, _T("three")));
        expr << ConstExpr(Decimal(_T("4"))) << ConstExpr(Value(_T("five")));
        CPPUNIT_ASSERT_EQUAL(string("1, 2.01, 'three', 4, 'five'"),
                NARROW(expr.get_sql()));
    }
#endif // defined(YB_USE_TUPLE)

#if defined(YB_USE_STDTUPLE)
    void testExprListStdTuple()
    {
        ExpressionList expr(std::make_tuple(1, 2.01, _T("three")));
        expr << ConstExpr(Decimal(_T("4"))) << ConstExpr(Value(_T("five")));
        CPPUNIT_ASSERT_EQUAL(string("1, 2.01, 'three', 4, 'five'"),
                NARROW(expr.get_sql()));
    }
#endif // defined(YB_USE_STDTUPLE)

    void testSelectExpr()
    {
        SelectExpr q(Expression(_T("A")));
        q.from_(ExpressionList(Expression(_T("DUAL"))));
        CPPUNIT_ASSERT_EQUAL(string("SELECT A FROM DUAL"),
                NARROW(q.get_sql()));
        q.where_(Expression(_T("A")) == String(_T("a")));
        CPPUNIT_ASSERT_EQUAL(string("SELECT A FROM DUAL WHERE A = 'a'"),
                NARROW(q.get_sql()));
    }

    void testFindAllTables()
    {
        ExpressionList expr(Expression(_T("A")),
                JoinExpr(JoinExpr(Expression(_T("B")),
                                  Expression(_T("C")),
                                  Expression(_T("X"))),
                         Expression(_T("D")),
                         Expression(_T("Y"))));
        Strings tables;
        find_all_tables(expr, tables);
        CPPUNIT_ASSERT_EQUAL((size_t)4, tables.size());
        CPPUNIT_ASSERT_EQUAL(string("A"), NARROW(tables[0]));
        CPPUNIT_ASSERT_EQUAL(string("B"), NARROW(tables[1]));
        CPPUNIT_ASSERT_EQUAL(string("C"), NARROW(tables[2]));
        CPPUNIT_ASSERT_EQUAL(string("D"), NARROW(tables[3]));
    }

    void testParenth()
    {
        CPPUNIT_ASSERT(is_number_or_object_name(_T("abz_ABZ#.xy09$")));
        CPPUNIT_ASSERT(is_number_or_object_name(_T("3.1415")));
        CPPUNIT_ASSERT(!is_number_or_object_name(_T("2-1")));
        CPPUNIT_ASSERT(is_string_constant(_T("''")));
        CPPUNIT_ASSERT(is_string_constant(_T("'abc'")));
        CPPUNIT_ASSERT(is_string_constant(_T("'ab''cd\"'")));
        CPPUNIT_ASSERT(is_string_constant(_T("'a'''")));
        CPPUNIT_ASSERT(!is_string_constant(_T("'a'b'")));
        CPPUNIT_ASSERT(!is_string_constant(_T("'a'b")));
        CPPUNIT_ASSERT(is_in_parentheses(_T("()")));
        CPPUNIT_ASSERT(is_in_parentheses(_T("(a(b)c(d))")));
        CPPUNIT_ASSERT(is_in_parentheses(_T("(a(b)')c('d)")));
        CPPUNIT_ASSERT(!is_in_parentheses(_T("a(bc)d")));
        CPPUNIT_ASSERT(!is_in_parentheses(_T("(a(b))c(d)")));
        CPPUNIT_ASSERT(!is_in_parentheses(_T("(a(bcd)")));
        CPPUNIT_ASSERT(!is_in_parentheses(_T("(a)bcd)")));
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestExpression);

// vim:ts=4:sts=4:sw=4:et:
