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
    CPPUNIT_TEST(testSerialize);
    CPPUNIT_TEST(testSerialize2);
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
        ElementTree::ElementPtr node(ElementTree::parse(xml));
        Table::Ptr t = cfg_.parse_table(node);
    }

    void testParseTable()
    {
        string xml = 
            "<table name='A' sequence='S'>"
            "<column type='string' name='ASTR' size='10' />"
            "<column type='longint' name='B_ID'>"
            "<foreign-key table='T_B' key='ID'/></column>"
            "</table>";
        ElementTree::ElementPtr node(ElementTree::parse(xml));
        Table::Ptr t = cfg_.parse_table(node);
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
        ElementTree::ElementPtr node(ElementTree::parse(
            "<table name='A'>"
            "<column type='longint' name='B'>"
            "<primary-key/>"
            "</column>"
            "</table>"
        ));
        Table::Ptr t = cfg_.parse_table(node);
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t->get_seq_name()));
        CPPUNIT_ASSERT_EQUAL(false, t->get_autoinc());
    }

    void testAutoInc()
    {
        ElementTree::ElementPtr node(ElementTree::parse(
            "<table name='A' autoinc='autoinc'>"
            "<column type='longint' name='B'>"
            "<primary-key/>"
            "</column>"
            "</table>"
        ));
        Table::Ptr t = cfg_.parse_table(node);
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t->get_seq_name()));
        CPPUNIT_ASSERT_EQUAL(true, t->get_autoinc());
    }

    void testNullable()
    {
        ElementTree::ElementPtr node(ElementTree::parse(
            "<table name='A'>"
            "<column type='longint' name='B'>"
            "<primary-key/>"
            "</column>"
            "<column type='datetime' name='C'/>"
            "<column type='decimal' name='D' null='false'/>"
            "</table>"
        ));
        Table::Ptr t = cfg_.parse_table(node);
        CPPUNIT_ASSERT_EQUAL(false, t->get_column(_T("B")).is_nullable());
        CPPUNIT_ASSERT_EQUAL(true, t->get_column(_T("C")).is_nullable());
        CPPUNIT_ASSERT_EQUAL(false, t->get_column(_T("D")).is_nullable());
    }

    void testClassName()
    {
        ElementTree::ElementPtr node(ElementTree::parse(
            "<table name='A' class='aa' xml-name='bb'>"
            "<column type='longint' name='B' property='xx'/>"
            "<column type='longint' name='C' xml-name='dd'/>"
            "</table>"
        ));
        Table::Ptr t = cfg_.parse_table(node);
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
        ElementTree::ElementPtr node(ElementTree::parse(
            "<table name='ABC'>"
            "<column type='longint' name='B'/>"
            "</table>"
        ));
        Table::Ptr t = cfg_.parse_table(node);
        CPPUNIT_ASSERT_EQUAL(string("ABC"), NARROW(t->get_name()));
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t->get_class_name()));
        CPPUNIT_ASSERT_EQUAL(string("abc"), NARROW(t->get_xml_name()));
    }

    void testGetWrongNodeValue()
    {
        int a;
        XMLMetaDataConfig::get_node_ptr_value(
                ElementTree::parse("<size>a</size>"), a);
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
        Column col = XMLMetaDataConfig::fill_column_meta(ElementTree::parse(xml));
    }
    
    void testParseColumn()
    {
        string xml = "<column type='longint' name='ID' xml-name='i-d'>"
                     "<read-only/><primary-key/></column>";
        ElementTree::ElementPtr node(ElementTree::parse(xml));
        Column col = XMLMetaDataConfig::fill_column_meta(node);
        CPPUNIT_ASSERT_EQUAL(string("ID"), NARROW(col.get_name()));
        CPPUNIT_ASSERT_EQUAL(true, col.is_pk());
        CPPUNIT_ASSERT_EQUAL(true, col.is_ro());
        CPPUNIT_ASSERT_EQUAL((int)Value::LONGINT, col.get_type());
        CPPUNIT_ASSERT_EQUAL(string("i-d"), NARROW(col.get_xml_name()));
    }

    void testParseForeignKey()
    {
        string xml = "<foreign-key table='T_INVOICE' key='ID'></foreign-key>";
        ElementTree::ElementPtr node(ElementTree::parse(xml));
        String fk_table, fk_field;
        XMLMetaDataConfig::get_foreign_key_data(node, fk_table, fk_field);
        CPPUNIT_ASSERT_EQUAL(string("T_INVOICE"), NARROW(fk_table));
        CPPUNIT_ASSERT_EQUAL(string("ID"), NARROW(fk_field));
    }
    
    void testInvalidCombination()
    {      
        string xml = "<column type='longint' name='ID' size='10' />";
        ElementTree::ElementPtr node(ElementTree::parse(xml));
        Column col = XMLMetaDataConfig::fill_column_meta(node);
    }

    void testAbsentForeignKeyField()
    {
        string xml = "<foreign-key table='T_INVOICE'></foreign-key>";
        String fk_table, fk_field;
        XMLMetaDataConfig::get_foreign_key_data(ElementTree::parse(xml),
                fk_table, fk_field);
        CPPUNIT_ASSERT(fk_field.empty());
    }

    void testAbsentForeignKeyTable()
    {
        string xml = "<foreign-key key='ID'></foreign-key>";
        String fk_table, fk_field;
        XMLMetaDataConfig::get_foreign_key_data(ElementTree::parse(xml),
                fk_table, fk_field);
    }
    
    void testAbsentColumnName()
    {
        string xml = "<column type='longint'></column>";
        ElementTree::ElementPtr node(ElementTree::parse(xml));
        Column col = XMLMetaDataConfig::fill_column_meta(node);
    }

    void testAbsentColumnType()
    {
        string xml = "<column name='ID'></column>";
        ElementTree::ElementPtr node(ElementTree::parse(xml));
        Column col = XMLMetaDataConfig::fill_column_meta(node);
    }

    void testBadXML()
    {
        XMLMetaDataConfig config("not a XML");
    }

    void testRelationSide() {
        ElementTree::ElementPtr node(ElementTree::parse(
            "<one class='Abc' property='def' use-list='false' xxx='1'/>"));
        static const Char
            *anames[] = {_T("property"), _T("use-list"), _T("some-other")};
        String cname;
        Relation::AttrMap attrs;
        cfg_.parse_relation_side(node, anames, sizeof(anames)/sizeof(void*),
                cname, attrs);
        CPPUNIT_ASSERT_EQUAL(string("Abc"), NARROW(cname));
        CPPUNIT_ASSERT_EQUAL(2, (int)attrs.size());
        CPPUNIT_ASSERT_EQUAL(string("def"), NARROW(attrs[_T("property")]));
        CPPUNIT_ASSERT_EQUAL(string("false"), NARROW(attrs[_T("use-list")]));
    }

    void testRelationSideNoClass() {
        ElementTree::ElementPtr node(ElementTree::parse(
            "<one property='def' use-list='false' xxx='1'/>"));
        static const Char
            *anames[] = {_T("property"), _T("use-list"), _T("some-other")};
        String cname;
        Relation::AttrMap attrs;
        cfg_.parse_relation_side(node, anames, sizeof(anames)/sizeof(void*),
                cname, attrs);
    }

    void testRelationOneToMany() {
        ElementTree::ElementPtr node(ElementTree::parse(
            "<relation type='one-to-many'>"
            "<one class='Abc' property='defs'/>"
            "<many class='Def' property='abc'/>"
            "</relation>"
        ));
        Relation::Ptr r = cfg_.parse_relation(node);
        CPPUNIT_ASSERT_EQUAL((int)Relation::ONE2MANY, r->type());
        CPPUNIT_ASSERT_EQUAL(string("Abc"), NARROW(r->side(0)));
        CPPUNIT_ASSERT_EQUAL(string("Def"), NARROW(r->side(1)));
        CPPUNIT_ASSERT(r->has_attr(0, _T("property")));
        CPPUNIT_ASSERT(r->has_attr(1, _T("property")));
        CPPUNIT_ASSERT(!r->has_attr(1, _T("xxx")));
        CPPUNIT_ASSERT_EQUAL(string("defs"), NARROW(r->attr(0, _T("property"))));
        CPPUNIT_ASSERT_EQUAL(string("abc"), NARROW(r->attr(1, _T("property"))));
    }

    void testSerialize() {
        string xml =
            "<relation type=\"one-to-many\">"
            "<one class=\"Abc\" property=\"defs\"/>"
            "<many class=\"Def\" property=\"abc\"/>"
            "</relation>\n";
        ElementTree::ElementPtr node(ElementTree::parse(xml));
        CPPUNIT_ASSERT_EQUAL(xml, node->serialize());
    }

    void testSerialize2() {
        string xml =
            "<list aaa=\"&lt;\" bbb=\"&amp;\">qwerty"
            "<li ccc=\"x&gt;\"/>asdfg"
            "<li>zxcvb</li>"
            "<li><nested n=\"n\">111</nested></li>"
            "</list>\n";
        ElementTree::ElementPtr node(ElementTree::parse(xml));
        CPPUNIT_ASSERT_EQUAL(string("qwertyasdfgzxcvb111"), node->get_text());
        CPPUNIT_ASSERT_EQUAL(xml, node->serialize());
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestXMLConfig);

// vim:ts=6:sts=4:sw=4:et:
