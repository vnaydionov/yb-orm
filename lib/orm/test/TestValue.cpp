#include <time.h>
#include <boost/lexical_cast.hpp>
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
    CPPUNIT_TEST_EXCEPTION(test_value_bad_cast_decimal, ValueBadCast);
    CPPUNIT_TEST_EXCEPTION(test_value_bad_cast_date_time, ValueBadCast);
    CPPUNIT_TEST_SUITE_END();

public:
    void test_type()
    {
        CPPUNIT_ASSERT(Value::String == Value("z").get_type());
        CPPUNIT_ASSERT(Value::LongLong == Value(12).get_type());
        CPPUNIT_ASSERT(Value::Decimal == Value(decimal("1")).get_type());
        boost::posix_time::ptime a(boost::posix_time::second_clock::local_time());
        CPPUNIT_ASSERT(Value::DateTime == Value(a).get_type());
    }

    void test_value()
    {
        CPPUNIT_ASSERT_EQUAL(string("z"), Value("z").as_string());
        CPPUNIT_ASSERT_EQUAL(12LL, Value("12").as_long_long());
        CPPUNIT_ASSERT(decimal("12.3") == Value("012.30").as_decimal());
        boost::posix_time::ptime a(
                boost::gregorian::date(2006, 11, 16),
                boost::posix_time::time_duration(15, 5, 10));
        CPPUNIT_ASSERT(a == Value("2006-11-16T15:05:10").as_date_time());
        CPPUNIT_ASSERT(a == Value("2006-11-16 15:05:10").as_date_time());
        time_t t = time(NULL);
        tm stm;
        localtime_r(&t, &stm);
        boost::posix_time::ptime b = boost::posix_time::ptime(
                boost::gregorian::date(stm.tm_year + 1900, stm.tm_mon + 1, stm.tm_mday),
                boost::posix_time::time_duration(stm.tm_hour, stm.tm_min, stm.tm_sec));
        CPPUNIT_ASSERT(b == Value(t).as_date_time());
    }

    void test_null()
    {
        CPPUNIT_ASSERT(Value().is_null());
        CPPUNIT_ASSERT(!Value(1).is_null());
        CPPUNIT_ASSERT(!Value("ab").is_null());
        CPPUNIT_ASSERT(!Value(decimal(1)).is_null());
        CPPUNIT_ASSERT(!Value(boost::posix_time::second_clock::local_time()).is_null());
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
        CPPUNIT_ASSERT(Value(decimal(1)) == Value(decimal(1)));
        CPPUNIT_ASSERT(Value(decimal(1)) != Value(decimal(2)));
        time_t t = time(NULL);
        boost::posix_time::ptime a = boost::posix_time::from_time_t(t),
            b = boost::posix_time::from_time_t(t + 1);
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
        CPPUNIT_ASSERT(Value(decimal(1)) < Value(decimal(2)));
        CPPUNIT_ASSERT(!(Value(decimal(1)) < Value(decimal(1))));
        time_t t = time(NULL);
        boost::posix_time::ptime a = boost::posix_time::from_time_t(t),
            b = boost::posix_time::from_time_t(t + 1);
        CPPUNIT_ASSERT(Value(a) < Value(b));
        CPPUNIT_ASSERT(!(Value(a) < Value(a)));
    }

    void test_as_sql_string()
    {
        CPPUNIT_ASSERT_EQUAL(string("NULL"), Value().sql_str());
        CPPUNIT_ASSERT_EQUAL(string("123"), Value(123).sql_str());
        CPPUNIT_ASSERT_EQUAL(string("'a''sd'"), Value("a'sd").sql_str());
        CPPUNIT_ASSERT_EQUAL(string("123.45"), Value(decimal("0123.450")).sql_str());
        Value now(boost::posix_time::second_clock::local_time());
        CPPUNIT_ASSERT_EQUAL(
                "TO_DATE('" + now.as_string() + "', 'YYYY-MM-DD\"T\"HH24:MI:SS')",
                now.sql_str());
    }

    void test_as_string()
    {
        CPPUNIT_ASSERT_EQUAL(string("123"), Value(123).as_string());
        CPPUNIT_ASSERT_EQUAL(string("a'sd"), Value("a'sd").as_string());
        CPPUNIT_ASSERT_EQUAL(string("123.45"), Value(decimal("0123.450")).as_string());
        boost::posix_time::ptime a(
                boost::gregorian::date(2006, 11, 16),
                boost::posix_time::time_duration(15, 5, 10));
        CPPUNIT_ASSERT_EQUAL(string("2006-11-16T15:05:10"), Value(a).as_string());
    }

    void test_value_is_null()
    {
        Value().as_long_long();
    }

    void test_value_bad_cast_long_long()
    {
        Value("ab").as_long_long();
    }

    void test_value_bad_cast_decimal()
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
