#include <iostream>
#include <iomanip>
#include <sstream>
#if !defined(YB_USE_QT) && !defined(YB_USE_WX)
#include <boost/lexical_cast.hpp>
#endif
#include <cppunit/extensions/HelperMacros.h>

#include "util/data_types.h"

using namespace std;
using namespace Yb;

class TestDecimal: public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestDecimal);

    CPPUNIT_TEST(testDecimalInit);
    CPPUNIT_TEST(testDecimalRounding);

    CPPUNIT_TEST_EXCEPTION(testDecimalInitFail0, DecimalOverflow);
    CPPUNIT_TEST_EXCEPTION(testDecimalInitFail1, DecimalException);
    CPPUNIT_TEST_EXCEPTION(testDecimalInitFail2, DecimalInvalidFormat);
    CPPUNIT_TEST_EXCEPTION(testDecimalInitFail3, DecimalInvalidFormat);
    CPPUNIT_TEST_EXCEPTION(testDecimalInitFail4, DecimalInvalidFormat);
    CPPUNIT_TEST_EXCEPTION(testDecimalInitFail5, DecimalInvalidFormat);
    CPPUNIT_TEST_EXCEPTION(testDecimalInitFail6, DecimalInvalidFormat);
    CPPUNIT_TEST_EXCEPTION(testDecimalInitFail7, DecimalInvalidFormat);
    CPPUNIT_TEST_EXCEPTION(testDecimalInitFail8, DecimalInvalidFormat);
    CPPUNIT_TEST_EXCEPTION(testDecimalInitFail9, DecimalInvalidFormat);
    CPPUNIT_TEST_EXCEPTION(testDecimalInitFail10, DecimalInvalidFormat);
    CPPUNIT_TEST_EXCEPTION(testDecimalInitFail11, DecimalInvalidFormat);
    CPPUNIT_TEST_EXCEPTION(testDecimalInitFail12, DecimalException);

    CPPUNIT_TEST(testDecimalComparison);
    CPPUNIT_TEST(testDecimalArithmeticMembers);
    CPPUNIT_TEST(testDecimalArithmetics);
    CPPUNIT_TEST_EXCEPTION(testDivByZero, DecimalDivByZero);
    CPPUNIT_TEST_EXCEPTION(testOverflow1, DecimalOverflow);
    CPPUNIT_TEST_EXCEPTION(testOverflow2, DecimalOverflow);
    CPPUNIT_TEST_EXCEPTION(testOverflow3, DecimalOverflow);

    CPPUNIT_TEST(testSerialization);
    CPPUNIT_TEST(testDeserialization);
    CPPUNIT_TEST(testFailedDeserialization);

#if !defined(YB_USE_QT) && !defined(YB_USE_WX)
    CPPUNIT_TEST(testCast);
    CPPUNIT_TEST_EXCEPTION(testInvalidCast1, boost::bad_lexical_cast);
    CPPUNIT_TEST_EXCEPTION(testInvalidCast2, boost::bad_lexical_cast);
#endif

    CPPUNIT_TEST_SUITE_END();

public:
    void testDecimalInit()
    {
        Decimal m0;
        Decimal m1(25);
        Decimal m2(10.20);
        Decimal m3(_T("-10.00"));
        Decimal m4((decimal_numerator)-3344, 2);
        Decimal m5(20, 1);

        Decimal m6(_T("1000000.11"));
        Decimal m7(_T("0"));
        Decimal m8(_T("-0"));
        Decimal m9(_T("0.75"));
        Decimal m10(_T(".15"));
        Decimal m11(_T("-.15"));
        Decimal m12(_T("+0"));

        Decimal x1(_T(" -999 999 999 999 999 999,"));
        Decimal x2(_T("+00.999999999999999999 "));

        CPPUNIT_ASSERT_EQUAL(false, m0.is_positive());
        CPPUNIT_ASSERT_EQUAL((decimal_numerator)0, m0.ipart());
        CPPUNIT_ASSERT_EQUAL((decimal_numerator)0, m0.fpart(2));
        CPPUNIT_ASSERT_EQUAL(true, m1.is_positive());
        CPPUNIT_ASSERT_EQUAL((decimal_numerator)25, m1.ipart());
        CPPUNIT_ASSERT_EQUAL((decimal_numerator)0, m1.fpart(2));
        CPPUNIT_ASSERT_EQUAL(true, m2.is_positive());
        CPPUNIT_ASSERT_EQUAL((decimal_numerator)10, m2.ipart());
        CPPUNIT_ASSERT_EQUAL((decimal_numerator)20, m2.fpart(2));
        CPPUNIT_ASSERT_EQUAL(false, m3.is_positive());
        CPPUNIT_ASSERT_EQUAL((decimal_numerator)10, m3.ipart());
        CPPUNIT_ASSERT_EQUAL((decimal_numerator)0, m3.fpart(1));
        CPPUNIT_ASSERT_EQUAL(false, m4.is_positive());
        CPPUNIT_ASSERT_EQUAL((decimal_numerator)33, m4.ipart());
        CPPUNIT_ASSERT_EQUAL((decimal_numerator)440, m4.fpart(3));
        CPPUNIT_ASSERT_EQUAL(true, m5.is_positive());
        CPPUNIT_ASSERT_EQUAL((decimal_numerator)2, m5.ipart());
        CPPUNIT_ASSERT_EQUAL((decimal_numerator)0, m5.fpart(3));
        CPPUNIT_ASSERT_EQUAL(MAX_DECIMAL_NUMERATOR, x1.ipart());
        CPPUNIT_ASSERT_EQUAL(MAX_DECIMAL_NUMERATOR, x2.fpart(18));

        CPPUNIT_ASSERT(Decimal((decimal_numerator)100000011, 2) == m6);
        CPPUNIT_ASSERT(Decimal() == m7);
        CPPUNIT_ASSERT(Decimal() == m8);
        CPPUNIT_ASSERT(Decimal((decimal_numerator)75, 2) == m9);
        CPPUNIT_ASSERT(Decimal((decimal_numerator)15, 2) == m10);
        CPPUNIT_ASSERT(Decimal((decimal_numerator)-15, 2) == m11);
        CPPUNIT_ASSERT(Decimal() == m12);
    }

    void testDecimalRounding()
    {
        Decimal m1(_T("100.999"));
        Decimal m2(_T("100.990"));
        Decimal m3(_T("100.991"));
        Decimal m4(_T("100.992"));
        Decimal m5(_T("100.993"));
        Decimal m6(_T("100.994"));
        Decimal m7(_T("100.995"));
        Decimal m8(_T("100.996"));
        Decimal m9(_T("100.997"));
        Decimal m10(_T("100.998"));

        CPPUNIT_ASSERT(Decimal(101) == m1.round(2));
        CPPUNIT_ASSERT(Decimal(_T("100.99")) == m2.round(2));
        CPPUNIT_ASSERT(Decimal(_T("100.99")) == m3.round(2));
        CPPUNIT_ASSERT(Decimal(_T("100.99")) == m4.round(2));
        CPPUNIT_ASSERT(Decimal(_T("100.99")) == m5.round(2));
        CPPUNIT_ASSERT(Decimal(_T("100.99")) == m6.round(2));
        CPPUNIT_ASSERT(Decimal(101) == m7.round(2));
        CPPUNIT_ASSERT(Decimal(101) == m8.round(2));
        CPPUNIT_ASSERT(Decimal(101) == m9.round(2));
        CPPUNIT_ASSERT(Decimal(101) == m10.round(2));
    }

    void testDecimalInitFail0() { Decimal m(_T("1000000000000000000")); (void)m.is_positive(); }
    void testDecimalInitFail1() { Decimal m(_T("1.999999999999999999")); (void)m.is_positive(); }
    void testDecimalInitFail2() { Decimal m(_T("-+10")); (void)m.is_positive(); }
    void testDecimalInitFail3() { Decimal m(_T("")); (void)m.is_positive(); }
    void testDecimalInitFail4() { Decimal m(_T(".")); (void)m.is_positive(); }
    void testDecimalInitFail5() { Decimal m(_T("10e3")); (void)m.is_positive(); }
    void testDecimalInitFail6() { Decimal m(_T("-11231220.-10")); (void)m.is_positive(); }
    void testDecimalInitFail7() { Decimal m(_T("-10.asdasd")); (void)m.is_positive(); }
    void testDecimalInitFail8() { Decimal m(_T("123123.sdfsdf")); (void)m.is_positive(); }
    void testDecimalInitFail9() { Decimal m(_T("abcdef.10")); (void)m.is_positive(); }
    void testDecimalInitFail10() { Decimal m(_T("-2356.11qedfwqef")); (void)m.is_positive(); }
    void testDecimalInitFail11() { Decimal m(_T("123.2356.11")); (void)m.is_positive(); }
    void testDecimalInitFail12() { Decimal m(_T(".")); (void)m.is_positive(); }

    void testDecimalComparison()
    {
        Decimal m1(25);
        Decimal m2(1020, 2);
        Decimal m3(_T("-10.00"));
        Decimal m4(_T("20.1"));
        Decimal m5(_T("20"));
        Decimal m6(_T("1000000.11"));
        Decimal m7(_T("10.20"));

        CPPUNIT_ASSERT(m1 == m1);
        CPPUNIT_ASSERT(m2 == m7);
        CPPUNIT_ASSERT(m2 != m6);
        CPPUNIT_ASSERT(m1 > m2);
        CPPUNIT_ASSERT(m1 > m3);
        CPPUNIT_ASSERT(m2 < m1);
        CPPUNIT_ASSERT(m3 < m4);
        CPPUNIT_ASSERT(m3 < m5);
    }

    void testDecimalArithmeticMembers()
    {
        Decimal m1(25);
        Decimal m2(1020, 2);
        Decimal m3(_T("-10.00"));
        Decimal m4(_T("20.1"));
        Decimal m5(_T("20"));
        Decimal m6(_T("1000000.11"));
        Decimal m7(_T("-4.2"));
        Decimal m8(_T("-2"));

        CPPUNIT_ASSERT((m1 += m2) == Decimal(_T("35.20")));
        CPPUNIT_ASSERT(m1 == Decimal(_T("35.20")));
        CPPUNIT_ASSERT(m2 == Decimal(1020, 2));

        CPPUNIT_ASSERT((m3 -= m4) == Decimal(_T("-30.10")));
        CPPUNIT_ASSERT(m3 == Decimal(_T("-30.10")));
        CPPUNIT_ASSERT(m4 == Decimal(_T("20.1")));

        CPPUNIT_ASSERT((m5 *= m6) == Decimal(_T("20000002.2")));
        CPPUNIT_ASSERT(m5 == Decimal(_T("20000002.2")));
        CPPUNIT_ASSERT(m6 == Decimal(_T("1000000.11")));

        CPPUNIT_ASSERT((m7 /= m8) == Decimal(_T("2.1")));
        CPPUNIT_ASSERT(m7 == Decimal(_T("2.1")));
        CPPUNIT_ASSERT(m8 == Decimal(-2));

        CPPUNIT_ASSERT(++m2 == Decimal(_T("11.2")));
        CPPUNIT_ASSERT(m2 == Decimal(_T("11.2")));

        CPPUNIT_ASSERT(m4++ == Decimal(_T("20.1")));
        CPPUNIT_ASSERT(m4 == Decimal(_T("21.1")));

        CPPUNIT_ASSERT(--m2 == Decimal(_T("10.2")));
        CPPUNIT_ASSERT(m2 == Decimal(_T("10.2")));

        CPPUNIT_ASSERT(m4-- == Decimal(_T("21.1")));
        CPPUNIT_ASSERT(m4 == Decimal(_T("20.1")));
    }

    void testDecimalArithmetics()
    {
        Decimal m1(25);
        Decimal m2(1020, 2);
        Decimal m3(_T("-10.00"));
        Decimal m4(_T("20.1"));
        Decimal m5(_T("20"));
        Decimal m6(_T("1000000.11"));
        Decimal m7(_T("1000000.11"));

        CPPUNIT_ASSERT(m1 + m2 == Decimal(_T("35.20")));
        CPPUNIT_ASSERT(m2 + m3 == Decimal(_T("0.20")));
        CPPUNIT_ASSERT(m3 + m4 == Decimal(_T("10.10")));
        CPPUNIT_ASSERT(m4 + m5 == Decimal(_T("40.1")));
        CPPUNIT_ASSERT(m5 + m6 == Decimal(_T("1000020.11")));
        CPPUNIT_ASSERT(m6 + m7 == Decimal(_T("2000000.22")));

        CPPUNIT_ASSERT(m1 - m2 == Decimal(_T("14.80")));
        CPPUNIT_ASSERT(m2 - m3 == Decimal(_T("20.20")));
        CPPUNIT_ASSERT(m3 - m4 == Decimal(_T("-30.10")));
        CPPUNIT_ASSERT(m4 - m5 == Decimal(_T("0.1")));
        CPPUNIT_ASSERT(m6 - m5 == Decimal(_T("999980.11")));
        CPPUNIT_ASSERT(m6 - m7 == Decimal(_T("0")));

        CPPUNIT_ASSERT_EQUAL(string("0.33333333333333333"), NARROW((Decimal(1)/3).str()));
        CPPUNIT_ASSERT_EQUAL(string("1.33333333333333333"), NARROW((Decimal(1)/3 + 1).str()));
        CPPUNIT_ASSERT_EQUAL(string("61649.35826"), NARROW((Decimal(178.234)*Decimal(345.89)).str()));
        CPPUNIT_ASSERT_EQUAL(string("-1936.61577608142"), NARROW((Decimal(_T("456.654")) / -Decimal(_T("0.2358"))).str()));
        CPPUNIT_ASSERT_EQUAL(string("0"), NARROW((Decimal(_T("34534.6")) * 0).str()));
        CPPUNIT_ASSERT_EQUAL(string("0"), NARROW((Decimal(_T("-0")) * Decimal(_T("34534.6"))).str()));
        CPPUNIT_ASSERT_EQUAL(string("0"), NARROW((Decimal(_T("-0")) / Decimal(_T("34534.6"))).str()));
        CPPUNIT_ASSERT_EQUAL(string("999999999999999999"), NARROW((Decimal(_T("-999999999999999999")) * -1).str()));
    }

    void testDivByZero()
    {
        Decimal x = Decimal(45)/(Decimal(4)-1-3);
    }

    void testOverflow1()
    {
        Decimal x = Decimal(1)/3 + 10;
    }

    void testOverflow2()
    {
        Decimal x = Decimal(_T("-999999999999999990")) - 10;
    }

    void testOverflow3()
    {
        Decimal x = Decimal(_T("123456789.123456789")) * 9;
    }

    void testSerialization()
    {
        Decimal m1(25);
        Decimal m2(1020, 2);
        Decimal m3(_T("-10.00"));
        Decimal m4(_T("0.999999999999999999"));
        Decimal m5(_T("20.123"));
        Decimal m6(_T("0.999999999999999999"));
        Decimal m7(_T("-1000000.11"));

        {
            std::ostringstream s;
            s << std::setprecision(2) << m1;
            CPPUNIT_ASSERT_EQUAL(string("25.00"), s.str());
        }
        {
            std::ostringstream s;
            s << setprecision(5) << setiosflags(ios_base::showpos) << m2;
            CPPUNIT_ASSERT_EQUAL(string("+10.20000"), s.str());
        }
        {
            std::ostringstream s;
            s << setprecision(0) << setfill('_') << setw(7) << m3;
            CPPUNIT_ASSERT_EQUAL(string("____-10"), s.str());
        }
        {
            std::ostringstream s;
            s << setprecision(23) << m4;
            CPPUNIT_ASSERT_EQUAL(string("0.99999999999999999900000"), s.str());
        }
        {
            std::ostringstream s;
            s << setw(3) << fixed << m5;
            CPPUNIT_ASSERT_EQUAL(string("20.123"), s.str());
        }
        {
            std::ostringstream s;
            s << setprecision(17) << m6;
            CPPUNIT_ASSERT_EQUAL(string("1.00000000000000000"), s.str());
        }
    }

    void testDeserialization()
    {
        std::stringstream s("1000000.11");
        Decimal d;
        s >> d;
        CPPUNIT_ASSERT(Decimal(_T("1000000.11")) == d);
    }

    void testFailedDeserialization()
    {
        std::stringstream s("qwelkfjwoefijw");
        std::string str;
        Decimal d;
        s >> d;
        s >> str;
        CPPUNIT_ASSERT_EQUAL(string(""), str);
    }

#if !defined(YB_USE_QT) && !defined(YB_USE_WX)
    void testCast()
    {
        Decimal r = boost::lexical_cast<Decimal>(_T("-32410.20")),
                d = boost::lexical_cast<Decimal>(
                        Yb::String(_T("10.23")));
        CPPUNIT_ASSERT(r == Decimal(_T("-32410.20")));
        CPPUNIT_ASSERT(d == Decimal(_T("10.23")));
    }

    void testInvalidCast1()
    {
        Decimal r = boost::lexical_cast<Decimal>(_T(""));
    }

    void testInvalidCast2()
    {
        Decimal r = boost::lexical_cast<Decimal>(_T("wefojhweofihwoe"));
    }
#endif
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestDecimal);

// vim:ts=4:sts=4:sw=4:et:
