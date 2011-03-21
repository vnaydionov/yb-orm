#include <iostream>
#include <iomanip>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <cppunit/extensions/HelperMacros.h>

#include <util/decimal.h>

using namespace std;

class TestDecimal: public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestDecimal);

	CPPUNIT_TEST(testDecimalInit);
	CPPUNIT_TEST(testDecimalRounding);

	CPPUNIT_TEST_EXCEPTION(testDecimalInitFail0, decimal::overflow);
	CPPUNIT_TEST_EXCEPTION(testDecimalInitFail1, decimal::exception);
	CPPUNIT_TEST_EXCEPTION(testDecimalInitFail2, decimal::invalid_format);
	CPPUNIT_TEST_EXCEPTION(testDecimalInitFail3, decimal::invalid_format);
	CPPUNIT_TEST_EXCEPTION(testDecimalInitFail4, decimal::invalid_format);
	CPPUNIT_TEST_EXCEPTION(testDecimalInitFail5, decimal::invalid_format);
	CPPUNIT_TEST_EXCEPTION(testDecimalInitFail6, decimal::invalid_format);
	CPPUNIT_TEST_EXCEPTION(testDecimalInitFail7, decimal::invalid_format);
	CPPUNIT_TEST_EXCEPTION(testDecimalInitFail8, decimal::invalid_format);
	CPPUNIT_TEST_EXCEPTION(testDecimalInitFail9, decimal::invalid_format);
	CPPUNIT_TEST_EXCEPTION(testDecimalInitFail10, decimal::invalid_format);
	CPPUNIT_TEST_EXCEPTION(testDecimalInitFail11, decimal::invalid_format);
	CPPUNIT_TEST_EXCEPTION(testDecimalInitFail12, decimal::exception);

	CPPUNIT_TEST(testDecimalComparison);
	CPPUNIT_TEST(testDecimalArithmeticMembers);
	CPPUNIT_TEST(testDecimalArithmetics);
	CPPUNIT_TEST_EXCEPTION(testDivByZero, decimal::divizion_by_zero);
	CPPUNIT_TEST_EXCEPTION(testOverflow1, decimal::overflow);
	CPPUNIT_TEST_EXCEPTION(testOverflow2, decimal::overflow);
	CPPUNIT_TEST_EXCEPTION(testOverflow3, decimal::overflow);

	CPPUNIT_TEST(testSerialization);
	CPPUNIT_TEST(testDeserialization);
	CPPUNIT_TEST(testFailedDeserialization);

	CPPUNIT_TEST(testCast);
	CPPUNIT_TEST_EXCEPTION(testInvalidCast1,boost::bad_lexical_cast);
	CPPUNIT_TEST_EXCEPTION(testInvalidCast2,boost::bad_lexical_cast);

	CPPUNIT_TEST_SUITE_END();	

public:
	void testDecimalInit()
	{
		decimal m0;
		decimal m1(25);
		decimal m2(10.20);
		decimal m3("-10.00");
		decimal m4(-3344LL, 2);
		decimal m5(20, 1);

		decimal m6("1000000.11");
		decimal m7("0");
		decimal m8("-0");
		decimal m9("0.75");
		decimal m10(".15");
		decimal m11("-.15");
		decimal m12("+0");

		decimal x1(" -999 999 999 999 999 999,");
		decimal x2("+00.999999999999999999 ");

		CPPUNIT_ASSERT_EQUAL(false, m0.is_positive());
		CPPUNIT_ASSERT_EQUAL(0LL, m0.ipart());
		CPPUNIT_ASSERT_EQUAL(0LL, m0.fpart(2));
		CPPUNIT_ASSERT_EQUAL(true, m1.is_positive());
		CPPUNIT_ASSERT_EQUAL(25LL, m1.ipart());
		CPPUNIT_ASSERT_EQUAL(0LL, m1.fpart(2));
		CPPUNIT_ASSERT_EQUAL(true, m2.is_positive());
		CPPUNIT_ASSERT_EQUAL(10LL, m2.ipart());
		CPPUNIT_ASSERT_EQUAL(20LL, m2.fpart(2));
		CPPUNIT_ASSERT_EQUAL(false, m3.is_positive());
		CPPUNIT_ASSERT_EQUAL(10LL, m3.ipart());
		CPPUNIT_ASSERT_EQUAL(0LL, m3.fpart(1));
		CPPUNIT_ASSERT_EQUAL(false, m4.is_positive());
		CPPUNIT_ASSERT_EQUAL(33LL, m4.ipart());
		CPPUNIT_ASSERT_EQUAL(440LL, m4.fpart(3));
		CPPUNIT_ASSERT_EQUAL(true, m5.is_positive());
		CPPUNIT_ASSERT_EQUAL(2LL, m5.ipart());
		CPPUNIT_ASSERT_EQUAL(0LL, m5.fpart(3));
		CPPUNIT_ASSERT_EQUAL(999999999999999999LL, x1.ipart());
		CPPUNIT_ASSERT_EQUAL(999999999999999999LL, x2.fpart(18));

		CPPUNIT_ASSERT(decimal(100000011LL, 2) == m6);
		CPPUNIT_ASSERT(decimal() == m7);
		CPPUNIT_ASSERT(decimal() == m8);
		CPPUNIT_ASSERT(decimal(75LL, 2) == m9);
		CPPUNIT_ASSERT(decimal(15LL, 2) == m10);
		CPPUNIT_ASSERT(decimal(-15LL, 2) == m11);
		CPPUNIT_ASSERT(decimal() == m12);
	}

	void testDecimalRounding()
	{
		decimal m1("100.999");
		decimal m2("100.990");
		decimal m3("100.991");
		decimal m4("100.992");
		decimal m5("100.993");
		decimal m6("100.994");
		decimal m7("100.995");
		decimal m8("100.996");
		decimal m9("100.997");
		decimal m10("100.998");

		CPPUNIT_ASSERT(decimal(101) == m1.round(2));
		CPPUNIT_ASSERT(decimal("100.99") == m2.round(2));
		CPPUNIT_ASSERT(decimal("100.99") == m3.round(2));
		CPPUNIT_ASSERT(decimal("100.99") == m4.round(2));
		CPPUNIT_ASSERT(decimal("100.99") == m5.round(2));
		CPPUNIT_ASSERT(decimal("100.99") == m6.round(2));
		CPPUNIT_ASSERT(decimal(101) == m7.round(2));
		CPPUNIT_ASSERT(decimal(101) == m8.round(2));
		CPPUNIT_ASSERT(decimal(101) == m9.round(2));
		CPPUNIT_ASSERT(decimal(101) == m10.round(2));
	}

	void testDecimalInitFail0() { decimal m("1000000000000000000"); (void)m.is_positive(); }
	void testDecimalInitFail1() { decimal m("1.999999999999999999"); (void)m.is_positive(); }
	void testDecimalInitFail2() { decimal m("-+10"); (void)m.is_positive(); }
	void testDecimalInitFail3() { decimal m(""); (void)m.is_positive(); }
	void testDecimalInitFail4() { decimal m("."); (void)m.is_positive(); }
	void testDecimalInitFail5() { decimal m("10e3"); (void)m.is_positive(); }
	void testDecimalInitFail6() { decimal m("-11231220.-10"); (void)m.is_positive(); }
	void testDecimalInitFail7() { decimal m("-10.asdasd"); (void)m.is_positive(); }
	void testDecimalInitFail8() { decimal m("123123.sdfsdf"); (void)m.is_positive(); }
	void testDecimalInitFail9() { decimal m("abcdef.10"); (void)m.is_positive(); }
	void testDecimalInitFail10() { decimal m("-2356.11qedfwqef"); (void)m.is_positive(); }
	void testDecimalInitFail11() { decimal m("123.2356.11"); (void)m.is_positive(); }
	void testDecimalInitFail12() { decimal m("."); (void)m.is_positive(); }

	void testDecimalComparison() 
	{
		decimal m1(25);
		decimal m2(1020, 2);
		decimal m3("-10.00");
		decimal m4("20.1");
		decimal m5("20");
		decimal m6("1000000.11");
		decimal m7("10.20");

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
		decimal m1(25);
		decimal m2(1020, 2);
		decimal m3("-10.00");
		decimal m4("20.1");
		decimal m5("20");
		decimal m6("1000000.11");
		decimal m7("-4.2");
		decimal m8("-2");

		CPPUNIT_ASSERT((m1 += m2) == decimal("35.20"));
		CPPUNIT_ASSERT(m1 == decimal("35.20"));
		CPPUNIT_ASSERT(m2 == decimal(1020, 2));

		CPPUNIT_ASSERT((m3 -= m4) == decimal("-30.10"));
		CPPUNIT_ASSERT(m3 == decimal("-30.10"));
		CPPUNIT_ASSERT(m4 == decimal("20.1"));

		CPPUNIT_ASSERT((m5 *= m6) == decimal("20000002.2"));
		CPPUNIT_ASSERT(m5 == decimal("20000002.2"));
		CPPUNIT_ASSERT(m6 == decimal("1000000.11"));

		CPPUNIT_ASSERT((m7 /= m8) == decimal("2.1"));
		CPPUNIT_ASSERT(m7 == decimal("2.1"));
		CPPUNIT_ASSERT(m8 == decimal(-2));

		CPPUNIT_ASSERT(++m2 == decimal("11.2"));
		CPPUNIT_ASSERT(m2 == decimal("11.2"));

		CPPUNIT_ASSERT(m4++ == decimal("20.1"));
		CPPUNIT_ASSERT(m4 == decimal("21.1"));

		CPPUNIT_ASSERT(--m2 == decimal("10.2"));
		CPPUNIT_ASSERT(m2 == decimal("10.2"));

		CPPUNIT_ASSERT(m4-- == decimal("21.1"));
		CPPUNIT_ASSERT(m4 == decimal("20.1"));
	}

	void testDecimalArithmetics() 
	{
		decimal m1(25);
		decimal m2(1020, 2);
		decimal m3("-10.00");
		decimal m4("20.1");
		decimal m5("20");
		decimal m6("1000000.11");
		decimal m7("1000000.11");

		CPPUNIT_ASSERT(m1 + m2 == decimal("35.20"));
		CPPUNIT_ASSERT(m2 + m3 == decimal("0.20"));
		CPPUNIT_ASSERT(m3 + m4 == decimal("10.10"));
		CPPUNIT_ASSERT(m4 + m5 == decimal("40.1"));
		CPPUNIT_ASSERT(m5 + m6 == decimal("1000020.11"));
		CPPUNIT_ASSERT(m6 + m7 == decimal("2000000.22"));

		CPPUNIT_ASSERT(m1 - m2 == decimal("14.80"));
		CPPUNIT_ASSERT(m2 - m3 == decimal("20.20"));
		CPPUNIT_ASSERT(m3 - m4 == decimal("-30.10"));
		CPPUNIT_ASSERT(m4 - m5 == decimal("0.1"));
		CPPUNIT_ASSERT(m6 - m5 == decimal("999980.11"));
		CPPUNIT_ASSERT(m6 - m7 == decimal("0"));

		CPPUNIT_ASSERT_EQUAL(string("0.33333333333333333"), (decimal(1)/3).str());
		CPPUNIT_ASSERT_EQUAL(string("1.33333333333333333"), (decimal(1)/3 + 1).str());
		CPPUNIT_ASSERT_EQUAL(string("61649.35826"), (decimal(178.234)*decimal(345.89)).str());
		CPPUNIT_ASSERT_EQUAL(string("-1936.61577608142"), (decimal("456.654") / -decimal("0.2358")).str());
		CPPUNIT_ASSERT_EQUAL(string("0"), (decimal("34534.6") * 0).str());
		CPPUNIT_ASSERT_EQUAL(string("0"), (decimal("-0") * decimal("34534.6")).str());
		CPPUNIT_ASSERT_EQUAL(string("0"), (decimal("-0") / decimal("34534.6")).str());
		CPPUNIT_ASSERT_EQUAL(string("999999999999999999"), (decimal("-999999999999999999") * -1).str());
	}

	void testDivByZero()
	{
		decimal x = decimal(45)/(decimal(4)-1-3);
	}

	void testOverflow1()
	{
		decimal x = decimal(1)/3 + 10;
	}

	void testOverflow2()
	{
		decimal x = decimal("-999999999999999990") - 10;
	}

	void testOverflow3()
	{
		decimal x = decimal("123456789.123456789") * 9;
	}

	void testSerialization() 
	{
		decimal m1(25);
		decimal m2(1020, 2);
		decimal m3("-10.00");
		decimal m4("0.999999999999999999");
		decimal m5("20.123");
		decimal m6("0.999999999999999999");
		decimal m7("-1000000.11");

		{
			ostringstream s;
			s << setprecision(2) << m1;
			CPPUNIT_ASSERT_EQUAL(string("25.00"), s.str());
		}
		{
			ostringstream s;
			s << setprecision(5) << setiosflags(ios_base::showpos) << m2;
			CPPUNIT_ASSERT_EQUAL(string("+10.20000"), s.str());
		}
		{
			ostringstream s;
			s << setprecision(0) << setfill('_') << setw(7) << m3;
			CPPUNIT_ASSERT_EQUAL(string("____-10"), s.str());
		}
		{
			ostringstream s;
			s << setprecision(23) << m4;
			CPPUNIT_ASSERT_EQUAL(string("0.99999999999999999900000"), s.str());
		}
		{
			ostringstream s;
			s << setw(3) << fixed << m5;
			CPPUNIT_ASSERT_EQUAL(string("20.123"), s.str());
		}
		{
			ostringstream s;
			s << setprecision(17) << m6;
			CPPUNIT_ASSERT_EQUAL(string("1.00000000000000000"), s.str());
		}
	}

	void testDeserialization() 
	{
		stringstream s("1000000.11");
		decimal d;
		s >> d;
		CPPUNIT_ASSERT_EQUAL(decimal("1000000.11"), d);
	}

	void testFailedDeserialization() 
	{
		stringstream s("qwelkfjwoefijw");
		string str;
		decimal d;
		s >> d;
		s >> str;
		CPPUNIT_ASSERT_EQUAL(string(""), str);
	}

	void testCast() 
	{
		decimal r = boost::lexical_cast<decimal>(string("-32410.20"));
		decimal d = boost::lexical_cast<decimal>(string("10.23"));
		CPPUNIT_ASSERT(r == decimal("-32410.20"));
		CPPUNIT_ASSERT(d == decimal("10.23"));
	}

	void testInvalidCast1() 
	{
		decimal r = boost::lexical_cast<decimal>(string(""));
	}

	void testInvalidCast2() 
	{
		decimal r = boost::lexical_cast<decimal>(string("wefojhweofihwoe"));
	}

};

CPPUNIT_TEST_SUITE_REGISTRATION(TestDecimal);

// vim:ts=4:sts=4:sw=4:noet

