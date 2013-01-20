#include <fstream>
#include <util/ElementTree.h>
#include <util/str_utils.hpp>
#include <orm/Value.h>
#include <orm/XMLMetaDataConfig.h>

using namespace std;

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

void load_schema(const String &name, Schema &reg, bool check)
{
    string xml;
    if (!load_xml_file(name, xml))
        throw XMLConfigError(_T("Can't read file: ") + name);
    XMLMetaDataConfig xml_config(xml);
    xml_config.parse(reg);
    reg.fill_fkeys();
    if (check)
        reg.check_cycles();
}

XMLMetaDataConfig::XMLMetaDataConfig(const string &xml_string)
{
    try {
        node_ = ElementTree::parse(xml_string);
    }
    catch (const exception &e) {
        throw ParseError(String(_T("XML tree parse error: ")) + WIDEN(e.what()));
    }
}

void XMLMetaDataConfig::parse(Schema &reg)
{
    if (node_->name_.compare(_T("schema")))
        throw ParseError(String(_T("Unknown element '")) + node_->name_ + 
                _T("' found during parse of root element, 'schema' expected"));

    ElementTree::Elements::const_iterator child = node_->children_.begin(),
        cend = node_->children_.end();
    for (; child != cend; ++child) {
        if (!(*child)->name_.compare(_T("table"))) {
            Table::Ptr t = parse_table(*child);
            reg.add_table(t);
        } else if (!(*child)->name_.compare(_T("relation"))) {
            Relation::Ptr r = parse_relation(*child);
            if (shptr_get(r))
                reg.add_relation(r);
        } else
            throw ParseError(String(_T("Unknown element '")) + (*child)->name_ +
                    _T("' found during parse of element 'schema'"));
    }
}

Table::Ptr XMLMetaDataConfig::parse_table(ElementTree::ElementPtr node)
{
    String sequence_name, name, xml_name, class_name;
    bool autoinc = false;

    if (!node->has_attr(_T("name")))
        throw MandatoryAttributeAbsent(_T("table"), _T("name"));
    name = node->get_attr(_T("name"));

    if (node->has_attr(_T("sequence")))
        sequence_name = node->get_attr(_T("sequence"));

    if (node->has_attr(_T("autoinc")))
        autoinc = true;

    if (node->has_attr(_T("xml-name")))
        xml_name = node->get_attr(_T("xml-name"));
    else
        xml_name = mk_xml_name(name, _T(""));

    if (node->has_attr(_T("class")))
        class_name = node->get_attr(_T("class"));
    else
        skip_generation_.push_back(name);

    Table::Ptr table_meta(new Table(name, xml_name, class_name));
    table_meta->set_seq_name(sequence_name);
    table_meta->set_autoinc(autoinc);

    parse_column(node, *table_meta);
    return table_meta;
}

void XMLMetaDataConfig::parse_relation_side(ElementTree::ElementPtr node,
        const char **attr_names, size_t attr_count,
        String &cname, Relation::AttrMap &attrs)
{
    if (!node->has_attr(_T("class")))
        throw MandatoryAttributeAbsent(node->name_, _T("class"));
    cname = node->get_attr(_T("class"));
    Relation::AttrMap new_attrs;
    for (int i = 0; i < attr_count; ++i)
        if (node->has_attr(std2str(attr_names[i])))
            new_attrs[std2str(attr_names[i])] = node->get_attr(std2str(attr_names[i]));
    attrs.swap(new_attrs);
}

Relation::Ptr XMLMetaDataConfig::parse_relation(ElementTree::ElementPtr node)
{
    if (!node->has_attr(_T("type")))
        throw MandatoryAttributeAbsent(_T("relation"), _T("type"));
    String rtype_str = node->get_attr(_T("type"));
    String cascade = _T("restrict");
    if (node->has_attr(_T("cascade")))
        cascade = node->get_attr(_T("cascade"));
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
    static const char
        *anames_one[] = {"property", "use-list"},
        *anames_many[] = {"property", "filter", "key"};
    ElementTree::Elements::const_iterator child = node->children_.begin(),
        cend = node->children_.end();
    for (; child != cend; ++child) {
        if (!(*child)->name_.compare(_T("one"))) {
            parse_relation_side(*child, anames_one,
                    sizeof(anames_one)/sizeof(void*), one, a1);
        } else if (!(*child)->name_.compare(_T("many"))) {
            parse_relation_side(*child, anames_many,
                    sizeof(anames_many)/sizeof(void*), many, a2);
        } else
            throw ParseError(String(_T("Unknown element '")) + (*child)->name_ +
                    _T("' found during parse of element 'relation'"));
    }
    Relation::Ptr rel(new Relation(rtype, one, a1, many, a2, cascade_code));
    return rel;
}

