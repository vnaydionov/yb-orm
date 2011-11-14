#ifndef YB__ORM__XML_METADATA_CONFIG__INCLUDED
#define YB__ORM__XML_METADATA_CONFIG__INCLUDED

#include <stdexcept>
#include <util/xmlnode.h>
#include <util/str_utils.hpp>
#include "MetaData.h"

class TestXMLConfig;

namespace Yb {

class XMLConfigError: public std::logic_error {
public:
    XMLConfigError(const String &msg) : std::logic_error(NARROW(msg)) {}
};

class MandatoryAttributeAbsent: public XMLConfigError
{
public:
    MandatoryAttributeAbsent(const String &element, const String &attr)
        : XMLConfigError(_T("Mandatory attribyte '") + attr +
                _T("' not found or empty while parsing element '") + element + _T("'"))
    {}
};

class WrongColumnType: public XMLConfigError
{
public:
    WrongColumnType(const String &type, const String &field_name)
        : XMLConfigError(_T("Type '") + type +
                _T("' is unknown and not supported while parsing field '") + field_name + _T("'"))
    {}
};

class ParseError: public XMLConfigError
{
public:
    ParseError(const String &msg)
        : XMLConfigError(_T("XML config parse error, details: ") + msg)
    {}
};

class InvalidCombination: public XMLConfigError
{
public:
    InvalidCombination(const String &msg)
        : XMLConfigError(_T("Invalid element-attribute combination, details: ") + msg)
    {}
};

class XMLMetaDataConfig
{
public:
    XMLMetaDataConfig(const std::string &xml_string);
    void parse(Schema &reg);
    bool need_generation(const String &table_name) const {
        return std::find(skip_generation_.begin(), skip_generation_.end(), table_name) == skip_generation_.end();
    }
private:
    friend class ::TestXMLConfig;
    void parse_table(xmlNodePtr p_node, Table &table_meta);
    void parse_relation_side(xmlNodePtr p_node, const Char **attr_names,
            size_t attr_count, String &cname, Relation::AttrMap &attrs);
    bool parse_relation(xmlNodePtr p_node, Relation &rel);
    template <typename T>
    static void get_node_ptr_value(xmlNodePtr p_node, T &t);
    template <typename T> 
    static bool get_value_of(xmlNodePtr p_node, const String &field, T &t);
    static int string_type_to_int(const String &type, const String &field_name);
    static bool is_current_child_name(xmlNodePtr p_node, const String &field);
    static void parse_column(const xmlNodePtr p_node, Table &table_meta);
    static Column fill_column_meta(xmlNodePtr p_node);
    static void get_foreign_key_data(xmlNodePtr p_node, String &fk_table, String &fk_field);
    std::vector<String> skip_generation_;
    Xml::Node node_;
};
 
template <typename T>
void XMLMetaDataConfig::get_node_ptr_value(xmlNodePtr p_node, T &t) {
    String str;
    Xml::Node node(p_node, false);
    str = node.GetContent();
    try {
        t = boost::lexical_cast<T>(str);  
    } 
    catch (const boost::bad_lexical_cast &) {
        throw ParseError(String(_T("Wrong element value for '")) +
                WIDEN((const char*)node.get()->name) + _T("'"));
    }
}

template <typename T> 
bool XMLMetaDataConfig::get_value_of(xmlNodePtr p_node, const String &field, T &t)
{
    if (WIDEN((const char *)p_node->name) == field) {
        get_node_ptr_value(p_node, t);
        return true;
    }
    return false;
}

bool load_xml_file(const String &name, std::string &where);

void load_meta(const String &name, Schema &reg);

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__XML_METADATA_CONFIG__INCLUDED
