#include <fstream>
#include <util/xmlnode.h>
#include <util/str_utils.hpp>
#include "Value.h"
#include "XMLMetaDataConfig.h"

using namespace std;
using namespace Xml;

class TestXMLConfig;

namespace Yb {

bool load_xml_file(const string &name, string &where)
{
    ifstream tfile(name.c_str());
    if (!tfile)
        return false;
    while (tfile.good()) {
        char ch;
        tfile.get(ch);
        where.push_back(ch);
    }
    return true;
}

XMLMetaDataConfig::XMLMetaDataConfig(const string &xml_string)
try
    :node_(Parse(xml_string))
{}
catch (const exception &e) {
    throw ParseError(string("XML tree parse error: ") + e.what());
}

void XMLMetaDataConfig::parse(TableMetaDataRegistry &reg)
{
    if(string((const char *)node_.get()->name) != "scheme")
        throw ParseError(string("Unknown element '") + 
                (const char *)node_.get()->name + 
                "' found during parse of root element, 'scheme' expected");

    for(xmlNodePtr child = node_.get()->children; child; child = child->next) {
        if(child->type != XML_ELEMENT_NODE)
            continue;
        if(string((const char *)child->name) != "table")
            throw ParseError(string("Unknown element '") + (const char *)child->name + "' found during parse of element 'scheme'");
        TableMetaData t;
        parse_table(child, t);
        reg.set_table(t);
    }
}

void XMLMetaDataConfig::parse_table(xmlNodePtr p_node, TableMetaData &table_meta)
{
    string sequence_name;
    string name;
    string xml_name;
    bool autoinc = false;

    Node node(p_node, false);

    if(!node.HasNotEmptyAttr("name"))
        throw MandatoryAttributeAbsent("table", "name");

    if(node.HasNotEmptyAttr("sequence"))
        sequence_name = node.GetAttr("sequence");

    if(node.HasAttr("autoinc"))
        autoinc = true;

    name = node.GetAttr("name");

    if(node.HasNotEmptyAttr("nodomainobject")) {
        skip_generation_.push_back(name);
    }
    
    if(node.HasNotEmptyAttr("xml-name"))
        xml_name = node.GetAttr("xml-name");

    table_meta.set_name (name);
    table_meta.set_xml_name(xml_name);
    table_meta.set_seq_name(sequence_name);
    table_meta.set_autoinc(autoinc);
    for(xmlNodePtr child = node.get()->children; child; child = child->next) {
        if(child->type != XML_ELEMENT_NODE)
            continue;
        if(string((const char *)child->name) != "columns")
            throw ParseError(string("Unknown element '") + (const char *)child->name + "' found during parse of element 'table'");
        parse_column(child->children, table_meta);
    }
}

template <typename T>
void XMLMetaDataConfig::get_node_ptr_value(xmlNodePtr p_node, T &t) {
    string str;
    Node node(p_node, false);
    str = node.GetContent();
    try {
        t = boost::lexical_cast<T>(str);  
    } 
    catch(const boost::bad_lexical_cast &) {
        throw ParseError(string("Wrong element value for '")+ (const char*)node.get()->name + "'");
    }
}

    template <typename T> 
bool XMLMetaDataConfig::get_value_of(xmlNodePtr p_node, const string &field, T &t)
{
    if(string((const char *)p_node->name) == field) {
        get_node_ptr_value(p_node, t);
        return true;
    }
    return false;
}
int XMLMetaDataConfig::string_type_to_int(const string &type, const string &field_name)
{
    if(Yb::StrUtils::str_to_lower(type) == string("longint"))
        return Value::LONGINT;
    else if(Yb::StrUtils::str_to_lower(type) == string("string"))
        return Value::STRING;
    else if(Yb::StrUtils::str_to_lower(type) == "decimal")
        return Value::DECIMAL;
    else if(Yb::StrUtils::str_to_lower(type) == "datetime")
        return Value::DATETIME;
    else
        throw WrongColumnType(type, field_name);
}

bool XMLMetaDataConfig::is_current_child_name(xmlNodePtr p_node, const string &field)
{
    return (string((const char *)p_node->name) == field) ? true : false;
}

void XMLMetaDataConfig::parse_column(const xmlNodePtr p_node, TableMetaData &table_meta)
{
    for(xmlNodePtr col = p_node; col; col = col->next) {
        if(col->type != XML_ELEMENT_NODE)
            continue;
        if(string((const char *)col->name) != "column")
            throw ParseError(string("Unknown element '") + (const char *)col->name + "' found during parse of element 'columns'");
        table_meta.set_column(fill_column_meta(col));
    }
}

ColumnMetaData XMLMetaDataConfig::fill_column_meta(xmlNodePtr p_node)
{
    string name, type, fk_table, fk_field, xml_name = "";
    int flags = 0, size = 0, col_type = 0;
    Xml::Node node(Xml::DupNode(p_node));
    Yb::Value default_val;
    if (!node.HasNotEmptyAttr("name"))
        throw MandatoryAttributeAbsent("column", "name");
    else
        name = node.GetAttr("name");

    if (!node.HasNotEmptyAttr("type"))
        throw MandatoryAttributeAbsent("column", "type");
    else {
        type = node.GetAttr("type");
        col_type = string_type_to_int(type, name);
    }

    if (node.HasAttr("nullable")) {
        if (col_type == Value::STRING)
            throw InvalidCombination("nullable attribute cannot be used for strings");
        flags |= ColumnMetaData::NULLABLE;
    }    
   
    if (node.HasNotEmptyAttr("default")) {
       string value = node.GetAttr("default");
        switch (col_type) {
            case Value::DECIMAL:
            case Value::LONGINT:
                try {
                    default_val = Value(boost::lexical_cast<LongInt>(value));
                } catch(const boost::bad_lexical_cast &) {
                        throw ParseError(string("Wrong default value '") + value + "' for integer element '" + name + "'");
                }
                break;
            case Value::DATETIME:
                if (Yb::StrUtils::str_to_lower(node.GetAttr("default")) != string("sysdate"))
                    throw ParseError(string("Wrong default value for datetime element '")+ name + "'");
                default_val = Value("sysdate");
                break;
            default:
                default_val = Value(node.GetAttr("default"));
        }
    }
    
    for(xmlNode *child = node.get()->children; child; child = child->next) {
        if(child->type != XML_ELEMENT_NODE)
            continue;
        get_value_of(child, "size", size);
        get_value_of(child, "xml-name", xml_name);
        if(is_current_child_name(child, "readonly"))
            flags |= ColumnMetaData::RO;
        if(is_current_child_name(child, "primarykey"))
            flags |= ColumnMetaData::PK;
        if (string((const char *)child->name) == "foreign-key") {
            get_foreign_key_data(child, fk_table, fk_field);
        }
    }

    if((size > 0) && (col_type != Value::STRING))
        throw InvalidCombination("Size musn't me used for not a string type");
    ColumnMetaData result(name, col_type, size, flags, fk_table, fk_field, xml_name);               
    result.set_default_value(default_val);
    return result;
}

void XMLMetaDataConfig::get_foreign_key_data(xmlNodePtr p_node, string &fk_table, string &fk_field)
{
    Node node(p_node, false);
    if(!node.HasNotEmptyAttr("table"))
        throw MandatoryAttributeAbsent("foreign-key", "table");
    else
        fk_table = node.GetAttr("table");

    if(!node.HasNotEmptyAttr("field"))
        throw MandatoryAttributeAbsent("foreign-key", "field");
    else
        fk_field = node.GetAttr("field");
}    

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