int XMLMetaDataConfig::string_type_to_int(const String &type, const String &field_name)
{
    if(Yb::StrUtils::str_to_lower(type) == _T("longint"))
        return Value::LONGINT;
    else if(Yb::StrUtils::str_to_lower(type) == _T("integer"))
        return Value::INTEGER;
    else if(Yb::StrUtils::str_to_lower(type) == _T("string"))
        return Value::STRING;
    else if(Yb::StrUtils::str_to_lower(type) == _T("decimal"))
        return Value::DECIMAL;
    else if(Yb::StrUtils::str_to_lower(type) == _T("datetime"))
        return Value::DATETIME;
    else
        throw WrongColumnType(type, field_name);
}

void XMLMetaDataConfig::parse_column(ElementTree::ElementPtr node, Table &table_meta)
{
    ElementTree::Elements::const_iterator child = node->children_.begin(),
        cend = node->children_.end();
    for (; child != cend; ++child) {
        ElementTree::ElementPtr col = *child;
        if (col->name_.compare(_T("column")))
            throw ParseError(String(_T("Unknown element '")) + col->name_
                    + _T("' found during parse of element 'table'"));
        table_meta.add_column(fill_column_meta(col));
    }
}

Column XMLMetaDataConfig::fill_column_meta(ElementTree::ElementPtr node)
{
    String name, type, fk_table, fk_field, prop_name, xml_name;
    int flags = 0, size = 0, col_type = 0;
    Yb::Value default_val;
    if (!node->has_attr(_T("name")))
        throw MandatoryAttributeAbsent(_T("column"), _T("name"));
    else
        name = node->get_attr(_T("name"));

    if (!node->has_attr(_T("type")))
        throw MandatoryAttributeAbsent(_T("column"), _T("type"));
    else {
        type = node->get_attr(_T("type"));
        col_type = string_type_to_int(type, name);
    }

    if (node->has_attr(_T("default"))) {
       String value = node->get_attr(_T("default"));
        switch (col_type) {
            case Value::DECIMAL:
                try {
                    Decimal x;
                    from_string(value, x);
                    default_val = x;
                } catch(const std::exception &) {
                    throw ParseError(String(_T("Wrong default value '")) + value + _T("' for integer element '") + name + _T("'"));
                }
                break;
            case Value::INTEGER:
            case Value::LONGINT:
                try {
                    LongInt x;
                    from_string(value, x);
                    default_val = x;
                } catch(const std::exception &) {
                    throw ParseError(String(_T("Wrong default value '")) + value + _T("' for integer element '") + name + _T("'"));
                }
                break;
            case Value::DATETIME:
                if (Yb::StrUtils::str_to_lower(node->get_attr(_T("default"))) != String(_T("sysdate")))
                    throw ParseError(String(_T("Wrong default value for datetime element '"))+ name + _T("'"));
                default_val = Value(_T("sysdate"));
                break;
            default:
                default_val = Value(node->get_attr(_T("default")));
        }
    }

    if (node->has_attr(_T("size")))
        from_string(node->get_attr(_T("size")), size);

    if (node->has_attr(_T("property")))
        prop_name = node->get_attr(_T("property"));
    if (node->has_attr(_T("xml-name")))
        xml_name = node->get_attr(_T("xml-name"));

    ElementTree::Elements::const_iterator child = node->children_.begin(),
        cend = node->children_.end();
    for (; child != cend; ++child) {
        if (!(*child)->name_.compare(_T("read-only")))
            flags |= Column::RO;
        if (!(*child)->name_.compare(_T("primary-key")))
            flags |= Column::PK;
        if (!(*child)->name_.compare(_T("foreign-key"))) {
            get_foreign_key_data(*child, fk_table, fk_field);
        }
    }

    bool nullable = !(flags & Column::PK);
    if (node->has_attr(_T("null"))) {
        //if (col_type == Value::STRING)
        //    throw InvalidCombination(_T("nullable attribute cannot be used for strings"));
        if (node->get_attr(_T("null")) == _T("false"))
            nullable = false;
    }
    if (nullable)
        flags |= Column::NULLABLE;

    if((size > 0) && (col_type != Value::STRING))
        throw InvalidCombination(_T("Size musn't me used for not a String type"));
    Column result(name, col_type, size, flags, default_val,
            fk_table, fk_field, xml_name, prop_name);
    return result;
}

void XMLMetaDataConfig::get_foreign_key_data(ElementTree::ElementPtr node, String &fk_table, String &fk_field)
{
    if (!node->has_attr(_T("table")))
        throw MandatoryAttributeAbsent(_T("foreign-key"), _T("table"));
    else
        fk_table = node->get_attr(_T("table"));

    if (node->has_attr(_T("key")))
        fk_field = node->get_attr(_T("key"));
    else
        fk_field = String();
}    

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
