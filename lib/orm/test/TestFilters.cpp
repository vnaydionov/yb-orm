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
        CPPUNIT_ASSERT_EQUAL(string("(ID = 1) OR (A = ('a'))"), 
                             NARROW((filter_eq(_T("ID"), Value(1)) ||
                                     filter_eq(_T("A"), Value(_T("a")))).get_sql()));
    }

    void testOperatorAnd()
    {
        CPPUNIT_ASSERT_EQUAL(string("(ID = 1) AND (A <> ('a'))"), 
                             NARROW((filter_eq(_T("ID"), Value(1)) &&
                                     filter_ne(_T("A"), Value(_T("a")))).get_sql()));
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestFilters);

// vim:ts=4:sts=4:sw=4:et:
