#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
#include "orm/XMLMetaDataConfig.h"
#include "orm/Value.h"

using namespace std;
using namespace Yb;

class TestXMLConfig : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestXMLConfig);
    CPPUNIT_TEST_EXCEPTION(testBadXML, ParseError);
    CPPUNIT_TEST(testParseColumn);
    CPPUNIT_TEST(testParseForeignKey);
    CPPUNIT_TEST(testStrTypeToInt);
    CPPUNIT_TEST_EXCEPTION(testInvalidCombination, InvalidCombination);
    CPPUNIT_TEST(testAbsentForeignKeyField);
    CPPUNIT_TEST_EXCEPTION(testAbsentForeignKeyTable, MandatoryAttributeAbsent);
    CPPUNIT_TEST_EXCEPTION(testAbsentColumnName, MandatoryAttributeAbsent);
    CPPUNIT_TEST_EXCEPTION(testAbsentColumnType, MandatoryAttributeAbsent);
    CPPUNIT_TEST_EXCEPTION(testWrongColumnType, WrongColumnType);
    CPPUNIT_TEST_EXCEPTION(testGetWrongNodeValue, ParseError);
    CPPUNIT_TEST(testParseTable);
    CPPUNIT_TEST(testNoAutoInc);
    CPPUNIT_TEST(testAutoInc);
    CPPUNIT_TEST(testNullable);
    CPPUNIT_TEST(testClassName);
    CPPUNIT_TEST(testClassNameDefault);
    CPPUNIT_TEST_EXCEPTION(testWrongElementTable, ParseError);
    CPPUNIT_TEST(testParseSchema);
    CPPUNIT_TEST(testRelationSide);
    CPPUNIT_TEST_EXCEPTION(testRelationSideNoClass, MandatoryAttributeAbsent);
    CPPUNIT_TEST(testRelationOneToMany);
    CPPUNIT_TEST_SUITE_END();

    XMLMetaDataConfig cfg_;
public:
    TestXMLConfig(): cfg_("<x/>") {}

    void testParseSchema()
    {
        string xml = 
            "<schema>"
            "<table name='A'>"
            "<column type='string' name='AA' size='10' />"
            "</table>"
            "<relation type='one-to-many'>"
            "<one class='X' property='ys'/>"
            "<many class='Y' property='x'/>"
            "</relation>"
            "<table name='B'>"
            "<column type='longint' name='BA'></column>"
            "</table>"
            "</schema>";
        XMLMetaDataConfig cfg(xml);
        Schema reg;
        cfg.parse(reg);
        CPPUNIT_ASSERT_EQUAL(2, (int)reg.tbl_count());
        const Table &t = reg.table(_T("A"));
        CPPUNIT_ASSERT_EQUAL(1, (int)t.size());
        CPPUNIT_ASSERT_EQUAL(string("AA"), NARROW(t.begin()->get_name()));
        CPPUNIT_ASSERT_EQUAL(10, (int)t.begin()->get_size());
        CPPUNIT_ASSERT_EQUAL((int)Value::STRING, t.begin()->get_type());
        const Table &t2 = reg.table(_T("B"));
        CPPUNIT_ASSERT_EQUAL(1, (int)t2.size());
        CPPUNIT_ASSERT_EQUAL(string("BA"), NARROW(t2.begin()->get_name()));
        CPPUNIT_ASSERT_EQUAL((int)Value::LONGINT, t2.begin()->get_type());

        CPPUNIT_ASSERT_EQUAL(1, (int)reg.rel_count());
        CPPUNIT_ASSERT_EQUAL((int)Relation::ONE2MANY, (*reg.rel_begin())->type());
    }
    
    void testWrongElementTable()
    {
        string xml =  "<table name='A' sequence='S'><col></col></table>";
        Xml::Node node(Xml::Parse(xml));
        Table::Ptr t = cfg_.parse_table(node.get());
    }

    void testParseTable()
    {
        string xml = 
            "<table name='A' sequence='S'>"
            "<column type='string' name='ASTR' size='10' />"
            "<column type='longint' name='B_ID'>"
            "<foreign-key table='T_B' key='ID'/></column>"
            "</table>";
        Xml::Node node(Xml::Parse(xml));
        Table::Ptr t = cfg_.parse_table(node.get());
        CPPUNIT_ASSERT_EQUAL(string("A"), NARROW(t->get_name()));
        CPPUNIT_ASSERT_EQUAL(string("S"), NARROW(t->get_seq_name()));
        CPPUNIT_ASSERT_EQUAL(2, (int)t->size());
        Columns::const_iterator it = t->begin();
        CPPUNIT_ASSERT_EQUAL(string("ASTR"), NARROW(it->get_name()));
        CPPUNIT_ASSERT_EQUAL(10, (int)it->get_size());
        CPPUNIT_ASSERT_EQUAL((int)Value::STRING, it->get_type());
        ++it;
        CPPUNIT_ASSERT_EQUAL(string("B_ID"), NARROW(it->get_name()));
        CPPUNIT_ASSERT_EQUAL(string("ID"), NARROW(it->get_fk_name()));
        CPPUNIT_ASSERT_EQUAL(string("T_B"), NARROW(it->get_fk_table_name()));
        CPPUNIT_ASSERT_EQUAL((int)Value::LONGINT, it->get_type()); 
    }

    void testNoAutoInc()
    {
        Xml::Node node(Xml::Parse(
            "<table name='A'>"
            "<column type='longint' name='B'>"
            "<primary-key/>"
            "</column>"
            "</table>"
        ));
        Table::Ptr t = cfg_.parse_table(node.get());
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t->get_seq_name()));
        CPPUNIT_ASSERT_EQUAL(false, t->get_autoinc());
    }

    void testAutoInc()
    {
        Xml::Node node(Xml::Parse(
            "<table name='A' autoinc='autoinc'>"
            "<column type='longint' name='B'>"
            "<primary-key/>"
            "</column>"
            "</table>"
        ));
        Table::Ptr t = cfg_.parse_table(node.get());
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t->get_seq_name()));
        CPPUNIT_ASSERT_EQUAL(true, t->get_autoinc());
    }

    void testNullable()
    {
        Xml::Node node(Xml::Parse(
            "<table name='A'>"
            "<column type='longint' name='B'>"
            "<primary-key/>"
            "</column>"
            "<column type='datetime' name='C'/>"
            "<column type='decimal' name='D' null='false'/>"
            "</table>"
        ));
        Table::Ptr t = cfg_.parse_table(node.get());
        CPPUNIT_ASSERT_EQUAL(false, t->get_column(_T("B")).is_nullable());
        CPPUNIT_ASSERT_EQUAL(true, t->get_column(_T("C")).is_nullable());
        CPPUNIT_ASSERT_EQUAL(false, t->get_column(_T("D")).is_nullable());
    }

    void testClassName()
    {
        Xml::Node node(Xml::Parse(
            "<table name='A' class='aa' xml-name='bb'>"
            "<column type='longint' name='B' property='xx'/>"
            "<column type='longint' name='C' xml-name='dd'/>"
            "</table>"
        ));
        Table::Ptr t = cfg_.parse_table(node.get());
        CPPUNIT_ASSERT_EQUAL(string("A"), NARROW(t->get_name()));
        CPPUNIT_ASSERT_EQUAL(string("bb"), NARROW(t->get_xml_name()));
        CPPUNIT_ASSERT_EQUAL(string("aa"), NARROW(t->get_class_name()));
        CPPUNIT_ASSERT_EQUAL(string("b"), NARROW(t->get_column(_T("B")).get_xml_name()));
        CPPUNIT_ASSERT_EQUAL(string("c"), NARROW(t->get_column(_T("C")).get_prop_name()));
        CPPUNIT_ASSERT_EQUAL(string("xx"), NARROW(t->get_column(_T("B")).get_prop_name()));
        CPPUNIT_ASSERT_EQUAL(string("dd"), NARROW(t->get_column(_T("C")).get_xml_name()));
    }

    void testClassNameDefault()
    {
        Xml::Node node(Xml::Parse(
            "<table name='ABC'>"
            "<column type='longint' name='B'/>"
            "</table>"
        ));
        Table::Ptr t = cfg_.parse_table(node.get());
        CPPUNIT_ASSERT_EQUAL(string("ABC"), NARROW(t->get_name()));
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t->get_class_name()));
        CPPUNIT_ASSERT_EQUAL(string("abc"), NARROW(t->get_xml_name()));
    }

    void testGetWrongNodeValue()
    {
        int a;
        XMLMetaDataConfig::get_node_ptr_value(Xml::Parse("<size>a</size>"), a);
    }
    
    void testStrTypeToInt()
    {
        String a;
        CPPUNIT_ASSERT_EQUAL((int)Value::LONGINT,
                XMLMetaDataConfig::string_type_to_int(String(_T("longint")),a));
        CPPUNIT_ASSERT_EQUAL((int)Value::STRING,
                XMLMetaDataConfig::string_type_to_int(String(_T("string")),a));
        CPPUNIT_ASSERT_EQUAL((int)Value::DECIMAL,
                XMLMetaDataConfig::string_type_to_int(String(_T("decimal")),a));
        CPPUNIT_ASSERT_EQUAL((int)Value::DATETIME,
                    XMLMetaDataConfig::string_type_to_int(String(_T("datetime")),a));
    }
    
    void testWrongColumnType()
    {
        string xml = "<column type='long' name='ID'></column>";
        Column col = XMLMetaDataConfig::fill_column_meta(Xml::Parse(xml));
    }
    
    void testParseColumn()
    {
        string xml = "<column type='longint' name='ID' xml-name='i-d'>"
                     "<read-only/><primary-key/></column>";
        Xml::Node node(Xml::Parse(xml));
        xmlNodePtr p_node = node.get();
        Column col = XMLMetaDataConfig::fill_column_meta(p_node);
        CPPUNIT_ASSERT_EQUAL(string("ID"), NARROW(col.get_name()));
        CPPUNIT_ASSERT_EQUAL(true, col.is_pk());
        CPPUNIT_ASSERT_EQUAL(true, col.is_ro());
        CPPUNIT_ASSERT_EQUAL((int)Value::LONGINT, col.get_type());
        CPPUNIT_ASSERT_EQUAL(string("i-d"), NARROW(col.get_xml_name()));
    }

    void testParseForeignKey()
    {
        string xml = "<foreign-key table='T_INVOICE' key='ID'></foreign-key>";
        Xml::Node node(Xml::Parse(xml));
        String fk_table, fk_field;
        XMLMetaDataConfig::get_foreign_key_data(node.get(), fk_table, fk_field);
        CPPUNIT_ASSERT_EQUAL(string("T_INVOICE"), NARROW(fk_table));
        CPPUNIT_ASSERT_EQUAL(string("ID"), NARROW(fk_field));
    }
    
    void testInvalidCombination()
    {      
        string xml = "<column type='longint' name='ID' size='10' />";
        Xml::Node node(Xml::Parse(xml));
        Column col = XMLMetaDataConfig::fill_column_meta(node.get());
    }

    void testAbsentForeignKeyField()
    {
        string xml = "<foreign-key table='T_INVOICE'></foreign-key>";
        String fk_table, fk_field;
        XMLMetaDataConfig::get_foreign_key_data(Xml::Parse(xml), fk_table, fk_field);
        CPPUNIT_ASSERT(fk_field.empty());
    }

    void testAbsentForeignKeyTable()
    {
        string xml = "<foreign-key key='ID'></foreign-key>";
        String fk_table, fk_field;
        XMLMetaDataConfig::get_foreign_key_data(Xml::Parse(xml), fk_table, fk_field);
    }
    
    void testAbsentColumnName()
    {
        string xml = "<column type='longint'></column>";
        Xml::Node node(Xml::Parse(xml));
        Column col = XMLMetaDataConfig::fill_column_meta(node.get());
    }

    void testAbsentColumnType()
    {
        string xml = "<column name='ID'></column>";
        Xml::Node node(Xml::Parse(xml));
        Column col = XMLMetaDataConfig::fill_column_meta(node.get());
    }

    void testBadXML()
    {
        XMLMetaDataConfig config("not a XML");
    }

    void testRelationSide() {
        Xml::Node node(Xml::Parse(
            "<one class='Abc' property='def' use-list='false' xxx='1'/>"));
        static const Char
            *anames[] = {_T("property"), _T("use-list"), _T("some-other")};
        String cname;
        Relation::AttrMap attrs;
        cfg_.parse_relation_side(node.get(), anames, sizeof(anames)/sizeof(void*),
                cname, attrs);
        CPPUNIT_ASSERT_EQUAL(string("Abc"), NARROW(cname));
        CPPUNIT_ASSERT_EQUAL(2, (int)attrs.size());
        CPPUNIT_ASSERT_EQUAL(string("def"), NARROW(attrs[_T("property")]));
        CPPUNIT_ASSERT_EQUAL(string("false"), NARROW(attrs[_T("use-list")]));
    }

    void testRelationSideNoClass() {
        Xml::Node node(Xml::Parse(
            "<one property='def' use-list='false' xxx='1'/>"));
        static const Char
            *anames[] = {_T("property"), _T("use-list"), _T("some-other")};
        String cname;
        Relation::AttrMap attrs;
        cfg_.parse_relation_side(node.get(), anames, sizeof(anames)/sizeof(void*),
                cname, attrs);
    }

    void testRelationOneToMany() {
        Xml::Node node(Xml::Parse(
            "<relation type='one-to-many'>"
            "<one class='Abc' property='defs'/>"
            "<many class='Def' property='abc'/>"
            "</relation>"
        ));
        Relation::Ptr r = cfg_.parse_relation(node.get());
        CPPUNIT_ASSERT_EQUAL((int)Relation::ONE2MANY, r->type());
        CPPUNIT_ASSERT_EQUAL(string("Abc"), NARROW(r->side(0)));
        CPPUNIT_ASSERT_EQUAL(string("Def"), NARROW(r->side(1)));
        CPPUNIT_ASSERT(r->has_attr(0, _T("property")));
        CPPUNIT_ASSERT(r->has_attr(1, _T("property")));
        CPPUNIT_ASSERT(!r->has_attr(1, _T("xxx")));
        CPPUNIT_ASSERT_EQUAL(string("defs"), NARROW(r->attr(0, _T("property"))));
        CPPUNIT_ASSERT_EQUAL(string("abc"), NARROW(r->attr(1, _T("property"))));
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION(TestXMLConfig);

// vim:ts=4:sts=4:sw=4:et:
