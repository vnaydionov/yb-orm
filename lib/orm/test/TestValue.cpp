#include <time.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
#include <util/nlogger.h>
#include <orm/Value.h>

using namespace std;
using namespace Yb;

class TestValue : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestValue);
    CPPUNIT_TEST(test_type);
    CPPUNIT_TEST(test_value);
    CPPUNIT_TEST(test_null);
    CPPUNIT_TEST(test_equality);
    CPPUNIT_TEST(test_less);
    CPPUNIT_TEST(test_as_sql_string);
    CPPUNIT_TEST(test_as_string);
    CPPUNIT_TEST(test_as_integer);
    CPPUNIT_TEST_EXCEPTION(test_value_is_null, ValueIsNull);
    CPPUNIT_TEST_EXCEPTION(test_value_bad_cast_long_long, ValueBadCast);
    CPPUNIT_TEST_EXCEPTION(test_value_bad_cast_integer, ValueBadCast);
    CPPUNIT_TEST_EXCEPTION(test_value_bad_cast_Decimal, ValueBadCast);
    CPPUNIT_TEST_EXCEPTION(test_value_bad_cast_date_time, ValueBadCast);
    CPPUNIT_TEST_SUITE_END();

public:
    void test_type()
    {
        CPPUNIT_ASSERT(Value::STRING == Value(_T("z")).get_type());
        CPPUNIT_ASSERT(Value::INTEGER == Value(0).get_type());
        CPPUNIT_ASSERT(Value::INTEGER == Value(12).get_type());
        CPPUNIT_ASSERT(Value::LONGINT == Value((LongInt)12).get_type());
        CPPUNIT_ASSERT(Value::DECIMAL == Value(Decimal(_T("1"))).get_type());
        DateTime a(now());
        CPPUNIT_ASSERT(Value::DATETIME == Value(a).get_type());
    }

    void test_value()
    {
        CPPUNIT_ASSERT_EQUAL(string("z"), NARROW(Value(_T("z")).as_string()));
        CPPUNIT_ASSERT_EQUAL(12, (int)Value(_T("12")).as_longint());
        CPPUNIT_ASSERT(Decimal(_T("12.3")) == Value(_T("012.30")).as_decimal());
        DateTime a(mk_datetime(2006, 11, 16, 15, 5, 10));
        CPPUNIT_ASSERT(a == Value(_T("2006-11-16T15:05:10")).as_date_time());
        CPPUNIT_ASSERT(a == Value(_T("2006-11-16 15:05:10")).as_date_time());
        time_t t = time(NULL);
        tm stm;
        localtime_safe(&t, &stm);
        DateTime b(mk_datetime(stm.tm_year + 1900, stm.tm_mon + 1, stm.tm_mday,
                    stm.tm_hour, stm.tm_min, stm.tm_sec));
        CPPUNIT_ASSERT(b == Value((LongInt)t).as_date_time());
    }

    void test_null()
    {
        CPPUNIT_ASSERT(Value().is_null());
        CPPUNIT_ASSERT(!Value(1).is_null());
        CPPUNIT_ASSERT(!Value(_T("ab")).is_null());
        CPPUNIT_ASSERT(!Value(Decimal(1)).is_null());
        CPPUNIT_ASSERT(!Value(now()).is_null());
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
        DateTime a(mk_datetime(t)), b(mk_datetime(t + 1));
        CPPUNIT_ASSERT(Value(a) == Value(a));
        CPPUNIT_ASSERT(Value(a) != Value(b));
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
        DateTime a(mk_datetime(t)), b(mk_datetime(t + 1));
        CPPUNIT_ASSERT(Value(a) < Value(b));
        CPPUNIT_ASSERT(!(Value(a) < Value(a)));
    }

    void test_as_sql_string()
    {
        CPPUNIT_ASSERT_EQUAL(string("NULL"), NARROW(Value().sql_str()));
        CPPUNIT_ASSERT_EQUAL(string("123"), NARROW(Value(123).sql_str()));
        CPPUNIT_ASSERT_EQUAL(string("'a''sd'"), NARROW(Value(_T("a'sd")).sql_str()));
        CPPUNIT_ASSERT_EQUAL(string("123.45"),
                NARROW(Value(Decimal(_T("0123.450"))).sql_str()));
        DateTime a(mk_datetime(2006, 11, 16, 15, 5, 10));
        CPPUNIT_ASSERT_EQUAL(string("'2006-11-16 15:05:10'"), NARROW(Value(a).sql_str()));
    }

    void test_as_string()
    {
        CPPUNIT_ASSERT_EQUAL(string("123"), NARROW(Value(123).as_string()));
        CPPUNIT_ASSERT_EQUAL(string("a'sd"), NARROW(Value(_T("a'sd")).as_string()));
        CPPUNIT_ASSERT_EQUAL(string("123.45"), NARROW(Value(Decimal(_T("0123.450"))).as_string()));
        DateTime a(mk_datetime(2006, 11, 16, 15, 5, 10));
        CPPUNIT_ASSERT_EQUAL(string("2006-11-16T15:05:10"), NARROW(Value(a).as_string()));
    }

    void test_as_integer()
    {
        CPPUNIT_ASSERT_EQUAL(-123, Value(-123).as_integer());
        CPPUNIT_ASSERT_EQUAL(-123, Value((LongInt)-123).as_integer());
        CPPUNIT_ASSERT_EQUAL(-123, Value(_T("-123")).as_integer());
        CPPUNIT_ASSERT_EQUAL(0, Value(_T("0")).as_integer());
        CPPUNIT_ASSERT_EQUAL(0, Value(0).as_integer());
        CPPUNIT_ASSERT_EQUAL(0, Value((LongInt)0).as_integer());
    }

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
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestValue);

// vim:ts=4:sts=4:sw=4:et:
