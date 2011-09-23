// -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
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
    CPPUNIT_TEST(test_data_object_link);
    CPPUNIT_TEST(test_data_object_delete);
    CPPUNIT_TEST(test_data_object_delete_cascade);
    CPPUNIT_TEST(test_data_object_delete_set_null);
    CPPUNIT_TEST_EXCEPTION(test_data_object_delete_restricted, CascadeDeleteError);
    CPPUNIT_TEST_SUITE_END();

    Schema r_;
    const Schema &get_r() const { return r_; }

public:
    void setUp()
    {
        Schema r;
        r_ = r;
        Table t("A", "", "A");
        t.add_column(Column("X", Value::LONGINT, 0, Column::PK));
        t.add_column(Column("Y", Value::STRING, 4));
        t.set_seq_name("S_A_X");
        r_.set_table(t);
        Table u("B", "", "B");
        u.add_column(Column("Z", Value::LONGINT, 0, Column::PK));
        u.add_column(Column("X", Value::LONGINT, 0, 0, "A", "X"));
        u.add_column(Column("Q", Value::DECIMAL));
        u.set_seq_name("S_B_Z");
        r_.set_table(u);
        Relation::AttrMap attr_a, attr_b;
        attr_a["property"] = "SlaveBs";
        attr_b["property"] = "MasterA";
        Relation re(Relation::ONE2MANY, "A", attr_a, "B", attr_b,
                Relation::Restrict);
        r_.add_relation(re);
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
        CPPUNIT_ASSERT_EQUAL((int)DataObject::New, (int)d->status());
    }

    void test_data_object_save_id()
    {
        DataObject::Ptr d = DataObject::create_new(get_r().get_table("A"));
        d->set("X", 10);
        CPPUNIT_ASSERT_EQUAL((int)DataObject::New, (int)d->status());
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
        CPPUNIT_ASSERT_EQUAL((int)DataObject::Ghost, (int)e->status());
        session.detach(d);
        session.detach(d);
        CPPUNIT_ASSERT_EQUAL((SessionV2 *)NULL, d->session());
        CPPUNIT_ASSERT_EQUAL((int)DataObject::New, (int)d->status());
    }

    void test_data_object_link()
    {
        DataObject::Ptr d = DataObject::create_new(get_r().get_table("A")),
            e = DataObject::create_new(get_r().get_table("B"));
        CPPUNIT_ASSERT_EQUAL((size_t)0, d->master_relations().size());
        CPPUNIT_ASSERT_EQUAL((size_t)0, d->slave_relations().size());
        CPPUNIT_ASSERT_EQUAL((size_t)0, e->master_relations().size());
        CPPUNIT_ASSERT_EQUAL((size_t)0, e->slave_relations().size());
        e->link_to_master(d, "MasterA");
        CPPUNIT_ASSERT_EQUAL((size_t)1, d->master_relations().size());
        CPPUNIT_ASSERT_EQUAL((size_t)0, d->slave_relations().size());
        CPPUNIT_ASSERT_EQUAL((size_t)0, e->master_relations().size());
        CPPUNIT_ASSERT_EQUAL((size_t)1, e->slave_relations().size());
        RelationObject *ro = *e->slave_relations().begin();
        CPPUNIT_ASSERT_EQUAL((size_t)1, ro->slave_objects().size());
        CPPUNIT_ASSERT_EQUAL(d.get(), ro->master_object());
    }

    void test_data_object_delete()
    {
        DataObject::Ptr d = DataObject::create_new(get_r().get_table("A")),
            e = DataObject::create_new(get_r().get_table("B"));
        d->link_to_slave(e);
        CPPUNIT_ASSERT_EQUAL((int)DataObject::New, (int)d->status());
        CPPUNIT_ASSERT_EQUAL((int)DataObject::New, (int)e->status());
        RelationObject *ro = *e->slave_relations().begin();
        CPPUNIT_ASSERT_EQUAL((size_t)1, ro->slave_objects().size());
        e->delete_object();
        CPPUNIT_ASSERT_EQUAL((size_t)0, ro->slave_objects().size());
        CPPUNIT_ASSERT_EQUAL((int)DataObject::New, (int)d->status());
        CPPUNIT_ASSERT_EQUAL((int)DataObject::Deleted, (int)e->status());
    }

    void test_data_object_delete_cascade()
    {
        DataObject::Ptr d = DataObject::create_new(get_r().get_table("A")),
            e = DataObject::create_new(get_r().get_table("B"));
        d->link_to_slave(e, "SlaveBs");
        CPPUNIT_ASSERT_EQUAL((int)DataObject::New, (int)d->status());
        CPPUNIT_ASSERT_EQUAL((int)DataObject::New, (int)e->status());
        RelationObject *ro = *e->slave_relations().begin();
        CPPUNIT_ASSERT_EQUAL((size_t)1, ro->slave_objects().size());
        ((Relation &)ro->relation_info()).set_cascade(Relation::Delete);
        d->delete_object();
        CPPUNIT_ASSERT_EQUAL((int)DataObject::Deleted, (int)d->status());
        CPPUNIT_ASSERT_EQUAL((int)DataObject::Deleted, (int)e->status());
        //CPPUNIT_ASSERT_EQUAL((size_t)0, ro->slave_objects().size());
    }

    void test_data_object_delete_set_null()
    {
        DataObject::Ptr d = DataObject::create_new(get_r().get_table("A")),
            e = DataObject::create_new(get_r().get_table("B"));
        e->link_to_master(d);
        CPPUNIT_ASSERT_EQUAL((int)DataObject::New, (int)d->status());
        CPPUNIT_ASSERT_EQUAL((int)DataObject::New, (int)e->status());
        RelationObject *ro = *e->slave_relations().begin();
        CPPUNIT_ASSERT_EQUAL((size_t)1, ro->slave_objects().size());
        ((Relation &)ro->relation_info()).set_cascade(Relation::Nullify);
        d->delete_object();
        CPPUNIT_ASSERT_EQUAL((int)DataObject::Deleted, (int)d->status());
        CPPUNIT_ASSERT_EQUAL((int)DataObject::New, (int)e->status());
        //CPPUNIT_ASSERT_EQUAL((size_t)0, ro->slave_objects().size());
    }

    void test_data_object_delete_restricted()
    {
        DataObject::Ptr d = DataObject::create_new(get_r().get_table("A")),
            e = DataObject::create_new(get_r().get_table("B"));
        e->link_to_master(d, "MasterA");
        d->delete_object();
   }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestDataObject);

// vim:ts=4:sts=4:sw=4:et:
