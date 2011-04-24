
#include <stdexcept>
#include <cppunit/extensions/HelperMacros.h>
#include <util/Singleton.h>

using namespace std;

struct GoodHoldee
{
	int x_;
	GoodHoldee(): x_(1) {}
};

struct BadHoldee
{
	int x_;
	BadHoldee(): x_(2) { throw logic_error("Exception in constructor of BadHoldee"); }
};

typedef Yb::SingletonHolder<GoodHoldee> GoodSingleton;
typedef Yb::SingletonHolder<BadHoldee> BadSingleton;

class TestSingleton: public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(TestSingleton);

	CPPUNIT_TEST(testOk);
	CPPUNIT_TEST(testFail);

	CPPUNIT_TEST_SUITE_END();

public:
	void testOk()
	{
		CPPUNIT_ASSERT(!GoodSingleton::singleton.instance_.get());
		CPPUNIT_ASSERT_EQUAL(1, GoodSingleton::instance().x_);
		CPPUNIT_ASSERT(GoodSingleton::singleton.instance_.get());
	}

	void testFail()
	{
		CPPUNIT_ASSERT(!BadSingleton::singleton.instance_.get());
		try {
			BadSingleton::instance().x_;
			CPPUNIT_FAIL("Exception not thrown!");
		}
		catch (const logic_error &) {}
		CPPUNIT_ASSERT(!BadSingleton::singleton.instance_.get());
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestSingleton);

// vim:ts=4:sts=4:sw=4:noet
