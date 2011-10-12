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
        CPPUNIT_ASSERT_EQUAL(2, (int)reg.size());
        const Table &t = reg.get_table("A");
        CPPUNIT_ASSERT_EQUAL(1, (int)t.size());
        CPPUNIT_ASSERT_EQUAL(string("AA"), t.begin()->get_name());
        CPPUNIT_ASSERT_EQUAL(10, (int)t.begin()->get_size());
        CPPUNIT_ASSERT_EQUAL((int)Value::STRING, t.begin()->get_type());
        const Table &t2 = reg.get_table("B");
        CPPUNIT_ASSERT_EQUAL(1, (int)t2.size());
        CPPUNIT_ASSERT_EQUAL(string("BA"), t2.begin()->get_name());
        CPPUNIT_ASSERT_EQUAL((int)Value::LONGINT, t2.begin()->get_type());

        CPPUNIT_ASSERT_EQUAL(1, (int)reg.rel_count());
        CPPUNIT_ASSERT_EQUAL((int)Relation::ONE2MANY, reg.rel_begin()->type());
    }
    
    void testWrongElementTable()
    {
        Table t;
        string xml =  "<table name='A' sequence='S'><col></col></table>";
        Xml::Node node(Xml::Parse(xml));
        cfg_.parse_table(node.get(), t);
    }

    void testParseTable()
    {
        Table t;
        string xml = 
            "<table name='A' sequence='S'>"
            "<column type='string' name='ASTR' size='10' />"
            "<column type='longint' name='B_ID'>"
            "<foreign-key table='T_B' key='ID'/></column>"
            "</table>";
        Xml::Node node(Xml::Parse(xml));
        cfg_.parse_table(node.get(), t);
        CPPUNIT_ASSERT_EQUAL(string("A"), t.get_name());
        CPPUNIT_ASSERT_EQUAL(string("S"), t.get_seq_name());
        CPPUNIT_ASSERT_EQUAL(2, (int)t.size());
        Columns::const_iterator it = t.begin();
        CPPUNIT_ASSERT_EQUAL(string("ASTR"), it->get_name());
        CPPUNIT_ASSERT_EQUAL(10, (int)it->get_size());
        CPPUNIT_ASSERT_EQUAL((int)Value::STRING, it->get_type());
        ++it;
        CPPUNIT_ASSERT_EQUAL(string("B_ID"), it->get_name());
        CPPUNIT_ASSERT_EQUAL(string("ID"), it->get_fk_name());
        CPPUNIT_ASSERT_EQUAL(string("T_B"), it->get_fk_table_name());
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
        Table t;
        cfg_.parse_table(node.get(), t);
        CPPUNIT_ASSERT_EQUAL(string(""), t.get_seq_name());
        CPPUNIT_ASSERT_EQUAL(false, t.get_autoinc());
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
        Table t;
        cfg_.parse_table(node.get(), t);
        CPPUNIT_ASSERT_EQUAL(string(""), t.get_seq_name());
        CPPUNIT_ASSERT_EQUAL(true, t.get_autoinc());
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
        Table t;
        cfg_.parse_table(node.get(), t);
        CPPUNIT_ASSERT_EQUAL(false, t.get_column("B").is_nullable());
        CPPUNIT_ASSERT_EQUAL(true, t.get_column("C").is_nullable());
        CPPUNIT_ASSERT_EQUAL(false, t.get_column("D").is_nullable());
    }

    void testClassName()
    {
        Xml::Node node(Xml::Parse(
            "<table name='A' class='aa' xml-name='bb'>"
            "<column type='longint' name='B' property='xx'/>"
            "<column type='longint' name='C' xml-name='dd'/>"
            "</table>"
        ));
        Table t;
        cfg_.parse_table(node.get(), t);
        CPPUNIT_ASSERT_EQUAL(string("A"), t.get_name());
        CPPUNIT_ASSERT_EQUAL(string("bb"), t.get_xml_name());
        CPPUNIT_ASSERT_EQUAL(string("aa"), t.get_class_name());
        CPPUNIT_ASSERT_EQUAL(string("b"), t.get_column("B").get_xml_name());
        CPPUNIT_ASSERT_EQUAL(string("c"), t.get_column("C").get_prop_name());
        CPPUNIT_ASSERT_EQUAL(string("xx"), t.get_column("B").get_prop_name());
        CPPUNIT_ASSERT_EQUAL(string("dd"), t.get_column("C").get_xml_name());
    }

    void testClassNameDefault()
    {
        Xml::Node node(Xml::Parse(
            "<table name='ABC'>"
            "<column type='longint' name='B'/>"
            "</table>"
        ));
        Table t;
        cfg_.parse_table(node.get(), t);
        CPPUNIT_ASSERT_EQUAL(string("ABC"), t.get_name());
        CPPUNIT_ASSERT_EQUAL(string(""), t.get_class_name());
        CPPUNIT_ASSERT_EQUAL(string("abc"), t.get_xml_name());
    }

    void testGetWrongNodeValue()
    {
        int a;
        XMLMetaDataConfig::get_node_ptr_value(Xml::Parse("<size>a</size>"), a);
    }
    
    void testStrTypeToInt()
    {
        string a;
        CPPUNIT_ASSERT_EQUAL((int)Value::LONGINT,
                XMLMetaDataConfig::string_type_to_int(string("longint"),a));
        CPPUNIT_ASSERT_EQUAL((int)Value::STRING,
                XMLMetaDataConfig::string_type_to_int(string("string"),a));
        CPPUNIT_ASSERT_EQUAL((int)Value::DECIMAL,
                XMLMetaDataConfig::string_type_to_int(string("decimal"),a));
        CPPUNIT_ASSERT_EQUAL((int)Value::DATETIME,
                    XMLMetaDataConfig::string_type_to_int(string("datetime"),a));
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
        CPPUNIT_ASSERT_EQUAL(string("ID"), col.get_name());
        CPPUNIT_ASSERT_EQUAL(true, col.is_pk());
        CPPUNIT_ASSERT_EQUAL(true, col.is_ro());
        CPPUNIT_ASSERT_EQUAL((int)Value::LONGINT, col.get_type());
        CPPUNIT_ASSERT_EQUAL(string("i-d"), col.get_xml_name());
    }

    void testParseForeignKey()
    {
        string xml = "<foreign-key table='T_INVOICE' key='ID'></foreign-key>";
        Xml::Node node(Xml::Parse(xml));
        string fk_table, fk_field;
        XMLMetaDataConfig::get_foreign_key_data(node.get(), fk_table, fk_field);
        CPPUNIT_ASSERT_EQUAL(string("T_INVOICE"), fk_table);
        CPPUNIT_ASSERT_EQUAL(string("ID"), fk_field);     
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
        string fk_table, fk_field;
        XMLMetaDataConfig::get_foreign_key_data(Xml::Parse(xml), fk_table, fk_field);
        CPPUNIT_ASSERT_EQUAL(string(""), fk_field);
    }

    void testAbsentForeignKeyTable()
    {
        string xml = "<foreign-key key='ID'></foreign-key>";
        string fk_table, fk_field;
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
        static const char
            *anames[] = {"property", "use-list", "some-other"};
        string cname;
        Relation::AttrMap attrs;
        cfg_.parse_relation_side(node.get(), anames, sizeof(anames)/sizeof(void*),
                cname, attrs);
        CPPUNIT_ASSERT_EQUAL(string("Abc"), cname);
        CPPUNIT_ASSERT_EQUAL(2, (int)attrs.size());
        CPPUNIT_ASSERT_EQUAL(string("def"), attrs["property"]);
        CPPUNIT_ASSERT_EQUAL(string("false"), attrs["use-list"]);
    }

    void testRelationSideNoClass() {
        Xml::Node node(Xml::Parse(
            "<one property='def' use-list='false' xxx='1'/>"));
        static const char
            *anames[] = {"property", "use-list", "some-other"};
        string cname;
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
        Relation r;
        cfg_.parse_relation(node.get(), r);
        CPPUNIT_ASSERT_EQUAL((int)Relation::ONE2MANY, r.type());
        CPPUNIT_ASSERT_EQUAL(string("Abc"), r.side(0));
        CPPUNIT_ASSERT_EQUAL(string("Def"), r.side(1));
        CPPUNIT_ASSERT(r.has_attr(0, "property"));
        CPPUNIT_ASSERT(r.has_attr(1, "property"));
        CPPUNIT_ASSERT(!r.has_attr(1, "xxx"));
        CPPUNIT_ASSERT_EQUAL(string("defs"), r.attr(0, "property"));
        CPPUNIT_ASSERT_EQUAL(string("abc"), r.attr(1, "property"));
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION(TestXMLConfig);

// vim:ts=4:sts=4:sw=4:et:
