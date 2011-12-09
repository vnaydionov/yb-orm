#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
#include "util/str_utils.hpp"
#include "orm/XMLNode.h"
#include "orm/MetaDataSingleton.h"
#include "orm/DomainFactorySingleton.h"
#include "orm/DomainObj.h"
#include "orm/Engine.h"

#define TEST_TBL1 _T("T_ORM_TEST")
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

class TestXMLNode : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestXMLNode);
    CPPUNIT_TEST(test_xmlize_data_object);
    CPPUNIT_TEST(test_replace_child_object);
    CPPUNIT_TEST(test_add_node);
    CPPUNIT_TEST(test_xmlize_null_value);
    CPPUNIT_TEST(test_deep_xmlize1);
    CPPUNIT_TEST(test_deep_xmlize2);
    CPPUNIT_TEST(test_deep_xmlize3);
    CPPUNIT_TEST_SUITE_END();

    Schema r_;
    String db_type_;

    void init_singleton_registry()
    {
        static bool registered = false;
        if (!registered) {
            registered = true;
            theDomainFactory::instance().register_creator(_T("T_ORM_XML"),
                CreatorPtr(new DomainCreator<OrmXMLDomainSimple>()));
            theDomainFactory::instance().register_creator(_T("T_ORM_TEST"),
                CreatorPtr(new DomainCreator<OrmTestDomainSimple>()));
        }

        Schema &r = theMetaData::instance();
//      if(r.size() == 0) {
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
            t2->add_column(Column(_T("ORM_TEST_ID"), Value::LONGINT, 0, 0, _T("T_ORM_TEST"), _T("ID")));
            t2->add_column(Column(_T("B"), Value::DECIMAL, 0, 0));
            r.add_table(t2);
            Relation::AttrMap a1, a2;
            a2[_T("property")] = _T("orm_test");
            Relation::Ptr re(new Relation(Relation::ONE2MANY, _T("OrmTest"), a1, _T("OrmXml"), a2));
            r.add_relation(re);
//      }
    }

public:
    void setUp()
    {
        db_type_ = xgetenv(_T("YBORM_DBTYPE"));

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

        const Char **st;
        if (db_type_ == _T("ORACLE")) {
            static const Char *st_data[NUM_STMT] = {
                _T("DELETE FROM T_ORM_XML"),
                _T("DELETE FROM ") TEST_TBL1,
                _T("INSERT INTO ") TEST_TBL1 _T("(ID, A, B, C) VALUES(1, ")
                    _T("'abc', TO_DATE('1981-05-30', 'YYYY-MM-DD'), 3.14)"),
                _T("INSERT INTO ") TEST_TBL1 _T("(ID, A, B, C) VALUES(2, ")
                    _T("'xyz', TO_DATE('2006-11-22 09:54:00', 'YYYY-MM-DD HH24:MI:SS'), -0.5)"),
                _T("INSERT INTO ") TEST_TBL1 _T("(ID, A, B, C) VALUES(3, ")
                    _T("'@#$', TO_DATE('2006-11-22', 'YYYY-MM-DD'), 0.01)"),
                _T("INSERT INTO T_ORM_XML(ID, ORM_TEST_ID, B) VALUES(10, 1, 4)"),
            };
            st = st_data;
        }
        else {
            static const Char *st_data[NUM_STMT] = {
                _T("DELETE FROM T_ORM_XML"),
                _T("DELETE FROM ") TEST_TBL1,
                _T("INSERT INTO ") TEST_TBL1 _T("(ID, A, B, C) VALUES(1, ")
                    _T("'abc', '1981-05-30', 3.14)"),
                _T("INSERT INTO ") TEST_TBL1 _T("(ID, A, B, C) VALUES(2, ")
                    _T("'xyz', '2006-11-22 09:54:00', -0.5)"),
                _T("INSERT INTO ") TEST_TBL1 _T("(ID, A, B, C) VALUES(3, ")
                    _T("'@#$', '2006-11-22', 0.01)"),
                _T("INSERT INTO T_ORM_XML(ID, ORM_TEST_ID, B) VALUES(10, 1, 4)"),
            };
            st = st_data;
        }
        SqlConnect conn(_T("ODBC"), xgetenv(_T("YBORM_DBTYPE")),
                xgetenv(_T("YBORM_DB")), xgetenv(_T("YBORM_USER")), xgetenv(_T("YBORM_PASSWD")));
        for (size_t i = 0; i < NUM_STMT; ++i)
            conn.exec_direct(st[i]);
        conn.commit();
    }

    void test_xmlize_data_object()
    {
        DataObject::Ptr data = DataObject::create_new(r_.table(_T("A")));
        data->set(_T("x"), 10);
        data->set(_T("y"), _T("zzz"));
        data->set(_T("z"), Decimal(_T("1.20")));
        XMLNode node(data);
        CPPUNIT_ASSERT_EQUAL(string("<a><x>10</x><y>zzz</y><z>1.2</z></a>\n"), node.get_xml());
    }

    void test_replace_child_object()
    {
        DataObject::Ptr data = DataObject::create_new(r_.table(_T("A")));
        data->set(_T("x"), 10);
        data->set(_T("y"), _T("zzz"));
        data->set(_T("z"), Decimal(1.2));
        DataObject::Ptr datax = DataObject::create_new(r_.table(_T("XX")));
        datax->set(_T("xa"), 20);
        datax->set(_T("xb"), _T("aaa"));
        datax->set(_T("xc"), Decimal(_T("1.1")));
        XMLNode node(data);
        node.replace_child_object_by_field(_T("x"), datax);
        CPPUNIT_ASSERT_EQUAL(string(
                    "<a><xx><xa>20</xa><xb>aaa</xb><xc>1.1</xc></xx><y>zzz</y><z>1.2</z></a>\n"),
                node.get_xml());
    }

    void test_add_node()
    {
        DataObject::Ptr data = DataObject::create_new(r_.table(_T("A")));
        data->set(_T("x"), 10);
        data->set(_T("y"), _T("zzz"));
        data->set(_T("z"), Decimal(_T("1.2")));
        DataObject::Ptr datax = DataObject::create_new(r_.table(_T("XX")));
        datax->set(_T("xa"), 20);
        datax->set(_T("xb"), _T("aaa"));
        datax->set(_T("xc"), Decimal(_T("1.1")));
        XMLNode node(data);
        XMLNode child(datax);
        node.add_node(datax);
        CPPUNIT_ASSERT_EQUAL(string(
                    "<a><x>10</x><y>zzz</y><z>1.2</z>"
                    "<xx><xa>20</xa><xb>aaa</xb><xc>1.1</xc></xx></a>\n"),
                node.get_xml());
    }

    void test_xmlize_null_value()
    {
        DataObject::Ptr data = DataObject::create_new(r_.table(_T("N")));
        data->set(_T("a"), Value());
        XMLNode node(data);
        CPPUNIT_ASSERT_EQUAL(std::string("<n><a is_null=\"1\"/></n>\n"), node.get_xml());
    }

    void test_deep_xmlize1()
    {
        init_singleton_registry();
        Engine engine(Engine::READ_ONLY);
        Session session(theMetaData::instance(), &engine);
        OrmXMLDomainSimple test(session, 10);
        XMLNode node = test.xmlize();
        CPPUNIT_ASSERT_EQUAL(string(
            "<orm-xml><id>10</id><orm-test-id>1</orm-test-id><b>4</b></orm-xml>\n"),
            node.get_xml());
    }

    void test_deep_xmlize2()
    {
        init_singleton_registry();
        Engine engine(Engine::READ_ONLY);
        Session session(theMetaData::instance(), &engine);
        OrmXMLDomainSimple test(session, 10);
        XMLNode node = test.xmlize(1);
        CPPUNIT_ASSERT_EQUAL(string(
            "<orm-xml><id>10</id><orm-test><id>1</id><a>abc</a>"
            "<b>1981-05-30T00:00:00</b><c>3.14</c></orm-test><b>4</b></orm-xml>\n"),
            node.get_xml());
    }

    void test_deep_xmlize3()
    {
        init_singleton_registry();
        Engine engine(Engine::READ_ONLY);
        Session session(theMetaData::instance(), &engine);
        OrmXMLDomainSimple test(session, 10);
        XMLNode node = test.xmlize(-1);
        CPPUNIT_ASSERT_EQUAL(string(
            "<orm-xml><id>10</id><orm-test><id>1</id><a>abc</a>"
            "<b>1981-05-30T00:00:00</b><c>3.14</c></orm-test><b>4</b></orm-xml>\n"),
            node.get_xml());
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestXMLNode);

// vim:ts=4:sts=4:sw=4:et:
