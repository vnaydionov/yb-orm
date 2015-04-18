#include <time.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
#include "util/nlogger.h"
#include "util/string_utils.h"
#include "util/value_type.h"

using namespace std;
using namespace Yb;
using namespace Yb::StrUtils;

class TestValue : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestValue);
    CPPUNIT_TEST(test_type);
    CPPUNIT_TEST(test_value);
    CPPUNIT_TEST(test_sysdate);
    CPPUNIT_TEST(test_null);
    CPPUNIT_TEST(test_equality);
    CPPUNIT_TEST(test_less);
    CPPUNIT_TEST(test_as_sql_string);
    CPPUNIT_TEST(test_as_string);
    CPPUNIT_TEST(test_as_integer);
    CPPUNIT_TEST(test_as_float);
    CPPUNIT_TEST(test_swap);
    CPPUNIT_TEST(test_fix_type);
#if defined(YB_USE_TUPLE)
    CPPUNIT_TEST(test_tuple_values);
#endif
#if defined(YB_USE_STDTUPLE)
    CPPUNIT_TEST(test_stdtuple_values);
#endif
    CPPUNIT_TEST_EXCEPTION(test_value_is_null, ValueIsNull);
    CPPUNIT_TEST_EXCEPTION(test_value_bad_cast_long_long, ValueBadCast);
    CPPUNIT_TEST_EXCEPTION(test_value_bad_cast_integer, ValueBadCast);
    CPPUNIT_TEST_EXCEPTION(test_value_bad_cast_Decimal, ValueBadCast);
    CPPUNIT_TEST_EXCEPTION(test_value_bad_cast_date_time, ValueBadCast);
    CPPUNIT_TEST(testEmptyKey);
    CPPUNIT_TEST_SUITE_END();

public:
    void test_type()
    {
        CPPUNIT_ASSERT(Value::STRING == Value(_T("z")).get_type());
        CPPUNIT_ASSERT(Value::STRING == Value(String(_T("xyz"))).get_type());
        CPPUNIT_ASSERT(Value::INTEGER == Value(0).get_type());
        CPPUNIT_ASSERT(Value::INTEGER == Value(345).get_type());
        CPPUNIT_ASSERT(Value::LONGINT == Value((LongInt)12).get_type());
        CPPUNIT_ASSERT(Value::DECIMAL == Value(Decimal(_T("1"))).get_type());
        CPPUNIT_ASSERT(Value::DATETIME == Value(now()).get_type());
        CPPUNIT_ASSERT(Value::FLOAT == Value(1.5).get_type());
        std::string str = "abc";
        Blob data(str.begin(), str.end());
        CPPUNIT_ASSERT(Value::BLOB == Value(data).get_type());
    }

    void test_value()
    {
        CPPUNIT_ASSERT_EQUAL(string("z"), NARROW(Value(_T("z")).as_string()));
        CPPUNIT_ASSERT_EQUAL(12, (int)Value(_T("12")).as_longint());
        CPPUNIT_ASSERT(Decimal(_T("12.3")) == Value(_T("012.30")).as_decimal());
        CPPUNIT_ASSERT_EQUAL(12.3, Value(_T("12.30")).as_float());
        DateTime a(dt_make(2006, 11, 16, 15, 5, 10));
        CPPUNIT_ASSERT(a == Value(_T("2006-11-16T15:05:10")).as_date_time());
        CPPUNIT_ASSERT(a == Value(_T("2006-11-16 15:05:10")).as_date_time());
        time_t t = time(NULL);
        tm stm;
        localtime_safe(&t, &stm);
        DateTime b(dt_make(stm.tm_year + 1900, stm.tm_mon + 1, stm.tm_mday,
                    stm.tm_hour, stm.tm_min, stm.tm_sec));
        CPPUNIT_ASSERT(b == Value((LongInt)t).as_date_time());
        std::string str = "abc";
        Blob data(str.begin(), str.end());
        CPPUNIT_ASSERT(data == Value(data).as_blob());
    }

    void test_sysdate()
    {
        DateTime d0 = now();
        DateTime d = Value(_T("sysdate")).as_date_time();
        CPPUNIT_ASSERT_EQUAL(dt_year(d), dt_year(d0));
        CPPUNIT_ASSERT_EQUAL(dt_month(d), dt_month(d0));
        CPPUNIT_ASSERT_EQUAL(dt_day(d), dt_day(d0));
    }

    void test_null()
    {
        CPPUNIT_ASSERT(Value().is_null());
        CPPUNIT_ASSERT(!Value(1).is_null());
        CPPUNIT_ASSERT(!Value(_T("ab")).is_null());
        CPPUNIT_ASSERT(!Value(Decimal(1)).is_null());
        CPPUNIT_ASSERT(!Value(now()).is_null());
        std::string str = "abc";
        Blob data(str.begin(), str.end());
        CPPUNIT_ASSERT(!Value(str).is_null());
    }

    void test_equality()
    {
        CPPUNIT_ASSERT(Value() == Value());
        CPPUNIT_ASSERT(Value(1) != Value());
        CPPUNIT_ASSERT(Value() != Value(1));
        CPPUNIT_ASSERT(Value(1) == Value(1));
        CPPUNIT_ASSERT(Value(1) != Value(2));
        CPPUNIT_ASSERT(Value(_T("ab")) == Value(_T("ab")));
        CPPUNIT_ASSERT(Value(_T("ab")) != Value(_T("ac")));
        CPPUNIT_ASSERT(Value(Decimal(1)) == Value(Decimal(1)));
        CPPUNIT_ASSERT(Value(Decimal(1)) != Value(Decimal(2)));
        time_t t = time(NULL);
        DateTime a(dt_from_time_t(t)), b(dt_from_time_t(t + 1));
        CPPUNIT_ASSERT(Value(a) == Value(a));
        CPPUNIT_ASSERT(Value(a) != Value(b));
        CPPUNIT_ASSERT(Value(1.4) == Value(1.4));
        CPPUNIT_ASSERT(Value(-1.4) != Value(1.4));
    }

    void test_less()
    {
        CPPUNIT_ASSERT(!(Value() < Value()));
        CPPUNIT_ASSERT(Value() < Value(1));
        CPPUNIT_ASSERT(!(Value(1) < Value()));
        CPPUNIT_ASSERT(Value(1) < Value(2));
        CPPUNIT_ASSERT(!(Value(1) < Value(1)));
        CPPUNIT_ASSERT(Value(_T("ab")) < Value(_T("ac")));
        CPPUNIT_ASSERT(!(Value(_T("ab")) < Value(_T("ab"))));
        CPPUNIT_ASSERT(Value(Decimal(1)) < Value(Decimal(2)));
        CPPUNIT_ASSERT(!(Value(Decimal(1)) < Value(Decimal(1))));
        time_t t = time(NULL);
        DateTime a(dt_from_time_t(t)), b(dt_from_time_t(t + 1));
        CPPUNIT_ASSERT(Value(a) < Value(b));
        CPPUNIT_ASSERT(!(Value(a) < Value(a)));
        std::string str1 = "abc";
        Blob data1(str1.begin(), str1.end());
        std::string str2 = "abcd";
        Blob data2(str2.begin(), str2.end());
        CPPUNIT_ASSERT(Value(data1) < Value(data2));
    }

    void test_as_sql_string()
    {
        CPPUNIT_ASSERT_EQUAL(string("NULL"), NARROW(Value().sql_str()));
        CPPUNIT_ASSERT_EQUAL(string("123"), NARROW(Value(123).sql_str()));
        CPPUNIT_ASSERT_EQUAL(string("'a''sd'"), NARROW(Value(_T("a'sd")).sql_str()));
        CPPUNIT_ASSERT_EQUAL(string("123.45"),
                NARROW(Value(Decimal(_T("0123.450"))).sql_str()));
        DateTime a(dt_make(2006, 11, 16, 15, 5, 10));
        CPPUNIT_ASSERT_EQUAL(string("'2006-11-16 15:05:10'"), NARROW(Value(a).sql_str()));
        CPPUNIT_ASSERT_EQUAL(string("123.45"), NARROW(Value(123.45).sql_str()));
        std::string str = "'abc'";
        Blob data(str.begin(), str.end());
        CPPUNIT_ASSERT_EQUAL(string("'abc'"), NARROW(Value(data).sql_str()));
    }

    void test_as_string()
    {
        CPPUNIT_ASSERT_EQUAL(string("123"), NARROW(Value(123).as_string()));
        CPPUNIT_ASSERT_EQUAL(string("a'sd"), NARROW(Value(_T("a'sd")).as_string()));
        CPPUNIT_ASSERT_EQUAL(string("123.45"), NARROW(Value(Decimal(_T("0123.450"))).as_string()));
        DateTime a(dt_make(2006, 11, 16, 15, 5, 10));
        CPPUNIT_ASSERT_EQUAL(string("2006-11-16T15:05:10"), NARROW(Value(a).as_string()));
        String s = str_to_lower(Value(1E30).as_string());
        CPPUNIT_ASSERT(starts_with(s, _T("1e")));
        CPPUNIT_ASSERT(ends_with(s, _T("30")));
        std::string str = "abc";
        Blob data(str.begin(), str.end());
        CPPUNIT_ASSERT_EQUAL(string("abc"), NARROW(Value(data).as_string()));
    }

    void test_as_integer()
    {
        CPPUNIT_ASSERT_EQUAL(-123, Value(-123).as_integer());
        CPPUNIT_ASSERT_EQUAL(-123, Value((LongInt)-123).as_integer());
        CPPUNIT_ASSERT_EQUAL(-123, Value(_T("-123")).as_integer());
        CPPUNIT_ASSERT_EQUAL(0, Value(_T("0")).as_integer());
        CPPUNIT_ASSERT_EQUAL(0, Value(0).as_integer());
        CPPUNIT_ASSERT_EQUAL(0, Value((LongInt)0).as_integer());
        CPPUNIT_ASSERT_EQUAL(-123, Value(-123.).as_integer());
    }

    void test_as_float()
    {
        CPPUNIT_ASSERT_EQUAL(-1.23, Value(-1.23).as_float());
        CPPUNIT_ASSERT_EQUAL(-1.23, Value(_T("-1.23")).as_float());
        CPPUNIT_ASSERT_EQUAL(-123., Value(-123).as_float());
    }

    void test_swap()
    {
        Value a(1234), b(_T("xyz"));
        a.swap(b);
        CPPUNIT_ASSERT_EQUAL(string("xyz"), NARROW(a.as_string()));
        CPPUNIT_ASSERT_EQUAL(1234, b.as_integer());
        std::swap(a, b);
        CPPUNIT_ASSERT_EQUAL(string("xyz"), NARROW(b.as_string()));
        CPPUNIT_ASSERT_EQUAL(1234, a.as_integer());
    }

    void test_fix_type()
    {
        Value a(1234), b(_T("12.3"));
        CPPUNIT_ASSERT_EQUAL((int)Value::INTEGER, (int)a.get_type());
        a.fix_type(Value::DECIMAL);
        CPPUNIT_ASSERT_EQUAL((int)Value::DECIMAL, (int)a.get_type());
        CPPUNIT_ASSERT(Decimal(1234) == a.read_as<Decimal>());
        CPPUNIT_ASSERT_EQUAL((int)Value::STRING, (int)b.get_type());
        b.fix_type(Value::FLOAT);
        CPPUNIT_ASSERT_EQUAL((int)Value::FLOAT, (int)b.get_type());
        CPPUNIT_ASSERT_EQUAL(12.3, b.read_as<double>());
    }

#if defined(YB_USE_TUPLE)
    void test_tuple_values()
    {
        Values v;
        tuple_values(boost::make_tuple(1, 2.5, _T("three")), v);
        CPPUNIT_ASSERT_EQUAL(3, (int)v.size());
        CPPUNIT_ASSERT_EQUAL((int)Value::INTEGER, v[0].get_type());
        CPPUNIT_ASSERT_EQUAL(1, v[0].as_integer());
        CPPUNIT_ASSERT_EQUAL((int)Value::FLOAT, v[1].get_type());
        CPPUNIT_ASSERT_EQUAL(2.5, v[1].as_float());
        CPPUNIT_ASSERT_EQUAL((int)Value::STRING, v[2].get_type());
        CPPUNIT_ASSERT_EQUAL(string("three"), NARROW(v[2].as_string()));
    }
#endif
#if defined(YB_USE_STDTUPLE)
    void test_stdtuple_values()
    {
        Values v;
        auto t = std::make_tuple(1, 2.5, _T("three"));
        stdtuple_values<0, decltype(t)>(t, v);
        CPPUNIT_ASSERT_EQUAL(3, (int)v.size());
        CPPUNIT_ASSERT_EQUAL((int)Value::INTEGER, v[0].get_type());
        CPPUNIT_ASSERT_EQUAL(1, v[0].as_integer());
        CPPUNIT_ASSERT_EQUAL((int)Value::FLOAT, v[1].get_type());
        CPPUNIT_ASSERT_EQUAL(2.5, v[1].as_float());
        CPPUNIT_ASSERT_EQUAL((int)Value::STRING, v[2].get_type());
        CPPUNIT_ASSERT_EQUAL(string("three"), NARROW(v[2].as_string()));
    }
#endif

    void test_value_is_null()
    {
        Value().as_longint();
    }

    void test_value_bad_cast_long_long()
    {
        Value(_T("ab")).as_longint();
    }

    void test_value_bad_cast_integer()
    {
        LongInt x = 1;
        x <<= 32;
        Value(x).as_integer();
    }

    void test_value_bad_cast_Decimal()
    {
        Value(_T("ab")).as_decimal();
    }

    void test_value_bad_cast_date_time()
    {
        Value(_T("ab")).as_date_time();
    }

    void testEmptyKey()
    {
        Key k1(_T("TBL1"), ValueMap());
        CPPUNIT_ASSERT(empty_key(k1));
        k1.second.push_back(make_pair(_T("A"), Value()));
        CPPUNIT_ASSERT(empty_key(k1));
        k1.second.push_back(make_pair(_T("B"), Value()));
        CPPUNIT_ASSERT(empty_key(k1));
        k1.second[0].second = Value(10);
        CPPUNIT_ASSERT(empty_key(k1));
        k1.second[1].second = Value(String());
        CPPUNIT_ASSERT(empty_key(k1));
        k1.second[1].second = Value(String(_T("X")));
        CPPUNIT_ASSERT(!empty_key(k1));
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestValue);

// vim:ts=4:sts=4:sw=4:et:
