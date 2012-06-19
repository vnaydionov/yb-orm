// -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
#include <util/str_utils.hpp>
#include <orm/DataObject.h>
#include <orm/DomainObj.h>
#include <orm/XMLMetaDataConfig.h>

using namespace std;
using namespace Yb;
using Yb::StrUtils::xgetenv;

static LogAppender appender(cerr);
#define SETUP_LOG(e) do{ e.set_echo(true); \
    ILogger::Ptr __log(new Logger(&appender)); \
    e.set_logger(__log); }while(0)

class TestDataObject : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestDataObject);
    CPPUNIT_TEST_EXCEPTION(test_init_wrong_val, BadTypeCast);
    CPPUNIT_TEST_EXCEPTION(test_init_ro, ReadOnlyColumn);
    CPPUNIT_TEST_EXCEPTION(test_init_long_string, StringTooLong);
    CPPUNIT_TEST(test_data_object_key);
    CPPUNIT_TEST(test_data_object_save_no_id);
    CPPUNIT_TEST(test_data_object_save_id);
    CPPUNIT_TEST_EXCEPTION(test_data_object_already_saved,
                           DataObjectAlreadyInSession);
    CPPUNIT_TEST(test_save_or_update);
    CPPUNIT_TEST_EXCEPTION(test_data_object_cant_change_key_if_saved,
                           ReadOnlyColumn);
    CPPUNIT_TEST(test_data_object_link);
    CPPUNIT_TEST(test_data_object_relink);
    CPPUNIT_TEST(test_data_object_delete);
    CPPUNIT_TEST(test_data_object_delete_cascade);
    CPPUNIT_TEST(test_data_object_delete_set_null);
    CPPUNIT_TEST_EXCEPTION(test_data_object_delete_restricted,
                           CascadeDeleteError);
    CPPUNIT_TEST(test_traverse_down_up);
    CPPUNIT_TEST(test_traverse_up_down);
    CPPUNIT_TEST(test_calc_depth);
    CPPUNIT_TEST_EXCEPTION(test_cycle_detected, CycleDetected);
    CPPUNIT_TEST(test_filter_by_key);
    CPPUNIT_TEST(test_bad_type_cast_format);
    CPPUNIT_TEST_SUITE_END();

    Schema r_;

public:
    void setUp()
    {
        Schema r;
        r_ = r;
        Table::Ptr t(new Table(_T("A"), _T(""), _T("A")));
        t->add_column(Column(_T("X"), Value::LONGINT, 0, Column::PK));
        t->add_column(Column(_T("Y"), Value::STRING, 4));
        t->add_column(Column(_T("P"), Value::LONGINT, 0, 0, _T("A"), _T("X")));
        t->add_column(Column(_T("R"), Value::LONGINT, 0, Column::RO));
        t->set_seq_name(_T("S_A_X"));
        r_.add_table(t);
        Table::Ptr u(new Table(_T("B"), _T(""), _T("B")));
        u->add_column(Column(_T("Z"), Value::LONGINT, 0, Column::PK));
        u->add_column(Column(_T("X"), Value::LONGINT, 0, 0, _T("A"), _T("X")));
        u->add_column(Column(_T("Q"), Value::DECIMAL));
        u->set_seq_name(_T("S_B_Z"));
        r_.add_table(u);
        {
            Relation::AttrMap attr_a, attr_b;
            attr_a[_T("property")] = _T("SlaveBs");
            attr_b[_T("property")] = _T("MasterA");
            Relation::Ptr re(new Relation(Relation::ONE2MANY,
                        _T("A"), attr_a, _T("B"), attr_b,
                        Relation::Restrict));
            r_.add_relation(re);
        }
        {
            Relation::AttrMap attr_a, attr_b;
            attr_a[_T("property")] = _T("ChildAs");
            attr_b[_T("property")] = _T("ParA");
            Relation::Ptr re(new Relation(Relation::ONE2MANY,
                        _T("A"), attr_a, _T("A"), attr_b,
                        Relation::Nullify));
            r_.add_relation(re);
        }
        r_.fill_fkeys();
    }

    void test_init_wrong_val()
    {
        DataObject::Ptr d = DataObject::create_new(r_.table(_T("A")));
        d->set(_T("X"), Decimal(1.2));
    }

    void test_init_ro()
    {
        DataObject::Ptr d = DataObject::create_new(r_.table(_T("A")));
        d->set(_T("R"), 123);
    }

    void test_init_long_string()
    {
        DataObject::Ptr d = DataObject::create_new(r_.table(_T("A")));
        d->set(_T("Y"), String(_T("zrzzz")));
    }

    void test_data_object_key()
    {
        DataObject::Ptr d = DataObject::create_new(r_.table(_T("A")));
        ValueMap values;
        values[_T("X")] = Value();
        CPPUNIT_ASSERT(Key(_T("A"), values) == d->key());
        CPPUNIT_ASSERT(!d->assigned_key());
        d->set(_T("X"), Value(10));
        CPPUNIT_ASSERT(d->assigned_key());
        values[_T("X")] = Value(10);
        CPPUNIT_ASSERT(Key(_T("A"), values) == d->key());
    }

    void test_data_object_save_no_id()
    {
        DataObject::Ptr d = DataObject::create_new(r_.table(_T("A")));
        CPPUNIT_ASSERT(DataObject::New == d->status());
        CPPUNIT_ASSERT_EQUAL((Session *)NULL, d->session());
        Session session(r_);
        session.save(d);
        session.save(d);
        CPPUNIT_ASSERT_EQUAL(&session, d->session());
        session.detach(d);
        CPPUNIT_ASSERT_EQUAL((Session *)NULL, d->session());
        CPPUNIT_ASSERT_EQUAL((int)DataObject::New, (int)d->status());
    }

    void test_data_object_save_id()
    {
        DataObject::Ptr d = DataObject::create_new(r_.table(_T("A")));
        d->set(_T("X"), 10);
        CPPUNIT_ASSERT_EQUAL((int)DataObject::New, (int)d->status());
        CPPUNIT_ASSERT_EQUAL((Session *)NULL, d->session());
        Session session(r_);
        session.save(d);
        CPPUNIT_ASSERT_EQUAL(&session, d->session());
        ValueMap values;
        values[_T("X")] = Value(10);
        DataObject::Ptr e = session.get_lazy(Key(_T("A"), values));
        CPPUNIT_ASSERT_EQUAL(shptr_get(d), shptr_get(e));
        values[_T("X")] = Value(11);
        e = session.get_lazy(Key(_T("A"), values));
        CPPUNIT_ASSERT(shptr_get(d) != shptr_get(e));
        CPPUNIT_ASSERT_EQUAL((int)DataObject::Ghost, (int)e->status());
        session.detach(d);
        CPPUNIT_ASSERT_EQUAL((Session *)NULL, d->session());
        CPPUNIT_ASSERT_EQUAL((int)DataObject::New, (int)d->status());
    }

    void test_data_object_already_saved()
    {
        DataObject::Ptr d = DataObject::create_new(r_.table(_T("A")));
        d->set(_T("X"), 10);
        Session session(r_);
        session.save(d);
        session.save(d);
    }

    void test_save_or_update()
    {
        DataObject::Ptr d = DataObject::create_new(r_.table(_T("A")));
        d->set(_T("X"), 10);
        d->set(_T("Y"), String(_T("abc")));
        Session session(r_);
        session.save(d);
        DataObject::Ptr e = DataObject::create_new(r_.table(_T("A")));
        e->set(_T("X"), 10);
        e->set(_T("Y"), String(_T("xyz")));
        DataObject::Ptr f = session.save_or_update(e);
        CPPUNIT_ASSERT(shptr_get(d) == shptr_get(f));
        CPPUNIT_ASSERT_EQUAL(string("xyz"), NARROW(f->get(_T("Y")).as_string()));
    }

    void test_data_object_cant_change_key_if_saved()
    {
        DataObject::Ptr d = DataObject::create_new(r_.table(_T("A")));
        d->set(_T("X"), 10);
        d->set(_T("Y"), String(_T("abc")));
        Session session(r_);
        session.save(d);
        d->set(_T("X"), 100);
    }

    void test_data_object_link()
    {
        DataObject::Ptr d = DataObject::create_new(r_.table(_T("A"))),
            e = DataObject::create_new(r_.table(_T("B")));
        CPPUNIT_ASSERT_EQUAL((size_t)0, d->master_relations().size());
        CPPUNIT_ASSERT_EQUAL((size_t)0, d->slave_relations().size());
        CPPUNIT_ASSERT_EQUAL((size_t)0, e->master_relations().size());
        CPPUNIT_ASSERT_EQUAL((size_t)0, e->slave_relations().size());
        DataObject::link_slave_to_master(e, d, _T("MasterA"));
        CPPUNIT_ASSERT_EQUAL((size_t)1, d->master_relations().size());
        CPPUNIT_ASSERT_EQUAL((size_t)0, d->slave_relations().size());
        CPPUNIT_ASSERT_EQUAL((size_t)0, e->master_relations().size());
        CPPUNIT_ASSERT_EQUAL((size_t)1, e->slave_relations().size());
        RelationObject *ro = *e->slave_relations().begin();
        CPPUNIT_ASSERT_EQUAL((size_t)1, ro->slave_objects().size());
        CPPUNIT_ASSERT_EQUAL(shptr_get(d), ro->master_object());
    }

    void test_data_object_relink()
    {
        DataObject::Ptr d = DataObject::create_new(r_.table(_T("A"))),
            e = DataObject::create_new(r_.table(_T("B")));
        CPPUNIT_ASSERT_EQUAL((size_t)0, d->master_relations().size());
        CPPUNIT_ASSERT_EQUAL((size_t)0, d->slave_relations().size());
        CPPUNIT_ASSERT_EQUAL((size_t)0, e->master_relations().size());
        CPPUNIT_ASSERT_EQUAL((size_t)0, e->slave_relations().size());
        DataObject::link_slave_to_master(e, d, _T("MasterA"));
        CPPUNIT_ASSERT_EQUAL((size_t)1, d->master_relations().size());
        CPPUNIT_ASSERT_EQUAL((size_t)0, d->slave_relations().size());
        CPPUNIT_ASSERT_EQUAL((size_t)0, e->master_relations().size());
        CPPUNIT_ASSERT_EQUAL((size_t)1, e->slave_relations().size());
        RelationObject *ro = *e->slave_relations().begin();
        CPPUNIT_ASSERT_EQUAL((size_t)1, ro->slave_objects().size());
        CPPUNIT_ASSERT_EQUAL(shptr_get(d), ro->master_object());
        DataObject::Ptr f = DataObject::create_new(r_.table(_T("A")));
        DataObject::link_slave_to_master(e, f, _T("MasterA"));
        CPPUNIT_ASSERT_EQUAL((size_t)1, d->master_relations().size());
        CPPUNIT_ASSERT_EQUAL((size_t)0, d->slave_relations().size());
        CPPUNIT_ASSERT_EQUAL((size_t)0, e->master_relations().size());
        CPPUNIT_ASSERT_EQUAL((size_t)1, e->slave_relations().size());
        CPPUNIT_ASSERT_EQUAL((size_t)0, ro->slave_objects().size());
        ro = *e->slave_relations().begin();
        CPPUNIT_ASSERT_EQUAL((size_t)1, ro->slave_objects().size());
        CPPUNIT_ASSERT_EQUAL(shptr_get(f), ro->master_object());
    }

    void test_data_object_delete()
    {
        DataObject::Ptr d = DataObject::create_new(r_.table(_T("A"))),
            e = DataObject::create_new(r_.table(_T("B")));
        DataObject::link_master_to_slave(d, e);
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
        DataObject::Ptr d = DataObject::create_new(r_.table(_T("A"))),
            e = DataObject::create_new(r_.table(_T("B")));
        DataObject::link_master_to_slave(d, e, _T("SlaveBs"));
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
        DataObject::Ptr d = DataObject::create_new(r_.table(_T("A"))),
            e = DataObject::create_new(r_.table(_T("B")));
        DataObject::link_slave_to_master(e, d);
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
        DataObject::Ptr d = DataObject::create_new(r_.table(_T("A"))),
            e = DataObject::create_new(r_.table(_T("B")));
        DataObject::link_slave_to_master(e, d, _T("MasterA"));
        d->delete_object();
    }

    void test_traverse_down_up()
    {
        DataObject::Ptr e = DataObject::create_new(r_.table(_T("B")));
        e->set(_T("X"), 10);
        Session session(r_);
        session.save(e);
        DataObject::Ptr d = DataObject::get_master(e, _T("MasterA"));
        DataObject::Ptr c = DataObject::get_master(e);
        CPPUNIT_ASSERT_EQUAL(shptr_get(d), shptr_get(c));
        CPPUNIT_ASSERT_EQUAL((int)DataObject::Ghost, (int)d->status());
    }    

    void test_traverse_up_down()
    {
        DataObject::Ptr d = DataObject::create_new(r_.table(_T("A")));
        Session session(r_);
        session.save(d);
        RelationObject *ro = d->get_slaves(_T("SlaveBs"));
        CPPUNIT_ASSERT_EQUAL((int)RelationObject::Incomplete,
                             (int)ro->status());
    }

    void test_calc_depth()
    {
        DataObject::Ptr d = DataObject::create_new(r_.table(_T("A"))),
            e = DataObject::create_new(r_.table(_T("A"))),
            f = DataObject::create_new(r_.table(_T("A")));
        DataObject::link_slave_to_master(e, d, _T("ParA"));
        DataObject::link_slave_to_master(f, e, _T("ParA"));
        CPPUNIT_ASSERT_EQUAL(0, d->depth());
        CPPUNIT_ASSERT_EQUAL(1, e->depth());
        CPPUNIT_ASSERT_EQUAL(2, f->depth());
    }

    void test_cycle_detected()
    {
        DataObject::Ptr d = DataObject::create_new(r_.table(_T("A"))),
            e = DataObject::create_new(r_.table(_T("A"))),
            f = DataObject::create_new(r_.table(_T("A")));
        DataObject::link_slave_to_master(e, d, _T("ParA"));
        DataObject::link_slave_to_master(f, e, _T("ParA"));
        DataObject::link_slave_to_master(d, f, _T("ParA"));
    }

    void test_filter_by_key()
    {
        ValueMap values;
        values[_T("X")] = Value(10);
        Session session(r_);
        DataObject::Ptr d = session.get_lazy(Key(_T("A"), values));
        KeyFilter kf(d->key());
        CPPUNIT_ASSERT_EQUAL(string("1=1 AND X = 10"), NARROW(kf.get_sql()));
    }

    void test_bad_type_cast_format()
    {
        try {
            DataObject::Ptr d = DataObject::create_new(r_.table(_T("A")));
            const Column &c = r_.table(_T("A")).get_column(_T("P"));
            Value x; // !! this is required for bcc code not to fall !!
            d->get_typed_value(c, Value(_T("100"))); // should be OK
            d->get_typed_value(c, Value(_T("#")));
            CPPUNIT_FAIL("Exception BadTypeCast not thrown!");
        }
        catch (const BadTypeCast &e) {
            string err = e.what();
            size_t pos = err.find("\nBacktrace");
            if (pos != string::npos)
                err = string(err, 0, pos);
            CPPUNIT_ASSERT_EQUAL(
                string("Can't cast field A.P = \"#\" to type LongInt"), err);
        }
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestDataObject);


class TestDataObjectSaveLoad : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestDataObjectSaveLoad);
    CPPUNIT_TEST(test_lazy_load);
    CPPUNIT_TEST_EXCEPTION(test_lazy_load_fail, NullPK);
    CPPUNIT_TEST(test_lazy_load_slaves);
    CPPUNIT_TEST(test_flush_dirty);
    CPPUNIT_TEST(test_flush_new);
    CPPUNIT_TEST(test_flush_new_with_id);
    CPPUNIT_TEST(test_flush_new_linked);
    CPPUNIT_TEST(test_flush_new_linked_to_existing);
    CPPUNIT_TEST(test_flush_deleted);
    CPPUNIT_TEST(test_domain_object);
    CPPUNIT_TEST_SUITE_END();

    Schema r_;

public:
    void setUp()
    {
        tearDown();
        SqlConnection conn(_T("DEFAULT"), xgetenv(_T("YBORM_DBTYPE")),
                        xgetenv(_T("YBORM_DB")),
                        xgetenv(_T("YBORM_USER")), xgetenv(_T("YBORM_PASSWD")));
        {
            String sql_str =
                _T("INSERT INTO T_ORM_TEST(ID, A, B, C) VALUES(?, ?, ?, ?)");
            conn.prepare(sql_str);
            Values params(4);
            params[0] = Value(-10);
            params[1] = Value(_T("item"));
            params[2] = Value(now());
            params[3] = Value(Decimal(_T("1.2")));
            conn.exec(params);
        }
        {
            String sql_str =
                _T("INSERT INTO T_ORM_XML(ID, ORM_TEST_ID, B) VALUES (?, ?, ?)");
            conn.prepare(sql_str);
            Values params(3);
            params[0] = Value(-20);
            params[1] = Value(-10);
            params[2] = Value(Decimal(_T("3.14")));
            conn.exec(params);
            params[0] = Value(-30);
            params[1] = Value(-10);
            params[2] = Value(Decimal(_T("2.7")));
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
        Schema r0;
        r_ = r0;
        cfg.parse(r_);
        r_.fill_fkeys();
    }

    void tearDown()
    {
        SqlConnection conn(_T("DEFAULT"), xgetenv(_T("YBORM_DBTYPE")),
                        xgetenv(_T("YBORM_DB")),
                        xgetenv(_T("YBORM_USER")), xgetenv(_T("YBORM_PASSWD")));
        conn.exec_direct(_T("DELETE FROM T_ORM_XML"));
        conn.exec_direct(_T("DELETE FROM T_ORM_TEST"));
        conn.commit();
    }

    void test_lazy_load()
    {
        Engine engine(Engine::READ_ONLY);
        SETUP_LOG(engine);
        Session session(r_, &engine);
        DataObject::Ptr e = session.get_lazy
            (r_.table(_T("T_ORM_XML")).mk_key(-20));
        CPPUNIT_ASSERT_EQUAL((int)DataObject::Ghost, (int)e->status());
        CPPUNIT_ASSERT(Decimal(_T("3.14")) == e->get(_T("B")).as_decimal());
        CPPUNIT_ASSERT_EQUAL((int)DataObject::Sync, (int)e->status());
        CPPUNIT_ASSERT_EQUAL((LongInt)-10,
                             e->get(_T("ORM_TEST_ID")).as_longint());
        DataObject::Ptr d = DataObject::get_master(e, _T("orm_test"));
        CPPUNIT_ASSERT_EQUAL((int)DataObject::Ghost, (int)d->status());
        CPPUNIT_ASSERT_EQUAL(string("item"), NARROW(d->get(_T("A")).as_string()));
        CPPUNIT_ASSERT_EQUAL((int)DataObject::Sync, (int)d->status());
        d->set(_T("A"), String(_T("meti")));
        CPPUNIT_ASSERT_EQUAL(string("meti"), NARROW(d->get(_T("A")).as_string()));
        CPPUNIT_ASSERT_EQUAL((int)DataObject::Dirty, (int)d->status());
    }

    void test_lazy_load_fail()
    {
        Key k;
        {
            Engine engine;
            SETUP_LOG(engine);
            Session session(r_, &engine);
            DataObject::Ptr e = DataObject::create_new(r_.table(_T("T_ORM_XML")));
            e->set(_T("B"), Value(Decimal(_T("0.01"))));
            session.save(e);
            session.flush();
            k = e->key();
            engine.commit();
        }
        {
            Engine engine;
            SETUP_LOG(engine);
            Session session(r_, &engine);
            DataObject::Ptr e = session.get_lazy(k);
            CPPUNIT_ASSERT(Decimal(_T("0.01")) == e->get(_T("B")).as_decimal());
            CPPUNIT_ASSERT(Value() == e->get(_T("ORM_TEST_ID")));
            DataObject::Ptr d = DataObject::get_master(e, _T("orm_test"));
        }
    }

    void test_lazy_load_slaves()
    {
        Engine engine(Engine::READ_ONLY);
        SETUP_LOG(engine);
        Session session(r_, &engine);
        DataObject::Ptr d = session.get_lazy
            (r_.table(_T("T_ORM_TEST")).mk_key(-10));
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

    void test_flush_dirty()
    {
        {
            Engine engine;
            SETUP_LOG(engine);
            Session session(r_, &engine);
            DataObject::Ptr d = session.get_lazy(r_.table(_T("T_ORM_TEST")).mk_key(-10));
            CPPUNIT_ASSERT_EQUAL(string("item"), NARROW(d->get(_T("A")).as_string()));
            d->set(_T("A"), Value(_T("xyz")));
            CPPUNIT_ASSERT_EQUAL((int)DataObject::Dirty, (int)d->status());
            session.flush();
            CPPUNIT_ASSERT_EQUAL((int)DataObject::Ghost, (int)d->status());
            engine.commit();
        }
        {
            Engine engine;
            SETUP_LOG(engine);
            Session session(r_, &engine);
            DataObject::Ptr d = session.get_lazy(r_.table(_T("T_ORM_TEST")).mk_key(-10));
            CPPUNIT_ASSERT_EQUAL(string("xyz"), NARROW(d->get(_T("A")).as_string()));
            CPPUNIT_ASSERT_EQUAL((int)DataObject::Sync, (int)d->status());
        }
    }

    void test_flush_new()
    {
        Key k;
        {
            Engine engine;
            SETUP_LOG(engine);
            Session session(r_, &engine);
            DataObject::Ptr d = DataObject::create_new(r_.table(_T("T_ORM_TEST")));
            d->set(_T("A"), Value(_T("abc")));
            session.save(d);
            CPPUNIT_ASSERT_EQUAL((size_t)0, session.identity_map_.size());
            CPPUNIT_ASSERT_EQUAL((size_t)1, session.objects_.size());
            session.flush();
            CPPUNIT_ASSERT_EQUAL((size_t)1, session.identity_map_.size());
            CPPUNIT_ASSERT_EQUAL((size_t)1, session.objects_.size());
            CPPUNIT_ASSERT_EQUAL((int)DataObject::Ghost, (int)d->status());
            k = d->key();
            engine.commit();
        }
        {
            Engine engine;
            SETUP_LOG(engine);
            Session session(r_, &engine);
            DataObject::Ptr d = session.get_lazy(k);
            CPPUNIT_ASSERT_EQUAL(string("abc"), NARROW(d->get(_T("A")).as_string()));
            CPPUNIT_ASSERT_EQUAL((int)DataObject::Sync, (int)d->status());
        }
    }

    void test_flush_new_with_id()
    {
        Key k;
        {
            Engine engine;
            SETUP_LOG(engine);
            Session session(r_, &engine);
            DataObject::Ptr d = DataObject::create_new
                (r_.table(_T("T_ORM_TEST")));
            d->set(_T("ID"), Value(-40));
            d->set(_T("A"), Value(_T("qwerty")));
            CPPUNIT_ASSERT(d->assigned_key());
            k = d->key();
            session.save(d);
            session.flush();
            engine.commit();
        }
        {
            Engine engine(Engine::READ_ONLY);
            SETUP_LOG(engine);
            Session session(r_, &engine);
            DataObject::Ptr d = session.get_lazy(k);
            CPPUNIT_ASSERT_EQUAL(string("qwerty"), NARROW(d->get(_T("A")).as_string()));
        }
    }

    void test_flush_new_linked()
    {
        Key k;
        {
            Engine engine;
            SETUP_LOG(engine);
            Session session(r_, &engine);
            DataObject::Ptr e = DataObject::create_new(r_.table(_T("T_ORM_XML")));
            e->set(_T("B"), Value(Decimal(_T("0.01"))));
            DataObject::Ptr d = DataObject::create_new(r_.table(_T("T_ORM_TEST")));
            d->set(_T("A"), Value(_T("abc")));
            DataObject::link_slave_to_master(e, d);
            session.save(e);
            session.save(d);
            DataObject::Ptr f = DataObject::create_new(r_.table(_T("T_ORM_XML")));
            f->set(_T("B"), Value(Decimal(_T("0.03"))));
            DataObject::link_slave_to_master(f, d);
            session.save(f);
            session.flush();
            CPPUNIT_ASSERT_EQUAL((int)DataObject::Ghost, (int)d->status());
            CPPUNIT_ASSERT_EQUAL((int)DataObject::Ghost, (int)e->status());
            CPPUNIT_ASSERT_EQUAL((int)DataObject::Ghost, (int)f->status());
            k = d->key();
            engine.commit();
        }
        {
            Engine engine;
            SETUP_LOG(engine);
            Session session(r_, &engine);
            DataObject::Ptr d = session.get_lazy(k);
            CPPUNIT_ASSERT_EQUAL(string("abc"), NARROW(d->get(_T("A")).as_string()));
            CPPUNIT_ASSERT_EQUAL((int)DataObject::Sync, (int)d->status());
            RelationObject *ro = d->get_slaves();
            CPPUNIT_ASSERT_EQUAL((size_t)2, ro->count_slaves());
        }
    }

    void test_flush_new_linked_to_existing()
    {
        Key k;
        {
            Engine engine;
            SETUP_LOG(engine);
            Session session(r_, &engine);
            DataObject::Ptr d = DataObject::create_new(r_.table(_T("T_ORM_TEST")));
            d->set(_T("A"), Value(_T("abc")));
            session.save(d);
            session.flush();
            k = d->key();
            engine.commit();
        }
        {
            Engine engine;
            SETUP_LOG(engine);
            Session session(r_, &engine);
            DataObject::Ptr d = session.get_lazy(k);
            d->get(_T("A"));
            DataObject::Ptr e = DataObject::create_new(r_.table(_T("T_ORM_XML")));
            e->set(_T("B"), Value(Decimal(_T("0.01"))));
            CPPUNIT_ASSERT_EQUAL((size_t)0, d->master_relations().size());
            CPPUNIT_ASSERT_EQUAL((size_t)0, d->slave_relations().size());
            CPPUNIT_ASSERT_EQUAL((size_t)0, e->master_relations().size());
            CPPUNIT_ASSERT_EQUAL((size_t)0, e->slave_relations().size());
            DataObject::link_slave_to_master(e, d);
            CPPUNIT_ASSERT_EQUAL((size_t)1, d->master_relations().size());
            CPPUNIT_ASSERT_EQUAL((size_t)0, d->slave_relations().size());
            CPPUNIT_ASSERT_EQUAL((size_t)0, e->master_relations().size());
            CPPUNIT_ASSERT_EQUAL((size_t)1, e->slave_relations().size());
            RelationObject *ro = *e->slave_relations().begin();
            CPPUNIT_ASSERT_EQUAL((size_t)1, ro->slave_objects().size());
            session.save(e);
            session.flush();
            engine.commit();
        }
        {
            Engine engine;
            SETUP_LOG(engine);
            Session session(r_, &engine);
            DataObject::Ptr d = session.get_lazy(k);
            CPPUNIT_ASSERT_EQUAL(string("abc"), NARROW(d->get(_T("A")).as_string()));
            CPPUNIT_ASSERT_EQUAL((int)DataObject::Sync, (int)d->status());
            RelationObject *ro = d->get_slaves();
            CPPUNIT_ASSERT_EQUAL((size_t)1, ro->count_slaves());
        }
    }

    void test_flush_deleted()
    {
        Key k;
        {
            Engine engine;
            SETUP_LOG(engine);
            Session session(r_, &engine);
            DataObject::Ptr d = session.get_lazy
                (r_.table(_T("T_ORM_TEST")).mk_key(-10));
            RelationObject *ro = d->get_slaves();
            ((Relation &)ro->relation_info()).set_cascade(Relation::Delete);
            ro->lazy_load_slaves();
            d->delete_object();
            session.flush();
            engine.commit();
        }
        {
            Engine engine;
            SETUP_LOG(engine);
            Session session(r_, &engine);
            int count = 0;
            DataObject::Ptr d = session.get_lazy
                (r_.table(_T("T_ORM_TEST")).mk_key(-10));
            try { d->get(_T("A")); }
            catch (const ObjectNotFoundByKey &) { ++count; }
            DataObject::Ptr e = session.get_lazy
                (r_.table(_T("T_ORM_XML")).mk_key(-20));
            try { e->get(_T("B")); }
            catch (const ObjectNotFoundByKey &) { ++count; }
            DataObject::Ptr f = session.get_lazy
                (r_.table(_T("T_ORM_XML")).mk_key(-30));
            try { f->get(_T("B")); }
            catch (const ObjectNotFoundByKey &) { ++count; }
            CPPUNIT_ASSERT_EQUAL(3, count);
        }
    }

    void test_domain_object(void)
    {
        Engine engine(Engine::READ_ONLY);
        SETUP_LOG(engine);
        Session session(r_, &engine);
        DomainObject d(session, r_.table(_T("T_ORM_TEST")).mk_key(-10));
        CPPUNIT_ASSERT_EQUAL((int)DataObject::Ghost, (int)d.status());
        DomainObject e = d;
        CPPUNIT_ASSERT_EQUAL(0, e.cmp(d));
        CPPUNIT_ASSERT_EQUAL(string("item"), NARROW(d.get(_T("A")).as_string()));
        CPPUNIT_ASSERT_EQUAL((int)DataObject::Sync, (int)e.status());
        typedef ManagedList<DomainObject> MList;
        MList slaves = d.get_slaves();
        CPPUNIT_ASSERT_EQUAL((size_t)2, slaves.size());
        MList::iterator it = slaves.begin();
        int a = it->get(_T("ID")).as_longint();
        CPPUNIT_ASSERT(-20 == a || -30 == a);
        ++it;
        a += it->get(_T("ID")).as_longint();
        CPPUNIT_ASSERT_EQUAL(-50, a);
        ++it;
        CPPUNIT_ASSERT(slaves.end() == it);
        CPPUNIT_ASSERT_EQUAL((size_t)2, slaves.size());
        DomainObject f(r_, _T("T_ORM_XML"));
        f.save(session);
        f.link_to_master(d);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestDataObjectSaveLoad);

// vim:ts=4:sts=4:sw=4:et:
