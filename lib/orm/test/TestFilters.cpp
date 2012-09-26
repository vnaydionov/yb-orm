#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
#include <orm/Filters.h>

using namespace std;
using namespace Yb;

class TestFilters : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestFilters);
    CPPUNIT_TEST(testFilterAll);
    CPPUNIT_TEST(testFilterEq);
    CPPUNIT_TEST(testOperatorOr);
    CPPUNIT_TEST(testOperatorAnd);
    CPPUNIT_TEST(testCollectParams);
    CPPUNIT_TEST(testExprList);
    CPPUNIT_TEST(testParenth);
    CPPUNIT_TEST_SUITE_END();

public:
    void testFilterAll()
    {
        CPPUNIT_ASSERT_EQUAL(string(), NARROW(Filter().get_sql()));
    }

    void testFilterEq()
    {
        CPPUNIT_ASSERT_EQUAL(string("ID = 1"), NARROW(filter_eq(_T("ID"), Value(1)).get_sql()));
    }

    void testOperatorOr()
    {
        CPPUNIT_ASSERT_EQUAL(string("(ID = 1) OR (A = 'a')"), 
                             NARROW((filter_eq(_T("ID"), Value(1)) ||
                                     filter_eq(_T("A"), Value(_T("a")))).get_sql()));
    }

    void testOperatorAnd()
    {
        CPPUNIT_ASSERT_EQUAL(string("(ID = 1) AND (A <> 'a')"), 
                             NARROW((filter_eq(_T("ID"), Value(1)) &&
                                     filter_ne(_T("A"), Value(_T("a")))).get_sql()));
    }

    void testCollectParams()
    {
        Expression expr = filter_eq(_T("ID"), Value(1)) &&
            filter_ne(_T("A"), Value(_T("a")));
        Values pvalues;
        String sql = expr.collect_params_and_build_sql(pvalues);
        CPPUNIT_ASSERT_EQUAL(string("(ID = 1) AND (A <> 'a')"),
                NARROW(expr.get_sql()));
        CPPUNIT_ASSERT_EQUAL((size_t)2, pvalues.size());
        CPPUNIT_ASSERT_EQUAL(1, pvalues[0].as_integer());
        CPPUNIT_ASSERT_EQUAL(string("a"), NARROW(pvalues[1].as_string()));
    }

    void testExprList()
    {
        ExpressionList expr(ConstExpr(1));
        expr << ConstExpr(2) << ConstExpr(_T("three"));
        CPPUNIT_ASSERT_EQUAL(string("1, 2, 'three'"),
                NARROW(expr.get_sql()));
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

CPPUNIT_TEST_SUITE_REGISTRATION(TestFilters);

// vim:ts=4:sts=4:sw=4:et:
