#ifndef YB__ORM__XML_METADATA_CONFIG__INCLUDED
#define YB__ORM__XML_METADATA_CONFIG__INCLUDED

#include <stdexcept>
#include <util/xmlnode.h>
#include <util/str_utils.hpp>
#include "MetaData.h"

class TestXMLConfig;

namespace Yb {

class XMLConfigError: public std::logic_error
{
public:
    XMLConfigError(const std::string &msg) :
#if defined(__BORLANDC__)
        std::logic_error
#else
        logic_error
#endif
        (msg)
    {}
};

class MandatoryAttributeAbsent: public XMLConfigError
{
public:
    MandatoryAttributeAbsent(const std::string &element, const std::string &attr)
        : XMLConfigError("Mandatory attribyte '" + attr +
                "' not found or empty while parsing element '" + element + "'")
    {}
};

class WrongColumnType: public XMLConfigError
{
public:
    WrongColumnType(const std::string &type, const std::string &field_name)
        : XMLConfigError("Type '" + type +
                "' is unknown and not supported while parsing field '" + field_name + "'")
    {}
};

class ParseError: public XMLConfigError
{
public:
    ParseError(const std::string &msg)
        : XMLConfigError("XML config parse error, details: " + msg)
    {}
};

class InvalidCombination: public XMLConfigError
{
public:
    InvalidCombination(const std::string &msg)
        : XMLConfigError("Invalid element-attribute combination, details: " + msg)
    {}
};

class XMLMetaDataConfig
{
public:
    XMLMetaDataConfig(const std::string &xml_string);
    void parse(TableMetaDataRegistry &reg);
    bool need_generation(const std::string &table_name) const {
        return std::find(skip_generation_.begin(), skip_generation_.end(), table_name) == skip_generation_.end();
    }
private:
    friend class ::TestXMLConfig;
    void parse_table(xmlNodePtr p_node, TableMetaData &table_meta);
    template <typename T>
    static void get_node_ptr_value(xmlNodePtr p_node, T &t);
    template <typename T> 
    static bool get_value_of(xmlNodePtr p_node, const std::string &field, T &t);
    static int string_type_to_int(const std::string &type, const std::string &field_name);
    static bool is_current_child_name(xmlNodePtr p_node, const std::string &field);
    static void parse_column(const xmlNodePtr p_node, TableMetaData &table_meta);
    static ColumnMetaData fill_column_meta(xmlNodePtr p_node);
    static void get_foreign_key_data(xmlNodePtr p_node, std::string &fk_table, std::string &fk_field);
    std::vector<std::string> skip_generation_;
    Xml::Node node_;
};
 
template <typename T>
void XMLMetaDataConfig::get_node_ptr_value(xmlNodePtr p_node, T &t) {
    std::string str;
    Xml::Node node(p_node, false);
    str = node.GetContent();
    try {
        t = boost::lexical_cast<T>(str);  
    } 
    catch(const boost::bad_lexical_cast &) {
        throw ParseError(std::string("Wrong element value for '")+ (const char*)node.get()->name + "'");
    }
}

template <typename T> 
bool XMLMetaDataConfig::get_value_of(xmlNodePtr p_node, const std::string &field, T &t)
{
    if(std::string((const char *)p_node->name) == field) {
        get_node_ptr_value(p_node, t);
        return true;
    }
    return false;
}

bool load_xml_file(const std::string &name, std::string &where);

void load_meta(const std::string &name, TableMetaDataRegistry &reg);

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__XML_METADATA_CONFIG__INCLUDED
