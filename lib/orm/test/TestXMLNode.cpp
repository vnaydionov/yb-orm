
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>

#include "util/str_utils.hpp"
#include "orm/XMLNode.h"
#include "orm/MapperSession.h"
#include "orm/MetaDataSingleton.h"
#include "orm/DomainFactorySingleton.h"
#include "orm/DataObj.h"
#include "orm/AutoXMLizable.h"
#include "orm/OdbcDriver.h"

#define TEST_TBL1 "T_ORM_TEST"
#define NUM_STMT 4

using namespace std;
using namespace Yb::ORMapper;
using namespace Yb;
using namespace Yb::StrUtils;

class OrmTestDomainSimple: public Domain::AutoXMLizable
{
    ORMapper::Mapper *mapper_;
    Domain::StrongObject obj_;
public:
    OrmTestDomainSimple(ORMapper::Mapper &mapper, long long id)
        :mapper_(&mapper)
        ,obj_(mapper, "T_ORM_TEST", id)
    {}
    const XMLNode auto_xmlize(int deep = 0) const
    {
        return obj_.auto_xmlize(*mapper_, deep);
    }
};

/*
        CREATE TABLE T_ORM_XML (
            ID  NUMBER          NOT NULL,
            ORM_TEST_ID NUMBER  NOT NULL,
            B   NUMBER,
            PRIMARY KEY(ID));
*/
class OrmXMLDomainSimple: public Domain::AutoXMLizable
{
    ORMapper::Mapper *mapper_;
    Domain::StrongObject obj_;
public:
    OrmXMLDomainSimple(ORMapper::Mapper &mapper, long long id)
        :mapper_(&mapper)
        ,obj_(mapper, "T_ORM_XML", id)
    {}
    const XMLNode auto_xmlize(int deep = 0) const
    {
        return obj_.auto_xmlize(*mapper_, deep);
    }
};

class TestXMLNode : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestXMLNode);
    CPPUNIT_TEST(test_xmlize_row_data);
    CPPUNIT_TEST(test_replace_child_object);
    CPPUNIT_TEST(test_add_node);
    CPPUNIT_TEST(test_xmlize_null_value);
    CPPUNIT_TEST(test_deep_xmlize1);
    CPPUNIT_TEST(test_deep_xmlize2);
    CPPUNIT_TEST(test_deep_xmlize3);
    CPPUNIT_TEST_SUITE_END();

    TableMetaDataRegistry r_;
    string db_type_;

    void init_singleton_registry()
    {
        static bool registered = false;
        if (!registered) {
            registered = true;
            theDomainFactory::instance().register_creator("T_ORM_XML",
                ORMapper::CreatorPtr(new ORMapper::DomainCreator<OrmXMLDomainSimple>()));
            theDomainFactory::instance().register_creator("T_ORM_TEST",
                ORMapper::CreatorPtr(new ORMapper::DomainCreator<OrmTestDomainSimple>()));
        }

        TableMetaDataRegistry &r = theMetaData::instance();
//      if(r.size() == 0) {
            TableMetaData t("T_ORM_TEST", "orm-test");
            t.set_seq_name("S_ORM_TEST_ID");
            t.set_column(ColumnMetaData("ID", Value::LongLong, 0, ColumnMetaData::PK | ColumnMetaData::RO));
            t.set_column(ColumnMetaData("A", Value::String, 50, 0));
            t.set_column(ColumnMetaData("B", Value::DateTime, 0, 0));
            t.set_column(ColumnMetaData("C", Value::Decimal, 0, 0));
            r.set_table(t);
            TableMetaData t2("T_ORM_XML", "orm-xml");
            t2.set_seq_name("S_ORM_TEST_ID");
            t2.set_column(ColumnMetaData("ID", Value::LongLong, 0, ColumnMetaData::PK | ColumnMetaData::RO));
            t2.set_column(ColumnMetaData("ORM_TEST_ID", Value::LongLong, 0, 0, "T_ORM_TEST", "ID"));
            t2.set_column(ColumnMetaData("B", Value::Decimal, 0, 0));
            r.set_table(t2);
//      }
    }
    
public:
    void setUp()
    {
        db_type_ = xgetenv("YBORM_DBTYPE");

        TableMetaData t("A");
        t.set_column(ColumnMetaData("X", Value::LongLong, 0, ColumnMetaData::PK | ColumnMetaData::RO));
        t.set_column(ColumnMetaData("Y", Value::String, 0, 0));
        t.set_column(ColumnMetaData("Z", Value::Decimal, 0, ColumnMetaData::RO));
        TableMetaDataRegistry r;
        r.set_table(t);
        TableMetaData t2("XX");
        t2.set_column(ColumnMetaData("XA", Value::LongLong, 0, ColumnMetaData::PK | ColumnMetaData::RO));
        t2.set_column(ColumnMetaData("XB", Value::String, 0, 0));
        t2.set_column(ColumnMetaData("XC", Value::Decimal, 0, ColumnMetaData::RO));
        r.set_table(t2);
        TableMetaData t3("N");
        t3.set_column(ColumnMetaData("A", Value::LongLong, 0, ColumnMetaData::PK | ColumnMetaData::RO));
        r.set_table(t3);
        r_ = r;

        const char **st;
        if (db_type_ == "ORACLE") {
            static const char *st_data[NUM_STMT] = {
                "DELETE FROM " TEST_TBL1,
                "INSERT INTO " TEST_TBL1 "(ID, A, B, C) VALUES(1, "                              
                    "'abc', TO_DATE('1981-05-30', 'YYYY-MM-DD'), 3.14)",                         
                "INSERT INTO " TEST_TBL1 "(ID, A, B, C) VALUES(2, "
                    "'xyz', TO_DATE('2006-11-22 09:54:00', 'YYYY-MM-DD HH24:MI:SS'), -0.5)",     
                "INSERT INTO " TEST_TBL1 "(ID, A, B, C) VALUES(3, "
                    "'@#$', TO_DATE('2006-11-22', 'YYYY-MM-DD'), 0.01)"    
            };
            st = st_data;
        }
        else {
            static const char *st_data[NUM_STMT] = {
                "DELETE FROM " TEST_TBL1,
                "INSERT INTO " TEST_TBL1 "(ID, A, B, C) VALUES(1, "                              
                    "'abc', '1981-05-30', 3.14)",
                "INSERT INTO " TEST_TBL1 "(ID, A, B, C) VALUES(2, "                              
                    "'xyz', '2006-11-22 09:54:00', -0.5)",
                "INSERT INTO " TEST_TBL1 "(ID, A, B, C) VALUES(3, "                              
                    "'@#$', '2006-11-22', 0.01)"
            };
            st = st_data;
        }
        Yb::SQL::OdbcDriver drv;
        drv.open(xgetenv("YBORM_DB"), xgetenv("YBORM_USER"),
                xgetenv("YBORM_PASSWD"));
        for (size_t i = 0; i < NUM_STMT; ++i)
            drv.exec_direct(st[i]);
        drv.commit();
    }

    void test_xmlize_row_data()
    {
        RowData data(r_, "A");
        data.set("x", 10);
        data.set("y", "zzz");
        data.set("z", decimal("1.20"));
        XMLNode node(data);
        CPPUNIT_ASSERT_EQUAL(string("<a><x>10</x><y>zzz</y><z>1.2</z></a>\n"), node.get_xml());
    }

    void test_replace_child_object()
    {
        RowData data(r_, "A");
        data.set("x", 10);
        data.set("y", "zzz");
        data.set("z", decimal(1.2));
        RowData datax(r_, "XX");
        datax.set("xa", 20);
        datax.set("xb", "aaa");
        datax.set("xc", decimal("1.1"));
        XMLNode node(data);
        node.replace_child_object_by_field("x", datax); 
        CPPUNIT_ASSERT_EQUAL(string(
                    "<a><xx><xa>20</xa><xb>aaa</xb><xc>1.1</xc></xx><y>zzz</y><z>1.2</z></a>\n"),
                node.get_xml());
    }

    void test_add_node()
    {
        RowData data(r_, "A");
        data.set("x", 10);
        data.set("y", "zzz");
        data.set("z", decimal("1.2"));
        RowData datax(r_, "XX");
        datax.set("xa", 20);
        datax.set("xb", "aaa");
        datax.set("xc", decimal("1.1"));
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
        RowData data(r_, "N");
        data.set("a", Value());
        XMLNode node(data);
        CPPUNIT_ASSERT_EQUAL(std::string("<n><a is_null=\"1\"/></n>\n"), node.get_xml());
    }
    
    void test_deep_xmlize1()
    {
        init_singleton_registry();
        MapperSession mapper;
        OrmXMLDomainSimple test(mapper, 10);
        XMLNode node = test.auto_xmlize();
        CPPUNIT_ASSERT_EQUAL(std::string("<orm-xml><b>4</b><id>10</id><orm-test-id>1</orm-test-id></orm-xml>\n"),
                node.get_xml());
    }

    void test_deep_xmlize2()
    {
        init_singleton_registry();
        MapperSession mapper;
        OrmXMLDomainSimple test(mapper, 10);
        XMLNode node = test.auto_xmlize(1);        
        CPPUNIT_ASSERT_EQUAL(
                std::string(
                    "<orm-xml><b>4</b><id>10</id><orm-test><a>abc</a><b>1981-05-30T00:00:00</b>"
                    "<c>3.14</c><id>1</id></orm-test></orm-xml>\n"), node.get_xml());       
    }

    void test_deep_xmlize3()
    {
        init_singleton_registry();
        MapperSession mapper;
        OrmXMLDomainSimple test(mapper, 10);
        XMLNode node = test.auto_xmlize(-1);     
        CPPUNIT_ASSERT_EQUAL(
                std::string(
                    "<orm-xml><b>4</b><id>10</id><orm-test><a>abc</a><b>1981-05-30T00:00:00</b>"
                    "<c>3.14</c><id>1</id></orm-test></orm-xml>\n"), node.get_xml());        
    }
    
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestXMLNode);

// vim:ts=4:sts=4:sw=4:et
