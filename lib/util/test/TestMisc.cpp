#include <iostream>
#include <iomanip>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <cppunit/extensions/HelperMacros.h>

#include <util/str_utils.hpp>

using namespace std;
using namespace Yb::StrUtils;

class TestMisc: public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestMisc);

    CPPUNIT_TEST(testSplitStr);
    CPPUNIT_TEST(testSplitStrEmpty);
    CPPUNIT_TEST(testSplitStrSingle);
    CPPUNIT_TEST(testSplitStrMargins);
    CPPUNIT_TEST(testJoinStr);

    CPPUNIT_TEST_SUITE_END();   

public:
    void testSplitStr()
    {
        vector<Yb::String> parts;
        split_str(_T("a<-b<-c"), _T("<-"), parts);
        CPPUNIT_ASSERT_EQUAL((size_t)3, parts.size());
        CPPUNIT_ASSERT_EQUAL(string("a"), NARROW(parts[0]));
        CPPUNIT_ASSERT_EQUAL(string("b"), NARROW(parts[1]));
        CPPUNIT_ASSERT_EQUAL(string("c"), NARROW(parts[2]));
    }

    void testSplitStrEmpty()
    {
        vector<Yb::String> parts;
        split_str(_T(""), _T(","), parts);
        CPPUNIT_ASSERT_EQUAL((size_t)1, parts.size());
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(parts[0]));
    }

    void testSplitStrSingle()
    {
        vector<Yb::String> parts;
        split_str(_T("a"), _T(","), parts);
        CPPUNIT_ASSERT_EQUAL((size_t)1, parts.size());
        CPPUNIT_ASSERT_EQUAL(string("a"), NARROW(parts[0]));
    }

    void testSplitStrMargins()
    {
        vector<Yb::String> parts;
        split_str(_T(",b,"), _T(","), parts);
        CPPUNIT_ASSERT_EQUAL((size_t)3, parts.size());
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(parts[0]));
        CPPUNIT_ASSERT_EQUAL(string("b"), NARROW(parts[1]));
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(parts[2]));
    }

    void testJoinStr()
    {
        vector<Yb::String> parts(3);
        parts[0] = _T("a"); parts[1] = _T("b"); parts[2] = _T("c");
        CPPUNIT_ASSERT_EQUAL(string("a->b->c"), NARROW(join_str(_T("->"), parts)));
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION(TestMisc);

// vim:ts=4:sts=4:sw=4:et:
