#include <iostream>
#include <iomanip>
#include <sstream>
#include <cppunit/extensions/HelperMacros.h>

#include <util/str_utils.hpp>

using namespace std;
using namespace Yb;
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
    CPPUNIT_TEST(testUrlEncode);
    CPPUNIT_TEST(testUrlDecode);
    CPPUNIT_TEST(testParseUrl);
    CPPUNIT_TEST(testFormatUrl);

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

    void testUrlEncode()
    {
        CPPUNIT_ASSERT_EQUAL(string("%20%21abc%40%EF%0D"),
                NARROW(url_encode(" !abc@\xef\r")));
    }

    void testUrlDecode()
    {
        CPPUNIT_ASSERT_EQUAL(string(" !abc@\xef\r"),
                url_decode(_T("%20%21abc%40%EF%0D")));
    }

    void testParseUrl()
    {
        Dict d = parse_url(
            _T("mysql+qtsql://usr:pwd@my.host:123/test_db?x1=a&y=bb#qw"));
        CPPUNIT_ASSERT_EQUAL(string("mysql"), NARROW(d[_T("&proto")]));
        CPPUNIT_ASSERT_EQUAL(string("qtsql"), NARROW(d[_T("&proto_ext")]));
        CPPUNIT_ASSERT_EQUAL(string("usr"), NARROW(d[_T("&user")]));
        CPPUNIT_ASSERT_EQUAL(string("pwd"), NARROW(d[_T("&passwd")]));
        CPPUNIT_ASSERT_EQUAL(string("my.host"), NARROW(d[_T("&host")]));
        CPPUNIT_ASSERT_EQUAL(string("123"), NARROW(d[_T("&port")]));
        CPPUNIT_ASSERT_EQUAL(string("/test_db"), NARROW(d[_T("&path")]));
        CPPUNIT_ASSERT_EQUAL(string("qw"), NARROW(d[_T("&anchor")]));
        CPPUNIT_ASSERT_EQUAL(string("a"), NARROW(d[_T("x1")]));
        CPPUNIT_ASSERT_EQUAL(string("bb"), NARROW(d[_T("y")]));
        d = parse_url(_T("sqlite://C:/docs%20etc/file.db"));
        CPPUNIT_ASSERT_EQUAL(string("sqlite"), NARROW(d[_T("&proto")]));
        CPPUNIT_ASSERT_EQUAL(string("C:/docs etc/file.db"),
                NARROW(d[_T("&path")]));
        CPPUNIT_ASSERT_EQUAL(true, d.empty(_T("&host")));
    }

    void testFormatUrl()
    {
        Dict d; 
        d[_T("&proto")] = _T("mysql");
        d[_T("&proto_ext")] = _T("qtsql");
        d[_T("&user")] = _T("usr");
        d[_T("&passwd")] = _T("pwd");
        d[_T("&host")] = _T("my.host");
        d[_T("&port")] = _T("123");
        d[_T("&path")] = _T("/test_db");
        d[_T("x1")] = _T("a");
        d[_T("y")] = _T("bb");
        d[_T("&anchor")] = _T("qw");
        CPPUNIT_ASSERT_EQUAL(string(
            "mysql+qtsql://usr@my.host:123/test_db?x1=a&y=bb#qw"),
                NARROW(format_url(d)));
        CPPUNIT_ASSERT_EQUAL(string(
            "mysql+qtsql://usr:pwd@my.host:123/test_db?x1=a&y=bb#qw"),
                NARROW(format_url(d, false)));
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestMisc);

// vim:ts=4:sts=4:sw=4:et:
