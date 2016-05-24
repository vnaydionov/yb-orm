#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
#include "util/string_utils.h"
#include "orm/xmlizer.h"
#include "orm/domain_object.h"
#include "orm/domain_factory.h"
#include "orm/engine.h"

#define NUM_STMT 6

using namespace std;
using namespace Yb;
using namespace Yb::StrUtils;

class OrmTestDomainSimple: public DomainObject
{
public:
    OrmTestDomainSimple(Session &session, LongInt id)
        : DomainObject(session, _T("T_ORM_TEST"), id)
    {}
};

/*
        CREATE TABLE T_ORM_XML (
            ID  NUMBER          NOT NULL,
            ORM_TEST_ID NUMBER  NOT NULL,
            B   NUMBER,
            PRIMARY KEY(ID));
*/
class OrmXMLDomainSimple: public DomainObject
{
public:
    OrmXMLDomainSimple(Session &session, LongInt id)
        : DomainObject(session, _T("T_ORM_XML"), id)
    {}
};

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
    e.set_logger(root_logger->new_logger(_T("engine")));
    e.set_echo(true);
}

static void setup_log(SqlConnection &c)
{
    init_log();
    c.init_logger(root_logger.get());
    c.set_echo(true);
}


class TestXMLizer : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestXMLizer);
    CPPUNIT_TEST(test_xmlize_data_object);
    CPPUNIT_TEST(test_replace_child_object);
    CPPUNIT_TEST(test_add_node);
    CPPUNIT_TEST(test_xmlize_null_value);
    CPPUNIT_TEST(test_deep_xmlize1);
    CPPUNIT_TEST(test_deep_xmlize2);
    CPPUNIT_TEST(test_deep_xmlize3);
    CPPUNIT_TEST_SUITE_END();

    Schema r_;

    void init_singleton_registry()
    {
        static bool registered = false;
        if (!registered) {
            registered = true;
            theDomainFactory().register_creator(_T("T_ORM_XML"),
                CreatorPtr(new DomainCreator<OrmXMLDomainSimple>()));
            theDomainFactory().register_creator(_T("T_ORM_TEST"),
                CreatorPtr(new DomainCreator<OrmTestDomainSimple>()));
        }

        Schema r;
        Table::Ptr t(new Table(_T("T_ORM_TEST"), _T("orm-test"), _T("OrmTest")));
        t->set_seq_name(_T("S_ORM_TEST_ID"));
        t->add_column(Column(_T("ID"), Value::LONGINT, 0, Column::PK | Column::RO));
        t->add_column(Column(_T("A"), Value::STRING, 50, 0));
        t->add_column(Column(_T("B"), Value::DATETIME, 0, 0));
        t->add_column(Column(_T("C"), Value::DECIMAL, 0, 0));
        r.add_table(t);
        Table::Ptr t2(new Table(_T("T_ORM_XML"), _T("orm-xml"), _T("OrmXml")));
        t2->set_seq_name(_T("S_ORM_TEST_ID"));
        t2->add_column(Column(_T("ID"), Value::LONGINT, 0, Column::PK | Column::RO));
        t2->add_column(Column(_T("ORM_TEST_ID"), Value::LONGINT, 0, 0,
                    Value(), _T("T_ORM_TEST"), _T("ID")));
        t2->add_column(Column(_T("B"), Value::DECIMAL, 0, 0));
        r.add_table(t2);
        Relation::AttrMap a1, a2;
        a2[_T("property")] = _T("orm_test");
        Relation::Ptr re(new Relation(Relation::ONE2MANY, _T("OrmTest"), a1, _T("OrmXml"), a2));
        r.add_relation(re);
        r_ = r;
    }

public:
    void setUp()
    {
        Table::Ptr t(new Table(_T("A")));
        t->add_column(Column(_T("X"), Value::LONGINT, 0, Column::PK | Column::RO));
        t->add_column(Column(_T("Y"), Value::STRING, 0, 0));
        t->add_column(Column(_T("Z"), Value::DECIMAL, 0, 0));
        Schema r;
        r.add_table(t);
        Table::Ptr t2(new Table(_T("XX")));
        t2->add_column(Column(_T("XA"), Value::LONGINT, 0, Column::PK | Column::RO));
        t2->add_column(Column(_T("XB"), Value::STRING, 0, 0));
        t2->add_column(Column(_T("XC"), Value::DECIMAL, 0, 0));
        r.add_table(t2);
        Table::Ptr t3(new Table(_T("N")));
        t3->add_column(Column(_T("A"), Value::LONGINT, 0, Column::PK | Column::RO));
        r.add_table(t3);
        r_ = r;

        SqlConnection conn(Engine::sql_source_from_env());
        conn.set_convert_params(true);
        setup_log(conn);
        conn.begin_trans_if_necessary();
        conn.grant_insert_id(_T("T_ORM_TEST"), true, true);
        conn.prepare(
            _T("INSERT INTO T_ORM_TEST(ID, A, B, C) VALUES(?, ?, ?, ?)"));
        Values params(4);
        params[0] = Value(1);
        params[1] = Value(_T("abc"));
        params[2] = Value(dt_make(1981, 5, 30));
        params[3] = Value(Decimal(_T("3.14")));
        conn.exec(params);
        params[0] = Value(2);
        params[1] = Value(_T("xyz"));
        params[2] = Value(dt_make(2006, 11, 22, 9, 54));
        params[3] = Value(Decimal(_T("-0.5")));
        conn.exec(params);
        params[0] = Value(3);
        params[1] = Value(_T("@#$"));
        params[2] = Value(dt_make(2006, 11, 22));
        params[3] = Value(Decimal(_T("0.01")));
        conn.exec(params);
        conn.grant_insert_id(_T("T_ORM_TEST"), false, true);
        conn.grant_insert_id(_T("T_ORM_XML"), true, true);
        conn.prepare(
            _T("INSERT INTO T_ORM_XML(ID, ORM_TEST_ID, B) VALUES (?, ?, ?)"));
        params.resize(3);
        params[0] = Value(10);
        params[1] = Value(1);
        params[2] = Value(4);
        conn.exec(params);
        conn.commit();
        conn.grant_insert_id(_T("T_ORM_XML"), false, true);
    }

    void tearDown()
    {
        SqlConnection conn(Engine::sql_source_from_env());
        setup_log(conn);
        conn.begin_trans_if_necessary();
        conn.exec_direct(_T("DELETE FROM T_ORM_XML"));
        conn.exec_direct(_T("DELETE FROM T_ORM_TEST"));
        conn.grant_insert_id(_T("T_ORM_TEST"), false, true);
        conn.grant_insert_id(_T("T_ORM_XML"), false, true);
        conn.commit();
    }

    void test_xmlize_data_object()
    {
        DataObject::Ptr data = DataObject::create_new(r_.table(_T("A")));
        data->set(_T("x"), 10);
        data->set(_T("y"), String(_T("zzz")));
        data->set(_T("z"), Decimal(_T("1.20")));
        ElementTree::ElementPtr node = data_object_to_etree(data);
        CPPUNIT_ASSERT_EQUAL(string("<a><x>10</x><y>zzz</y><z>1.2</z></a>\n"), node->serialize());
    }

    void test_replace_child_object()
    {
        DataObject::Ptr data = DataObject::create_new(r_.table(_T("A")));
        data->set(_T("x"), 10);
        data->set(_T("y"), String(_T("zzz")));
        data->set(_T("z"), Decimal(1.2));
        DataObject::Ptr datax = DataObject::create_new(r_.table(_T("XX")));
        datax->set(_T("xa"), 20);
        datax->set(_T("xb"), String(_T("aaa")));
        datax->set(_T("xc"), Decimal(_T("1.1")));
        ElementTree::ElementPtr node = data_object_to_etree(data);
        replace_child_object_by_field(node, _T("x"), datax);
        CPPUNIT_ASSERT_EQUAL(string(
                    "<a><xx><xa>20</xa><xb>aaa</xb><xc>1.1</xc></xx><y>zzz</y><z>1.2</z></a>\n"),
                node->serialize());
    }

    void test_add_node()
    {
        DataObject::Ptr data = DataObject::create_new(r_.table(_T("A")));
        data->set(_T("x"), 10);
        data->set(_T("y"), String(_T("zzz")));
        data->set(_T("z"), Decimal(_T("1.2")));
        DataObject::Ptr datax = DataObject::create_new(r_.table(_T("XX")));
        datax->set(_T("xa"), 20);
        datax->set(_T("xb"), String(_T("aaa")));
        datax->set(_T("xc"), Decimal(_T("1.1")));
        ElementTree::ElementPtr node = data_object_to_etree(data);
        ElementTree::ElementPtr child = data_object_to_etree(datax);
        node->children_.push_back(child);
        CPPUNIT_ASSERT_EQUAL(string(
                    "<a><x>10</x><y>zzz</y><z>1.2</z>"
                    "<xx><xa>20</xa><xb>aaa</xb><xc>1.1</xc></xx></a>\n"),
                node->serialize());
    }

    void test_xmlize_null_value()
    {
        DataObject::Ptr data = DataObject::create_new(r_.table(_T("N")));
        data->set(_T("a"), Value());
        ElementTree::ElementPtr node = data_object_to_etree(data);
        CPPUNIT_ASSERT_EQUAL(std::string("<n><a is_null=\"1\"/></n>\n"),
                node->serialize());
    }

    void test_deep_xmlize1()
    {
        init_singleton_registry();
        Engine engine(Engine::READ_ONLY);
        setup_log(engine);
        Session session(r_, &engine);
        OrmXMLDomainSimple test(session, 10);
        ElementTree::ElementPtr node = test.xmlize();
        CPPUNIT_ASSERT_EQUAL(string(
            "<orm-xml><id>10</id><orm-test-id>1</orm-test-id><b>4</b></orm-xml>\n"),
            node->serialize());
    }

    void test_deep_xmlize2()
    {
        init_singleton_registry();
        Engine engine(Engine::READ_ONLY);
        setup_log(engine);
        Session session(r_, &engine);
        OrmXMLDomainSimple test(session, 10);
        ElementTree::ElementPtr node = test.xmlize(1);
        CPPUNIT_ASSERT_EQUAL(string(
            "<orm-xml><id>10</id><orm-test><id>1</id><a>abc</a>"
            "<b>1981-05-30T00:00:00</b><c>3.14</c></orm-test><b>4</b></orm-xml>\n"),
            node->serialize());
    }

    void test_deep_xmlize3()
    {
        init_singleton_registry();
        Engine engine(Engine::READ_ONLY);
        setup_log(engine);
        Session session(r_, &engine);
        OrmXMLDomainSimple test(session, 10);
        ElementTree::ElementPtr node = test.xmlize(-1);
        CPPUNIT_ASSERT_EQUAL(string(
            "<orm-xml><id>10</id><orm-test><id>1</id><a>abc</a>"
            "<b>1981-05-30T00:00:00</b><c>3.14</c></orm-test><b>4</b></orm-xml>\n"),
            node->serialize());
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestXMLizer);

// vim:ts=4:sts=4:sw=4:et:
