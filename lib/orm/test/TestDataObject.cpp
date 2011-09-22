#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
#include <orm/DataObject.h>

using namespace std;
using namespace Yb;

class TestDataObject : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestDataObject);
    CPPUNIT_TEST(test_data_object_key);
    CPPUNIT_TEST(test_data_object_save_no_id);
    CPPUNIT_TEST(test_data_object_save_id);
    CPPUNIT_TEST_SUITE_END();

    Schema r_;
    const Schema &get_r() const { return r_; }

public:
    void setUp()
    {
        Table t("A");
        t.add_column(Column("X", Value::LONGINT, 0,
                    Column::PK));
        t.add_column(Column("Y", Value::STRING, 4, 0));
        t.set_seq_name("S_A_X");
        Schema r;
        r.set_table(t);
        r_ = r;
    }

    void test_data_object_key()
    {
        DataObject::Ptr d = DataObject::create_new(get_r().get_table("A"));
        ValuesMap values;
        values["X"] = Value();
        CPPUNIT_ASSERT(Key("A", values) == d->key());
        CPPUNIT_ASSERT(!d->assigned_key());
        d->set("X", Value(10));
        CPPUNIT_ASSERT(d->assigned_key());
        values["X"] = Value(10);
        CPPUNIT_ASSERT(Key("A", values) == d->key());
    }

    void test_data_object_save_no_id()
    {
        DataObject::Ptr d = DataObject::create_new(get_r().get_table("A"));
        CPPUNIT_ASSERT(DataObject::New == d->status());
        CPPUNIT_ASSERT_EQUAL((SessionV2 *)NULL, d->session());
        SessionV2 session(get_r());
        session.save(d);
        session.save(d);
        CPPUNIT_ASSERT_EQUAL(&session, d->session());
        session.detach(d);
        session.detach(d);
        CPPUNIT_ASSERT_EQUAL((SessionV2 *)NULL, d->session());
        CPPUNIT_ASSERT(DataObject::New == d->status());
    }

    void test_data_object_save_id()
    {
        DataObject::Ptr d = DataObject::create_new(get_r().get_table("A"));
        d->set("X", 10);

        CPPUNIT_ASSERT(DataObject::New == d->status());
        CPPUNIT_ASSERT_EQUAL((SessionV2 *)NULL, d->session());
        SessionV2 session(get_r());
        session.save(d);
        session.save(d);
        CPPUNIT_ASSERT_EQUAL(&session, d->session());
        ValuesMap values;
        values["X"] = Value(10);
        DataObject::Ptr e = session.get_lazy(Key("A", values));
        CPPUNIT_ASSERT_EQUAL(d.get(), e.get());
        values["X"] = Value(11);
        e = session.get_lazy(Key("A", values));
        CPPUNIT_ASSERT(d.get() != e.get());
        CPPUNIT_ASSERT(DataObject::Ghost == e->status());
        session.detach(d);
        session.detach(d);
        CPPUNIT_ASSERT_EQUAL((SessionV2 *)NULL, d->session());
        CPPUNIT_ASSERT(DataObject::New == d->status());
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestDataObject);

// vim:ts=4:sts=4:sw=4:et:
