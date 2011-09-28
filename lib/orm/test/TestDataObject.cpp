// -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
#include <util/str_utils.hpp>
#include <orm/DataObject.h>
#include <orm/XMLMetaDataConfig.h>

using namespace std;
using namespace Yb;
using Yb::StrUtils::xgetenv;

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
    CPPUNIT_TEST_EXCEPTION(test_data_object_delete_restricted,
                           CascadeDeleteError);
    CPPUNIT_TEST(test_traverse_down_up);
    CPPUNIT_TEST(test_traverse_up_down);
    CPPUNIT_TEST_SUITE_END();

    Schema r_;

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
        r_.fill_fkeys();
    }

    void test_data_object_key()
    {
        DataObject::Ptr d = DataObject::create_new(r_.get_table("A"));
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
        DataObject::Ptr d = DataObject::create_new(r_.get_table("A"));
        CPPUNIT_ASSERT(DataObject::New == d->status());
        CPPUNIT_ASSERT_EQUAL((SessionV2 *)NULL, d->session());
        SessionV2 session(r_);
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
        DataObject::Ptr d = DataObject::create_new(r_.get_table("A"));
        d->set("X", 10);
        CPPUNIT_ASSERT_EQUAL((int)DataObject::New, (int)d->status());
        CPPUNIT_ASSERT_EQUAL((SessionV2 *)NULL, d->session());
        SessionV2 session(r_);
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
        DataObject::Ptr d = DataObject::create_new(r_.get_table("A")),
            e = DataObject::create_new(r_.get_table("B"));
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
        DataObject::Ptr d = DataObject::create_new(r_.get_table("A")),
            e = DataObject::create_new(r_.get_table("B"));
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
        DataObject::Ptr d = DataObject::create_new(r_.get_table("A")),
            e = DataObject::create_new(r_.get_table("B"));
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
        DataObject::Ptr d = DataObject::create_new(r_.get_table("A")),
            e = DataObject::create_new(r_.get_table("B"));
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
        DataObject::Ptr d = DataObject::create_new(r_.get_table("A")),
            e = DataObject::create_new(r_.get_table("B"));
        e->link_to_master(d, "MasterA");
        d->delete_object();
    }

    void test_traverse_down_up()
    {
        DataObject::Ptr e = DataObject::create_new(r_.get_table("B"));
        e->set("X", 10);
        SessionV2 session(r_);
        session.save(e);
        DataObject::Ptr d = e->get_master("MasterA");
        DataObject::Ptr c = e->get_master();
        CPPUNIT_ASSERT_EQUAL(d.get(), c.get());
        CPPUNIT_ASSERT_EQUAL((int)DataObject::Ghost, (int)d->status());
    }    

    void test_traverse_up_down()
    {
        DataObject::Ptr d = DataObject::create_new(r_.get_table("A"));
        SessionV2 session(r_);
        session.save(d);
        RelationObject *ro = d->get_slaves();
        CPPUNIT_ASSERT_EQUAL((int)RelationObject::Incomplete,
                             (int)ro->status());
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestDataObject);


class TestDataObjectSaveLoad : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestDataObjectSaveLoad);
    CPPUNIT_TEST(test_lazy_load);
    CPPUNIT_TEST(test_lazy_load_slaves);
    CPPUNIT_TEST_SUITE_END();

    Schema r_;

public:
    void setUp()
    {
        tearDown();
        SqlConnect conn("ODBC", xgetenv("YBORM_DBTYPE"),
                        xgetenv("YBORM_DB"),
                        xgetenv("YBORM_USER"), xgetenv("YBORM_PASSWD"));
        {
            ostringstream sql;
            sql << "INSERT INTO T_ORM_TEST(ID, A, B, C) VALUES(?, ?, ?, ?)";
            string sql_str = sql.str();
            conn.prepare(sql_str);
            Values params(4);
            params[0] = Value(10);
            params[1] = Value("item");
            params[2] = Value(now());
            params[3] = Value(Decimal("1.2"));
            conn.exec(params);
        }
        {
            ostringstream sql;
            sql << "INSERT INTO T_ORM_XML(ID, ORM_TEST_ID, B) VALUES (?, ?, ?)";
            string sql_str = sql.str();
            conn.prepare(sql_str);
            Values params(3);
            params[0] = Value(20);
            params[1] = Value(10);
            params[2] = Value(Decimal("3.14"));
            conn.exec(params);
            params[0] = Value(30);
            params[1] = Value(10);
            params[2] = Value(Decimal("2.7"));
            conn.exec(params);
        }
        conn.commit();

        string xml = 
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
"<schema>"
"    <table name=\"T_ORM_TEST\" sequence=\"S_ORM_TEST_ID\""
"            class=\"OrmTest\" xml-name=\"orm-test\">"
"        <column name=\"ID\" type=\"longint\">"
"            <primary-key />"
"            <read-only />"
"        </column>"
"        <column name=\"A\" type=\"string\" size=\"200\" />"
"        <column name=\"B\" type=\"datetime\" default=\"sysdate\"/>"
"        <column name=\"C\" type=\"decimal\"/>"
"    </table>"
"    <table name=\"T_ORM_XML\" sequence=\"S_ORM_XML_ID\""
"            class=\"OrmXml\" xml-name=\"orm-xml\">"
"        <column name=\"ID\" type=\"longint\">"
"            <primary-key />"
"            <read-only />"
"        </column>"
"        <column name=\"ORM_TEST_ID\" type=\"longint\">"
"            <foreign-key table=\"T_ORM_TEST\" key=\"ID\"/>"
"        </column>"
"        <column name=\"B\" type=\"decimal\"/>"
"    </table>"
"    <relation type=\"one-to-many\">"
"        <one class=\"OrmTest\" />"
"        <many class=\"OrmXml\" property=\"orm_test\" />"
"    </relation>"
"</schema>";
        XMLMetaDataConfig cfg(xml);
        r_ = Schema();
        cfg.parse(r_);
        r_.fill_fkeys();
    }

    void tearDown()
    {
        SqlConnect conn("ODBC", xgetenv("YBORM_DBTYPE"),
                        xgetenv("YBORM_DB"),
                        xgetenv("YBORM_USER"), xgetenv("YBORM_PASSWD"));
        conn.exec_direct("DELETE FROM T_ORM_XML");
        conn.exec_direct("DELETE FROM T_ORM_TEST");
        conn.commit();
    }

    void test_lazy_load()
    {
        Engine engine(Engine::READ_ONLY);
        engine.get_connect()->set_echo(true);
        SessionV2 session(r_, &engine);
        DataObject::Ptr e = session.get_lazy
            (r_.get_table("T_ORM_XML").mk_key(20));
        CPPUNIT_ASSERT_EQUAL((int)DataObject::Ghost, (int)e->status());
        CPPUNIT_ASSERT(Decimal("3.14") == e->get("B").as_decimal());
        CPPUNIT_ASSERT_EQUAL((int)DataObject::Sync, (int)e->status());
        CPPUNIT_ASSERT_EQUAL((LongInt)10,
                             e->get("ORM_TEST_ID").as_longint());
        DataObject::Ptr d = e->get_master("orm_test");
        CPPUNIT_ASSERT_EQUAL((int)DataObject::Ghost, (int)d->status());
        CPPUNIT_ASSERT_EQUAL(string("item"), d->get("A").as_string());
        CPPUNIT_ASSERT_EQUAL((int)DataObject::Sync, (int)d->status());
        d->set("A", "meti");
        CPPUNIT_ASSERT_EQUAL(string("meti"), d->get("A").as_string());
        CPPUNIT_ASSERT_EQUAL((int)DataObject::Dirty, (int)d->status());
    }

    void test_lazy_load_slaves()
    {
        Engine engine(Engine::READ_ONLY);
        engine.get_connect()->set_echo(true);
        SessionV2 session(r_, &engine);
        DataObject::Ptr d = session.get_lazy
            (r_.get_table("T_ORM_TEST").mk_key(10));
        CPPUNIT_ASSERT_EQUAL((int)DataObject::Ghost, (int)d->status());
        RelationObject *ro = d->get_slaves();
        CPPUNIT_ASSERT_EQUAL((int)RelationObject::Incomplete, (int)ro->status());
        CPPUNIT_ASSERT_EQUAL((size_t)0, ro->slave_objects().size());
        CPPUNIT_ASSERT_EQUAL((size_t)2, ro->count_slaves());
        CPPUNIT_ASSERT_EQUAL((size_t)0, ro->slave_objects().size());
        ro->lazy_load_slaves();
        CPPUNIT_ASSERT_EQUAL((size_t)2, ro->slave_objects().size());
        CPPUNIT_ASSERT_EQUAL((int)RelationObject::Sync, (int)ro->status());
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestDataObjectSaveLoad);

// vim:ts=4:sts=4:sw=4:et:
