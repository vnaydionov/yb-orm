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

void load_meta(const string &name, Schema &reg)
{
    string xml;
    if (!load_xml_file(name, xml))
        throw XMLConfigError("Can't read file: " + name);
    XMLMetaDataConfig xml_config(xml);
    xml_config.parse(reg);
    reg.fill_fkeys();
}

XMLMetaDataConfig::XMLMetaDataConfig(const string &xml_string)
    :node_(NULL)
{
    try {
        node_.set(Parse(xml_string));
    }
    catch (const exception &e) {
        throw ParseError(string("XML tree parse error: ") + e.what());
    }
}

void XMLMetaDataConfig::parse(Schema &reg)
{
    if (strcmp((const char *)node_.get()->name, "schema"))
        throw ParseError(string("Unknown element '") + 
                (const char *)node_.get()->name + 
                "' found during parse of root element, 'schema' expected");

    for (xmlNodePtr child = node_.get()->children; child; child = child->next) {
        if (child->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *)child->name, "table")) {
            Table t;
            parse_table(child, t);
            reg.set_table(t);
        } else if (!strcmp((const char *)child->name, "relation")) {
            Relation r;
            if (parse_relation(child, r))
                reg.add_relation(r);
        } else
            throw ParseError(string("Unknown element '") +
                    (const char *)child->name +
                    "' found during parse of element 'schema'");
    }
}

void XMLMetaDataConfig::parse_table(xmlNodePtr p_node, Table &table_meta)
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

    if(node.HasNotEmptyAttr("xml-name"))
        xml_name = node.GetAttr("xml-name");
    else
        xml_name = mk_xml_name(name, "");

    table_meta.set_name (name);
    table_meta.set_xml_name(xml_name);
    table_meta.set_seq_name(sequence_name);
    table_meta.set_autoinc(autoinc);

    if (node.HasNotEmptyAttr("class"))
        table_meta.set_class_name(node.GetAttr("class"));
    else
        skip_generation_.push_back(name);

    parse_column(node.get()->children, table_meta);
}

void XMLMetaDataConfig::parse_relation_side(xmlNodePtr p_node,
        const char **attr_names, size_t attr_count,
        string &cname, Relation::AttrMap &attrs)
{
    Node node(p_node, false);
    if (!node.HasAttr("class"))
        throw MandatoryAttributeAbsent((const char *)node.get()->name, "class");
    cname = node.GetAttr("class");
    Relation::AttrMap new_attrs;
    for (size_t i = 0; i < attr_count; ++i)
        if (node.HasAttr(attr_names[i]))
            new_attrs[attr_names[i]] = node.GetAttr(attr_names[i]);
    attrs.swap(new_attrs);
}

bool XMLMetaDataConfig::parse_relation(xmlNodePtr p_node, Relation &rel)
{
    Node node(p_node, false);
    if (!node.HasNotEmptyAttr("type"))
        throw MandatoryAttributeAbsent("relation", "type");
    string rtype_str = node.GetAttr("type");
    string cascade = "restrict";
    if (node.HasNotEmptyAttr("cascade"))
        cascade = node.GetAttr("cascade");
    int cascade_code = -1;
    if (cascade == "delete")
        cascade_code = Relation::Delete;
    else if (cascade == "set-null")
        cascade_code = Relation::Nullify;
    else if (cascade == "restrict")
        cascade_code = Relation::Restrict;
    else
        throw ParseError(string("Unknown 'cascade' value: ") + cascade);
    int rtype = Relation::UNKNOWN;
    if (!rtype_str.compare("one-to-many"))
        rtype = Relation::ONE2MANY;
    else
        //throw MandatoryAttributeAbsent("relation", "type");
        return false;
    string one, many;
    Relation::AttrMap a1, a2;
    static const char
        *anames_one[] = {"property", "use-list"},
        *anames_many[] = {"property", "filter"};
    for (xmlNodePtr child = node.get()->children; child; child = child->next) {
        if (child->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *)child->name, "one")) {
            parse_relation_side(child, anames_one,
                    sizeof(anames_one)/sizeof(void*), one, a1);
        } else if (!strcmp((const char *)child->name, "many")) {
            parse_relation_side(child, anames_many,
                    sizeof(anames_many)/sizeof(void*), many, a2);
        } else
            throw ParseError(string("Unknown element '") +
                    (const char *)child->name +
                    "' found during parse of element 'relation'");
    }
    rel = Relation(rtype, one, a1, many, a2);
    return true;
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

void XMLMetaDataConfig::parse_column(const xmlNodePtr p_node, Table &table_meta)
{
    for(xmlNodePtr col = p_node; col; col = col->next) {
        if(col->type != XML_ELEMENT_NODE)
            continue;
        if(string((const char *)col->name) != "column")
            throw ParseError(string("Unknown element '") + (const char *)col->name
                    + "' found during parse of element 'table'");
        table_meta.add_column(fill_column_meta(col));
    }
}

Column XMLMetaDataConfig::fill_column_meta(xmlNodePtr p_node)
{
    string name, type, fk_table, fk_field, prop_name, xml_name;
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

    if (node.HasAttr("size"))
        size = node.GetLongAttr("size");

    if (node.HasAttr("property"))
        prop_name = node.GetAttr("property");
    if (node.HasAttr("xml-name"))
        xml_name = node.GetAttr("xml-name");

    for (xmlNode *child = node.get()->children; child; child = child->next) {
        if (child->type != XML_ELEMENT_NODE)
            continue;
        if (is_current_child_name(child, "read-only"))
            flags |= Column::RO;
        if (is_current_child_name(child, "primary-key"))
            flags |= Column::PK;
        if (string((const char *)child->name) == "foreign-key") {
            get_foreign_key_data(child, fk_table, fk_field);
        }
    }

    bool nullable = !(flags & Column::PK);
    if (node.HasAttr("null")) {
        //if (col_type == Value::STRING)
        //    throw InvalidCombination("nullable attribute cannot be used for strings");
        if (node.GetAttr("null") == "false")
            nullable = false;
    }
    if (nullable)
        flags |= Column::NULLABLE;

    if((size > 0) && (col_type != Value::STRING))
        throw InvalidCombination("Size musn't me used for not a string type");
    Column result(name, col_type, size, flags, fk_table, fk_field, xml_name, prop_name);
    result.set_default_value(default_val);
    return result;
}

void XMLMetaDataConfig::get_foreign_key_data(xmlNodePtr p_node, string &fk_table, string &fk_field)
{
    Node node(p_node, false);
    if (!node.HasNotEmptyAttr("table"))
        throw MandatoryAttributeAbsent("foreign-key", "table");
    else
        fk_table = node.GetAttr("table");

    if (node.HasNotEmptyAttr("key"))
        fk_field = node.GetAttr("key");
    else
        fk_field = string();
}    

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
