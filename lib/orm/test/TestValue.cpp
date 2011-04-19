#include <time.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
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
    CPPUNIT_TEST_EXCEPTION(test_value_is_null, ValueIsNull);
    CPPUNIT_TEST_EXCEPTION(test_value_bad_cast_long_long, ValueBadCast);
    CPPUNIT_TEST_EXCEPTION(test_value_bad_cast_Decimal, ValueBadCast);
    CPPUNIT_TEST_EXCEPTION(test_value_bad_cast_date_time, ValueBadCast);
    CPPUNIT_TEST_SUITE_END();

public:
    void test_type()
    {
        CPPUNIT_ASSERT(Value::STRING == Value("z").get_type());
        CPPUNIT_ASSERT(Value::LONGINT == Value(12).get_type());
        CPPUNIT_ASSERT(Value::DECIMAL == Value(Decimal("1")).get_type());
        DateTime a(now());
        CPPUNIT_ASSERT(Value::DATETIME == Value(a).get_type());
    }

    void test_value()
    {
        CPPUNIT_ASSERT_EQUAL(string("z"), Value("z").as_string());
        CPPUNIT_ASSERT_EQUAL(12, (int)Value("12").as_longint());
        CPPUNIT_ASSERT(Decimal("12.3") == Value("012.30").as_decimal());
        DateTime a(mk_datetime(2006, 11, 16, 15, 5, 10));
        CPPUNIT_ASSERT(a == Value("2006-11-16T15:05:10").as_date_time());
        CPPUNIT_ASSERT(a == Value("2006-11-16 15:05:10").as_date_time());
        time_t t = time(NULL);
        tm stm;
        localtime_r(&t, &stm);
        DateTime b(mk_datetime(stm.tm_year + 1900, stm.tm_mon + 1, stm.tm_mday,
                    stm.tm_hour, stm.tm_min, stm.tm_sec));
        CPPUNIT_ASSERT(b == Value(t).as_date_time());
    }

    void test_null()
    {
        CPPUNIT_ASSERT(Value().is_null());
        CPPUNIT_ASSERT(!Value(1).is_null());
        CPPUNIT_ASSERT(!Value("ab").is_null());
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
        CPPUNIT_ASSERT(Value("ab") == Value("ab"));
        CPPUNIT_ASSERT(Value("ab") != Value("ac"));
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
        CPPUNIT_ASSERT(Value("ab") < Value("ac"));
        CPPUNIT_ASSERT(!(Value("ab") < Value("ab")));
        CPPUNIT_ASSERT(Value(Decimal(1)) < Value(Decimal(2)));
        CPPUNIT_ASSERT(!(Value(Decimal(1)) < Value(Decimal(1))));
        time_t t = time(NULL);
        DateTime a(mk_datetime(t)), b(mk_datetime(t + 1));
        CPPUNIT_ASSERT(Value(a) < Value(b));
        CPPUNIT_ASSERT(!(Value(a) < Value(a)));
    }

    void test_as_sql_string()
    {
        CPPUNIT_ASSERT_EQUAL(string("NULL"), Value().sql_str());
        CPPUNIT_ASSERT_EQUAL(string("123"), Value(123).sql_str());
        CPPUNIT_ASSERT_EQUAL(string("'a''sd'"), Value("a'sd").sql_str());
        CPPUNIT_ASSERT_EQUAL(string("123.45"), Value(Decimal("0123.450")).sql_str());
        Value t(now());
        //CPPUNIT_ASSERT_EQUAL(
        //        "TO_DATE('" + t.as_string() + "', 'YYYY-MM-DD\"T\"HH24:MI:SS')",
        //        t.sql_str());
    }

    void test_as_string()
    {
        CPPUNIT_ASSERT_EQUAL(string("123"), Value(123).as_string());
        CPPUNIT_ASSERT_EQUAL(string("a'sd"), Value("a'sd").as_string());
        CPPUNIT_ASSERT_EQUAL(string("123.45"), Value(Decimal("0123.450")).as_string());
        DateTime a(mk_datetime(2006, 11, 16, 15, 5, 10));
        CPPUNIT_ASSERT_EQUAL(string("2006-11-16T15:05:10"), Value(a).as_string());
    }

    void test_value_is_null()
    {
        Value().as_longint();
    }

    void test_value_bad_cast_long_long()
    {
        Value("ab").as_longint();
    }

    void test_value_bad_cast_Decimal()
    {
        Value("ab").as_decimal();
    }

    void test_value_bad_cast_date_time()
    {
        Value("ab").as_date_time();
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestValue);

// vim:ts=4:sts=4:sw=4:et:
