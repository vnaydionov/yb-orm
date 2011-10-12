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
        vector<string> parts;
        split_str("a<-b<-c", "<-", parts);
        CPPUNIT_ASSERT_EQUAL((size_t)3, parts.size());
        CPPUNIT_ASSERT_EQUAL(string("a"), parts[0]);
        CPPUNIT_ASSERT_EQUAL(string("b"), parts[1]);
        CPPUNIT_ASSERT_EQUAL(string("c"), parts[2]);
    }

    void testSplitStrEmpty()
    {
        vector<string> parts;
        split_str("", ",", parts);
        CPPUNIT_ASSERT_EQUAL((size_t)1, parts.size());
        CPPUNIT_ASSERT_EQUAL(string(""), parts[0]);
    }

    void testSplitStrSingle()
    {
        vector<string> parts;
        split_str("a", ",", parts);
        CPPUNIT_ASSERT_EQUAL((size_t)1, parts.size());
        CPPUNIT_ASSERT_EQUAL(string("a"), parts[0]);
    }

    void testSplitStrMargins()
    {
        vector<string> parts;
        split_str(",b,", ",", parts);
        CPPUNIT_ASSERT_EQUAL((size_t)3, parts.size());
        CPPUNIT_ASSERT_EQUAL(string(""), parts[0]);
        CPPUNIT_ASSERT_EQUAL(string("b"), parts[1]);
        CPPUNIT_ASSERT_EQUAL(string(""), parts[2]);
    }

    void testJoinStr()
    {
        vector<string> parts(3);
        parts[0] = "a"; parts[1] = "b"; parts[2] = "c";
        CPPUNIT_ASSERT_EQUAL(string("a->b->c"), join_str("->", parts));
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION(TestMisc);

// vim:ts=4:sts=4:sw=4:et:
