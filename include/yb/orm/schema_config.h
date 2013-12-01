#ifndef YB__ORM__XML_METADATA_CONFIG__INCLUDED
#define YB__ORM__XML_METADATA_CONFIG__INCLUDED

#include <stdexcept>
#include <algorithm>
#include <util/ElementTree.h>
#include <util/str_utils.hpp>
#include <orm/MetaData.h>

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

class MetaDataConfig
{
public:
    MetaDataConfig(const std::string &xml_string);
    void parse(Schema &reg);
    bool need_generation(const String &table_name) const {
        return std::find(skip_generation_.begin(), skip_generation_.end(), table_name) == skip_generation_.end();
    }
private:
    friend class ::TestXMLConfig;
    Table::Ptr parse_table(ElementTree::ElementPtr node);
    void parse_relation_side(ElementTree::ElementPtr node, const char **attr_names,
            size_t attr_count, String &cname, Relation::AttrMap &attrs);
    Relation::Ptr parse_relation(ElementTree::ElementPtr node);
    template <typename T>
    static void get_node_ptr_value(ElementTree::ElementPtr node, T &t);
    template <typename T> 
    static bool get_value_of(ElementTree::ElementPtr node, const String &field, T &t);
    static int string_type_to_int(const String &type, const String &field_name);
    static void parse_column(ElementTree::ElementPtr node, Table &table_meta);
    static Column fill_column_meta(ElementTree::ElementPtr node);
    static void get_foreign_key_data(ElementTree::ElementPtr node, String &fk_table, String &fk_field);
    std::vector<String> skip_generation_;
    ElementTree::ElementPtr node_;
};
 
template <typename T>
void MetaDataConfig::get_node_ptr_value(ElementTree::ElementPtr node, T &t) {
    String str = node->get_text();
    try {
        from_string(str, t);
    } 
    catch (const std::exception &) {
        throw ParseError(_T("Wrong element value for '") + node->name_ + _T("'"));
    }
}

template <typename T> 
bool MetaDataConfig::get_value_of(ElementTree::ElementPtr node, const String &field, T &t)
{
    if (node->name_ == field) {
        get_node_ptr_value(node, t);
        return true;
    }
    return false;
}

bool load_xml_file(const String &name, std::string &where);

void load_schema(const String &name, Schema &reg, bool check = true);

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__XML_METADATA_CONFIG__INCLUDED
