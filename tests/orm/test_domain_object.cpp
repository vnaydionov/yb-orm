// -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
#include "util/string_utils.h"
#include "orm/domain_object.h"
#include "orm/domain_factory.h"
#include "orm/schema_decl.h"

class OrmXml;

class OrmTest: public Yb::DomainObject {

YB_DECLARE(OrmTest, "T_ORM_TEST", "S_ORM_TEST_ID", "orm-test",
    YB_COL_PK(id, "ID")
    YB_COL_STR(a, "A", 200)
    YB_COL_DATA(b, "B", DATETIME)
    YB_COL_DATA(c_, "C", DECIMAL)
    YB_COL_DATA(d, "D", FLOAT)

    YB_REL_ONE(OrmTest, orm_test, OrmXml, orm_xmls, Yb::Relation::Delete, "ORM_TEST_ID", 1, "1")
    YB_COL_END)
};

class OrmXml: public Yb::DomainObject {

YB_DECLARE(OrmXml, "T_ORM_XML", "S_ORM_XML_ID", "orm-xml",
    YB_COL_PK(id, "ID")
    YB_COL_FK_NULLABLE(orm_test_id, "ORM_TEST_ID", "T_ORM_TEST", "ID")
    YB_COL_DATA(b, "B", DECIMAL)

    YB_REL_MANY(OrmTest, orm_test, OrmXml, orm_xmls, Yb::Relation::Delete, "ORM_TEST_ID", 1, "1")
    YB_COL_END)
};

YB_DEFINE(OrmTest)
YB_DEFINE(OrmXml)

using namespace std;
using namespace Yb;

// Real DB tests

static LogAppender appender(cerr);
static ILogger::Ptr root_logger;

static void init_log()
{
    if (!root_logger.get())
        root_logger.reset(new Logger(&appender));
}

static void setup_log(Engine &e)
{
    init_log();
    e.set_logger(root_logger->new_logger("engine"));
    e.set_echo(true);
}

static void setup_log(SqlConnection &c)
{
    init_log();
    c.init_logger(root_logger.get());
    c.set_echo(true);
}


class TestDomainObject : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestDomainObject);
    CPPUNIT_TEST(test_lazy_load);
    CPPUNIT_TEST_EXCEPTION(test_lazy_load_failed, ObjectNotFoundByKey);
    CPPUNIT_TEST(test_empty_object);
    CPPUNIT_TEST(test_null_fk_relation);
    CPPUNIT_TEST(test_holder);
    CPPUNIT_TEST(test_link_one2many);
    CPPUNIT_TEST(test_join);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp()
    {
        SqlConnection conn(Engine::sql_source_from_env());
        conn.set_convert_params(true);
        setup_log(conn);
        conn.begin_trans_if_necessary();
        {
            String sql_str =
                _T("INSERT INTO T_ORM_TEST(ID, A, B, C, D) VALUES(?, ?, ?, ?, ?)");
            conn.prepare(sql_str);
            Values params(5);
            params[0] = Value(-10);
            params[1] = Value(_T("item"));
            params[2] = Value(now());
            params[3] = Value(Decimal(_T("1.2")));
            params[4] = Value(4.56);
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
        {
            String sql_str =
                _T("INSERT INTO T_ORM_XML(ID, ORM_TEST_ID, B) VALUES (?, ?, ?)");
            conn.prepare(sql_str);
            Values params(3);
            params[0] = Value(-40);
            params[1] = Value();
            params[2] = Value(Decimal(_T("42")));
            conn.exec(params);
        }
        conn.commit();

        Yb::init_schema();
    }

    void tearDown()
    {
        SqlConnection conn(Engine::sql_source_from_env());
        setup_log(conn);
        conn.begin_trans_if_necessary();
        conn.exec_direct(_T("DELETE FROM T_ORM_XML"));
        conn.exec_direct(_T("DELETE FROM T_ORM_TEST"));
        conn.commit();
    }

    void test_lazy_load()
    {
        Engine engine(Engine::READ_ONLY);
        setup_log(engine);
        Session session(Yb::theSchema(), &engine);
        OrmTest dobj(session, -10);
        CPPUNIT_ASSERT_EQUAL((int)DataObject::Ghost, (int)dobj.get_data_object()->status());
        CPPUNIT_ASSERT_EQUAL(string("item"), NARROW(dobj.a.value()));
        CPPUNIT_ASSERT_EQUAL((int)DataObject::Sync, (int)dobj.get_data_object()->status());
        dobj.a = _T("new_item");
        CPPUNIT_ASSERT_EQUAL(string("new_item"), NARROW(dobj.a.value()));
        CPPUNIT_ASSERT_EQUAL((int)DataObject::Dirty, (int)dobj.get_data_object()->status());
    }

    void test_lazy_load_failed()
    {
        Engine engine(Engine::READ_ONLY);
        setup_log(engine);
        Session session(Yb::theSchema(), &engine);
        OrmTest dobj(session, -20);
        CPPUNIT_ASSERT_EQUAL((int)DataObject::Ghost, (int)dobj.get_data_object()->status());
        String x = dobj.a;
    }

    void test_empty_object()
    {
        // DomainObject does not contain link to DataObject
        OrmXml dobj(EMPTY_DATAOBJ);
        CPPUNIT_ASSERT(dobj.is_empty());
        // The same, using D::Holder:
        OrmXml::Holder dh(EMPTY_DATAOBJ);
        CPPUNIT_ASSERT(dh->is_empty());
    }

    void test_null_fk_relation()
    {
        Engine engine(Engine::READ_ONLY);
        setup_log(engine);
        Session session(Yb::theSchema(), &engine);
        OrmXml dobj(session, -40);
        CPPUNIT_ASSERT_EQUAL((int)DataObject::Ghost, (int)dobj.get_data_object()->status());
        Decimal x = dobj.b;
        CPPUNIT_ASSERT_EQUAL((int)DataObject::Sync, (int)dobj.get_data_object()->status());
        // Holder is always not empty!
        CPPUNIT_ASSERT(dobj.orm_test._get_p() != NULL);
        // but:
        // DomainObject does not contain link to DataObject
        CPPUNIT_ASSERT(dobj.orm_test->is_empty());
        // Referencing DataObject does not have FK field set
        CPPUNIT_ASSERT(dobj.get(_T("ORM_TEST_ID")).is_null());
    }

    void test_holder()
    {
        // a new DomainObject with a new DataObject object are created
        OrmXml::Holder dh0;
        CPPUNIT_ASSERT(dh0._get_p() != NULL);
        CPPUNIT_ASSERT(!dh0->is_empty());
        // a new DomainObject with an empty DataObject are created
        OrmXml::Holder dh1(EMPTY_DATAOBJ);
        CPPUNIT_ASSERT(dh1._get_p() != NULL);
        CPPUNIT_ASSERT(dh1->is_empty());

        Engine engine(Engine::READ_ONLY);
        setup_log(engine);
        Session session(Yb::theSchema(), &engine);
        OrmXml dobj(session, -40);
        OrmXml::Holder dh2(dobj);
        CPPUNIT_ASSERT(dh2._get_p() != NULL);
        CPPUNIT_ASSERT_EQUAL(shptr_get(dobj.get_data_object()),
                shptr_get(dh2->get_data_object()));
    }

    void test_link_one2many()
    {
        OrmTest ot;
        OrmXml ox1, ox2, ox3;
        CPPUNIT_ASSERT_EQUAL(0, (int)ot.orm_xmls.size());
        ot.orm_xmls.insert(ox1);
        CPPUNIT_ASSERT_EQUAL(1, (int)ot.orm_xmls.size());
        CPPUNIT_ASSERT_EQUAL(shptr_get(ox1.get_data_object()),
                shptr_get(ot.orm_xmls.begin()->get_data_object()));
        ot.orm_xmls.insert(ox2);
        CPPUNIT_ASSERT_EQUAL(2, (int)ot.orm_xmls.size());
        ox3.orm_test = OrmTest::Holder(ot);
        CPPUNIT_ASSERT_EQUAL(3, (int)ot.orm_xmls.size());
    }

    void test_join()
    {
        Engine engine(Engine::READ_ONLY);
        setup_log(engine);
        Session session(Yb::theSchema(), &engine);
        auto rs = Yb::query<OrmTest, OrmXml>(*session)
                    .select_from<OrmTest>()
                    .join<OrmXml>();
        CPPUNIT_ASSERT_EQUAL(3, (int)rs.count());
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestDomainObject);

// vim:ts=4:sts=4:sw=4:et:
