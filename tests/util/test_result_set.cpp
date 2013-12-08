#include <vector>
#include <cppunit/extensions/HelperMacros.h>
#include "util/result_set.h"

using namespace std;

typedef int Item;
typedef vector<Item> Items;

class MockResultSet: public Yb::ResultSetBase<Item> {
    Items::reverse_iterator start_, finish_;
    bool fetch(Item &item) {
        if (start_ == finish_)
            return false;
        item = *start_;
        ++start_;
        return true;
    }
public:
    MockResultSet(Items &items)
        : start_(items.rbegin()), finish_(items.rend())
    {}
};

class TestResultSet: public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestResultSet);

    CPPUNIT_TEST(testEnum);
    CPPUNIT_TEST(testCopy);
    CPPUNIT_TEST(testLimitedCopy2);
    CPPUNIT_TEST(testLimitedCopy0);
    CPPUNIT_TEST_EXCEPTION(testThrows, Yb::AssertError);

    CPPUNIT_TEST_SUITE_END();

public:
    void testEnum()
    {
        Items items(3);
        items[0] = 10; items[1] = 11; items[2] = 12;
        MockResultSet rs(items);
        MockResultSet::iterator it = rs.begin(), end = rs.end();
        CPPUNIT_ASSERT(end != it);
        CPPUNIT_ASSERT_EQUAL(12, *it);
        CPPUNIT_ASSERT_EQUAL(12, *it);
        CPPUNIT_ASSERT(it != end);
        CPPUNIT_ASSERT_EQUAL(11, *(++it));
        CPPUNIT_ASSERT_EQUAL(11, *it);
        CPPUNIT_ASSERT_EQUAL(11, *it++);
        CPPUNIT_ASSERT(end != it);
        CPPUNIT_ASSERT_EQUAL(10, *it);
        ++it;
        CPPUNIT_ASSERT(it == end);
    }

    void testCopy()
    {
        Items items(3), out;
        items[0] = 10; items[1] = 11; items[2] = 12;
        MockResultSet rs(items);
        Yb::copy_no_more_than_n(rs.begin(), rs.end(), -1,
                std::back_inserter(out));
        CPPUNIT_ASSERT_EQUAL((size_t)3, out.size());
        CPPUNIT_ASSERT_EQUAL(12, out[0]);
        CPPUNIT_ASSERT_EQUAL(11, out[1]);
        CPPUNIT_ASSERT_EQUAL(10, out[2]);
    }

    void testLimitedCopy2()
    {
        Items items(3), out;
        items[0] = 10; items[1] = 11; items[2] = 12;
        MockResultSet rs(items);
        Yb::copy_no_more_than_n(rs.begin(), rs.end(), 2,
                std::back_inserter(out));
        CPPUNIT_ASSERT_EQUAL((size_t)2, out.size());
        CPPUNIT_ASSERT_EQUAL(12, out[0]);
        CPPUNIT_ASSERT_EQUAL(11, out[1]);
    }

    void testLimitedCopy0()
    {
        Items items(3), out;
        items[0] = 10; items[1] = 11; items[2] = 12;
        MockResultSet rs(items);
        Yb::copy_no_more_than_n(rs.begin(), rs.end(), 0,
                std::back_inserter(out));
        CPPUNIT_ASSERT_EQUAL((size_t)0, out.size());
    }

    void testThrows()
    {
        Items items;
        MockResultSet rs(items);
        MockResultSet::iterator it = rs.begin(), end = rs.end();
        CPPUNIT_ASSERT(end == it);
        *it;
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestResultSet);

// vim:ts=4:sts=4:sw=4:et:
