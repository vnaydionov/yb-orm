#include <iostream>
#include <iomanip>
#include <sstream>
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
    CPPUNIT_TEST(testSplitStrByChars);
    CPPUNIT_TEST(testSplitStrByCharsEmpty);
    CPPUNIT_TEST(testSplitStrByCharsSingle);
    CPPUNIT_TEST(testSplitStrByCharsLimit);
    CPPUNIT_TEST(testSplitPath);
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

    void testSplitStrByChars()
    {
        vector<Yb::String> parts;
        split_str_by_chars(_T("-a<-b<-c<"), _T("-<"), parts);
        CPPUNIT_ASSERT_EQUAL((size_t)3, parts.size());
        CPPUNIT_ASSERT_EQUAL(string("a"), NARROW(parts[0]));
        CPPUNIT_ASSERT_EQUAL(string("b"), NARROW(parts[1]));
        CPPUNIT_ASSERT_EQUAL(string("c"), NARROW(parts[2]));
    }

    void testSplitStrByCharsEmpty()
    {
        vector<Yb::String> parts;
        split_str_by_chars(_T(",,.."), _T(".,"), parts);
        CPPUNIT_ASSERT_EQUAL((size_t)0, parts.size());
    }

    void testSplitStrByCharsSingle()
    {
        vector<Yb::String> parts;
        split_str_by_chars(_T("a"), _T(","), parts);
        CPPUNIT_ASSERT_EQUAL((size_t)1, parts.size());
        CPPUNIT_ASSERT_EQUAL(string("a"), NARROW(parts[0]));
        parts.clear();
        split_str_by_chars(_T(",a"), _T(","), parts);
        CPPUNIT_ASSERT_EQUAL((size_t)1, parts.size());
        CPPUNIT_ASSERT_EQUAL(string("a"), NARROW(parts[0]));
        parts.clear();
        split_str_by_chars(_T("a,"), _T(","), parts);
        CPPUNIT_ASSERT_EQUAL((size_t)1, parts.size());
        CPPUNIT_ASSERT_EQUAL(string("a"), NARROW(parts[0]));
    }

    void testSplitStrByCharsLimit()
    {
        vector<Yb::String> parts;
        split_str_by_chars(_T(",b,c,d,"), _T(","), parts, 2);
        CPPUNIT_ASSERT_EQUAL((size_t)2, parts.size());
        CPPUNIT_ASSERT_EQUAL(string("b"), NARROW(parts[0]));
        CPPUNIT_ASSERT_EQUAL(string("c,d,"), NARROW(parts[1]));
    }

    void testSplitPath()
    {
        vector<Yb::String> parts;
        split_path(_T("/usr/bin/ls.exe"), parts);
        CPPUNIT_ASSERT_EQUAL((size_t)3, parts.size());
        CPPUNIT_ASSERT_EQUAL(string("usr"), NARROW(parts[0]));
        CPPUNIT_ASSERT_EQUAL(string("bin"), NARROW(parts[1]));
        CPPUNIT_ASSERT_EQUAL(string("ls.exe"), NARROW(parts[2]));
        parts.clear();
        split_path(_T("usr/bin/ls.exe"), parts);
        CPPUNIT_ASSERT_EQUAL((size_t)3, parts.size());
        CPPUNIT_ASSERT_EQUAL(string("usr"), NARROW(parts[0]));
        CPPUNIT_ASSERT_EQUAL(string("bin"), NARROW(parts[1]));
        CPPUNIT_ASSERT_EQUAL(string("ls.exe"), NARROW(parts[2]));
        parts.clear();
        split_path(_T("usr/bin/ls.exe/"), parts);
        CPPUNIT_ASSERT_EQUAL((size_t)3, parts.size());
        CPPUNIT_ASSERT_EQUAL(string("usr"), NARROW(parts[0]));
        CPPUNIT_ASSERT_EQUAL(string("bin"), NARROW(parts[1]));
        CPPUNIT_ASSERT_EQUAL(string("ls.exe"), NARROW(parts[2]));
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
