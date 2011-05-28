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
    CPPUNIT_TEST_EXCEPTION(testAbsentForeignKeyField, MandatoryAttributeAbsent);
    CPPUNIT_TEST_EXCEPTION(testAbsentForeignKeyTable, MandatoryAttributeAbsent);
    CPPUNIT_TEST_EXCEPTION(testAbsentColumnName, MandatoryAttributeAbsent);
    CPPUNIT_TEST_EXCEPTION(testAbsentColumnType, MandatoryAttributeAbsent);
    CPPUNIT_TEST_EXCEPTION(testWrongColumnType, WrongColumnType);
    CPPUNIT_TEST_EXCEPTION(testGetWrongNodeValue, ParseError);
    CPPUNIT_TEST(testParseTable);
    CPPUNIT_TEST_EXCEPTION(testWrongElementTable, ParseError);
    CPPUNIT_TEST(testParseSchema);
    CPPUNIT_TEST_SUITE_END();
public:
    void testParseSchema()
    {
        string xml = 
            "<schema>"
            "<table name=\"A\">"
            "<column type=\"string\" name=\"AA\" size=\"10\" />"
            "</table>"
            "<table name=\"B\">"
            "<column type=\"longint\" name=\"BA\"></column>"
            "</table>"
            "</schema>";
        XMLMetaDataConfig cfg(xml);
        Schema reg;
        cfg.parse(reg);
        CPPUNIT_ASSERT_EQUAL(2, (int)reg.size());
        const Table &t = reg.get_table("A");
        CPPUNIT_ASSERT_EQUAL(1, (int)t.size());
        CPPUNIT_ASSERT_EQUAL(std::string("AA"), t.begin()->second.get_name());
        CPPUNIT_ASSERT_EQUAL(10, (int)t.begin()->second.get_size());
        CPPUNIT_ASSERT_EQUAL((int)Value::STRING, t.begin()->second.get_type());
        const Table &t2 = reg.get_table("B");
        CPPUNIT_ASSERT_EQUAL(1, (int)t2.size());
        CPPUNIT_ASSERT_EQUAL(std::string("BA"), t2.begin()->second.get_name());
        CPPUNIT_ASSERT_EQUAL((int)Value::LONGINT, t2.begin()->second.get_type());
    }
    
    void testWrongElementTable()
    {
        Table t;
        string xml =  "<table name=\"A\" sequence=\"S\"><col></col></table>";
        Xml::Node node(Xml::Parse(xml));
        XMLMetaDataConfig cfg("");
        cfg.parse_table(node.get(), t);
    }
    
    void testParseTable()
    {
        Table t;
        string xml = 
            "<table name=\"A\" sequence=\"S\">"
            "<column type=\"string\" name=\"ASTR\" size=\"10\" />"
            "<column type=\"longint\" name=\"B_ID\">"
            "<foreign-key table=\"T_B\" field=\"ID\"/></column>"
            "</table>";
        Xml::Node node(Xml::Parse(xml));

        XMLMetaDataConfig cfg("<empty/>");
        cfg.parse_table(node.get(), t);
        CPPUNIT_ASSERT_EQUAL(std::string("A"), t.get_name());
        CPPUNIT_ASSERT_EQUAL(std::string("S"), t.get_seq_name());
        CPPUNIT_ASSERT_EQUAL(2, (int)t.size());
        Table::Map::const_iterator it = t.begin();
        CPPUNIT_ASSERT_EQUAL(string("ASTR"), it->second.get_name());
        CPPUNIT_ASSERT_EQUAL(10, (int)it->second.get_size());
        CPPUNIT_ASSERT_EQUAL((int)Value::STRING, it->second.get_type());
        ++it;
        CPPUNIT_ASSERT_EQUAL(string("B_ID"), it->second.get_name());
        CPPUNIT_ASSERT_EQUAL(string("ID"), it->second.get_fk_name());
        CPPUNIT_ASSERT_EQUAL(string("T_B"), it->second.get_fk_table_name());
        CPPUNIT_ASSERT_EQUAL((int)Value::LONGINT, it->second.get_type()); 
    }
    
    void testGetWrongNodeValue()
    {
        int a;
        XMLMetaDataConfig::get_node_ptr_value(Xml::Parse("<size>a</size>"), a);
    }
    
    void testStrTypeToInt()
    {
        string a;
        CPPUNIT_ASSERT_EQUAL((int)Value::LONGINT, XMLMetaDataConfig::string_type_to_int(std::string("longint"),a));
        CPPUNIT_ASSERT_EQUAL((int)Value::STRING, XMLMetaDataConfig::string_type_to_int(std::string("string"),a));
        CPPUNIT_ASSERT_EQUAL((int)Value::DECIMAL, XMLMetaDataConfig::string_type_to_int(std::string("decimal"),a));
        CPPUNIT_ASSERT_EQUAL((int)Value::DATETIME, XMLMetaDataConfig::string_type_to_int(std::string("datetime"),a));
    }
    
    void testWrongColumnType()
    {
        string xml = "<column type=\"long\" name=\"ID\"></column>";
        Column col = XMLMetaDataConfig::fill_column_meta(Xml::Parse(xml));
    }
    
    void testParseColumn()
    {
        string xml = "<column type=\"longint\" name=\"ID\"><xml-name>i-d</xml-name>"
                     "<readonly/><primarykey/></column>";
        Xml::Node node(Xml::Parse(xml));
        xmlNodePtr p_node = node.get();
        Column col = XMLMetaDataConfig::fill_column_meta(p_node);
        CPPUNIT_ASSERT_EQUAL(std::string("ID"), col.get_name());
        CPPUNIT_ASSERT_EQUAL(true, col.is_pk());
        CPPUNIT_ASSERT_EQUAL(true, col.is_ro());
        CPPUNIT_ASSERT_EQUAL((int)Value::LONGINT, col.get_type());
        CPPUNIT_ASSERT_EQUAL(std::string("i-d"), col.get_xml_name());
    }

    void testParseForeignKey()
    {
        string xml = "<foreign-key table=\"T_INVOICE\" field=\"ID\"></foreign-key>";
        Xml::Node node(Xml::Parse(xml));
        std::string fk_table, fk_field;
        XMLMetaDataConfig::get_foreign_key_data(node.get(), fk_table, fk_field);
        CPPUNIT_ASSERT_EQUAL(std::string("T_INVOICE"), fk_table);
        CPPUNIT_ASSERT_EQUAL(std::string("ID"), fk_field);     
    }
    
    void testInvalidCombination()
    {      
        string xml = "<column type=\"longint\" name=\"ID\" size=\"10\" />";
        Xml::Node node(Xml::Parse(xml));
        Column col = XMLMetaDataConfig::fill_column_meta(node.get());
    }

    void testAbsentForeignKeyField()
    {
        string xml = "<foreign-key table=\"T_INVOICE\"></foreign-key>";
        std::string fk_table, fk_field;
        XMLMetaDataConfig::get_foreign_key_data(Xml::Parse(xml), fk_table, fk_field);
    }

    void testAbsentForeignKeyTable()
    {
        string xml = "<foreign-key field=\"ID\"></foreign-key>";
        std::string fk_table, fk_field;
        XMLMetaDataConfig::get_foreign_key_data(Xml::Parse(xml), fk_table, fk_field);
    }
    
    void testAbsentColumnName()
    {
        string xml = "<column type=\"longint\"></column>";
        Xml::Node node(Xml::Parse(xml));
        Column col = XMLMetaDataConfig::fill_column_meta(node.get());
    }

    void testAbsentColumnType()
    {
        string xml = "<column name=\"ID\"></column>";
        Xml::Node node(Xml::Parse(xml));
        Column col = XMLMetaDataConfig::fill_column_meta(node.get());
    }

    void testBadXML()
    {
        XMLMetaDataConfig config("not a XML");
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestXMLConfig);

// vim:ts=4:sts=4:sw=4:et:
