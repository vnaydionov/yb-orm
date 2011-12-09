#include <fstream>
#include <util/xmlnode.h>
#include <util/str_utils.hpp>
#include "Value.h"
#include "XMLMetaDataConfig.h"

using namespace std;
using namespace Xml;

class TestXMLConfig;

namespace Yb {

bool load_xml_file(const String &name, string &where)
{
    ifstream tfile(NARROW(name).c_str());
    if (!tfile)
        return false;
    while (tfile.good()) {
        char ch;
        tfile.get(ch);
        where.push_back(ch);
    }
    return true;
}

void load_meta(const String &name, Schema &reg)
{
    string xml;
    if (!load_xml_file(name, xml))
        throw XMLConfigError(_T("Can't read file: ") + name);
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
        throw ParseError(String(_T("XML tree parse error: ")) + WIDEN(e.what()));
    }
}

void XMLMetaDataConfig::parse(Schema &reg)
{
    if (strcmp((const char *)node_.get()->name, "schema"))
        throw ParseError(String(_T("Unknown element '")) + 
                WIDEN((const char *)node_.get()->name) + 
                _T("' found during parse of root element, 'schema' expected"));

    for (xmlNodePtr child = node_.get()->children; child; child = child->next) {
        if (child->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *)child->name, "table")) {
            Table::Ptr t = parse_table(child);
            reg.add_table(t);
        } else if (!strcmp((const char *)child->name, "relation")) {
            Relation::Ptr r = parse_relation(child);
            if (r.get())
                reg.add_relation(r);
        } else
            throw ParseError(String(_T("Unknown element '")) +
                    WIDEN((const char *)child->name) +
                    _T("' found during parse of element 'schema'"));
    }
}

Table::Ptr XMLMetaDataConfig::parse_table(xmlNodePtr p_node)
{
    String sequence_name, name, xml_name, class_name;
    bool autoinc = false;

    Node node(p_node, false);

    if(!node.HasNotEmptyAttr(_T("name")))
        throw MandatoryAttributeAbsent(_T("table"), _T("name"));

    if(node.HasNotEmptyAttr(_T("sequence")))
        sequence_name = node.GetAttr(_T("sequence"));

    if(node.HasAttr(_T("autoinc")))
        autoinc = true;

    name = node.GetAttr(_T("name"));

    if(node.HasNotEmptyAttr(_T("xml-name")))
        xml_name = node.GetAttr(_T("xml-name"));
    else
        xml_name = mk_xml_name(name, _T(""));

    if (node.HasNotEmptyAttr(_T("class")))
        class_name = node.GetAttr(_T("class"));
    else
        skip_generation_.push_back(name);

    Table::Ptr table_meta(new Table(name, xml_name, class_name));
    table_meta->set_seq_name(sequence_name);
    table_meta->set_autoinc(autoinc);

    parse_column(node.get()->children, *table_meta);
    return table_meta;
}

void XMLMetaDataConfig::parse_relation_side(xmlNodePtr p_node,
        const Char **attr_names, size_t attr_count,
        String &cname, Relation::AttrMap &attrs)
{
    Node node(p_node, false);
    if (!node.HasAttr(_T("class")))
        throw MandatoryAttributeAbsent(WIDEN((const char *)node.get()->name), _T("class"));
    cname = node.GetAttr(_T("class"));
    Relation::AttrMap new_attrs;
    for (size_t i = 0; i < attr_count; ++i)
        if (node.HasAttr(attr_names[i]))
            new_attrs[attr_names[i]] = node.GetAttr(attr_names[i]);
    attrs.swap(new_attrs);
}

Relation::Ptr XMLMetaDataConfig::parse_relation(xmlNodePtr p_node)
{
    Node node(p_node, false);
    if (!node.HasNotEmptyAttr(_T("type")))
        throw MandatoryAttributeAbsent(_T("relation"), _T("type"));
    String rtype_str = node.GetAttr(_T("type"));
    String cascade = _T("restrict");
    if (node.HasNotEmptyAttr(_T("cascade")))
        cascade = node.GetAttr(_T("cascade"));
    int cascade_code = -1;
    if (cascade == _T("delete"))
        cascade_code = Relation::Delete;
    else if (cascade == _T("set-null"))
        cascade_code = Relation::Nullify;
    else if (cascade == _T("restrict"))
        cascade_code = Relation::Restrict;
    else
        throw ParseError(String(_T("Unknown 'cascade' value: ")) + cascade);
    int rtype = Relation::UNKNOWN;
    if (!rtype_str.compare(_T("one-to-many")))
        rtype = Relation::ONE2MANY;
    else
        //throw MandatoryAttributeAbsent(_T("relation"), _T("type"));
        return Relation::Ptr();
    String one, many;
    Relation::AttrMap a1, a2;
    static const Char
        *anames_one[] = {_T("property"), _T("use-list")},
        *anames_many[] = {_T("property"), _T("filter")};
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
            throw ParseError(String(_T("Unknown element '")) +
                    WIDEN((const char *)child->name) +
                    _T("' found during parse of element 'relation'"));
    }
    Relation::Ptr rel(new Relation(rtype, one, a1, many, a2));
    return rel;
}

int XMLMetaDataConfig::string_type_to_int(const String &type, const String &field_name)
{
    if(Yb::StrUtils::str_to_lower(type) == _T("longint"))
        return Value::LONGINT;
    else if(Yb::StrUtils::str_to_lower(type) == _T("string"))
        return Value::STRING;
    else if(Yb::StrUtils::str_to_lower(type) == _T("decimal"))
        return Value::DECIMAL;
    else if(Yb::StrUtils::str_to_lower(type) == _T("datetime"))
        return Value::DATETIME;
    else
        throw WrongColumnType(type, field_name);
}

bool XMLMetaDataConfig::is_current_child_name(xmlNodePtr p_node, const String &field)
{
    return (WIDEN((const char *)p_node->name) == field) ? true : false;
}

void XMLMetaDataConfig::parse_column(const xmlNodePtr p_node, Table &table_meta)
{
    for(xmlNodePtr col = p_node; col; col = col->next) {
        if(col->type != XML_ELEMENT_NODE)
            continue;
        if(string((const char *)col->name) != "column")
            throw ParseError(String(_T("Unknown element '")) + WIDEN((const char *)col->name)
                    + _T("' found during parse of element 'table'"));
        table_meta.add_column(fill_column_meta(col));
    }
}

Column XMLMetaDataConfig::fill_column_meta(xmlNodePtr p_node)
{
    String name, type, fk_table, fk_field, prop_name, xml_name;
    int flags = 0, size = 0, col_type = 0;
    Xml::Node node(Xml::DupNode(p_node));
    Yb::Value default_val;
    if (!node.HasNotEmptyAttr(_T("name")))
        throw MandatoryAttributeAbsent(_T("column"), _T("name"));
    else
        name = node.GetAttr(_T("name"));

    if (!node.HasNotEmptyAttr(_T("type")))
        throw MandatoryAttributeAbsent(_T("column"), _T("type"));
    else {
        type = node.GetAttr(_T("type"));
        col_type = string_type_to_int(type, name);
    }

    if (node.HasNotEmptyAttr(_T("default"))) {
       String value = node.GetAttr(_T("default"));
        switch (col_type) {
            case Value::DECIMAL:
            case Value::LONGINT:
                try {
                    default_val = Value(boost::lexical_cast<LongInt>(value));
                } catch(const boost::bad_lexical_cast &) {
                        throw ParseError(String(_T("Wrong default value '")) + value + _T("' for integer element '") + name + _T("'"));
                }
                break;
            case Value::DATETIME:
                if (Yb::StrUtils::str_to_lower(node.GetAttr(_T("default"))) != String(_T("sysdate")))
                    throw ParseError(String(_T("Wrong default value for datetime element '"))+ name + _T("'"));
                default_val = Value(_T("sysdate"));
                break;
            default:
                default_val = Value(node.GetAttr(_T("default")));
        }
    }

    if (node.HasAttr(_T("size")))
        size = node.GetLongAttr(_T("size"));

    if (node.HasAttr(_T("property")))
        prop_name = node.GetAttr(_T("property"));
    if (node.HasAttr(_T("xml-name")))
        xml_name = node.GetAttr(_T("xml-name"));

    for (xmlNode *child = node.get()->children; child; child = child->next) {
        if (child->type != XML_ELEMENT_NODE)
            continue;
        if (is_current_child_name(child, _T("read-only")))
            flags |= Column::RO;
        if (is_current_child_name(child, _T("primary-key")))
            flags |= Column::PK;
        if (string((const char *)child->name) == "foreign-key") {
            get_foreign_key_data(child, fk_table, fk_field);
        }
    }

    bool nullable = !(flags & Column::PK);
    if (node.HasAttr(_T("null"))) {
        //if (col_type == Value::STRING)
        //    throw InvalidCombination(_T("nullable attribute cannot be used for strings"));
        if (node.GetAttr(_T("null")) == _T("false"))
            nullable = false;
    }
    if (nullable)
        flags |= Column::NULLABLE;

    if((size > 0) && (col_type != Value::STRING))
        throw InvalidCombination(_T("Size musn't me used for not a String type"));
    Column result(name, col_type, size, flags, fk_table, fk_field, xml_name, prop_name);
    result.set_default_value(default_val);
    return result;
}

void XMLMetaDataConfig::get_foreign_key_data(xmlNodePtr p_node, String &fk_table, String &fk_field)
{
    Node node(p_node, false);
    if (!node.HasNotEmptyAttr(_T("table")))
        throw MandatoryAttributeAbsent(_T("foreign-key"), _T("table"));
    else
        fk_table = node.GetAttr(_T("table"));

    if (node.HasNotEmptyAttr(_T("key")))
        fk_field = node.GetAttr(_T("key"));
    else
        fk_field = String();
}    

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
