// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#define YBORM_SOURCE

#include <fstream>
#include "util/element_tree.h"
#include "util/string_utils.h"
#include "util/value_type.h"
#include "orm/schema_config.h"

using namespace std;

class TestXMLConfig;

namespace Yb {

XMLConfigError::XMLConfigError(const String &msg)
    : MetaDataError(msg)
{}

MandatoryAttributeAbsent::MandatoryAttributeAbsent(
        const String &element, const String &attr)
    : XMLConfigError(_T("Mandatory attribyte '") + attr +
            _T("' not found or empty while parsing element '") +
            element + _T("'"))
{}

WrongColumnType::WrongColumnType(const String &type, const String &field_name)
    : XMLConfigError(_T("Type '") + type +
            _T("' is unknown and not supported while parsing field '")
            + field_name + _T("'"))
{}

ParseError::ParseError(const String &msg)
    : XMLConfigError(_T("XML config parse error, details: ") + msg)
{}

InvalidCombination::InvalidCombination(const String &msg)
    : XMLConfigError(_T("Invalid element-attribute combination, details: ") + msg)
{}

YBORM_DECL bool
load_xml_file(const String &name, string &where)
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

YBORM_DECL void
load_schema(const String &name, Schema &reg, bool check)
{
    string xml;
    if (!load_xml_file(name, xml))
        throw XMLConfigError(_T("Can't read file: ") + name);
    MetaDataConfig xml_config(xml);
    xml_config.parse(reg);
    reg.fill_fkeys();
    if (check)
        reg.check_cycles();
}

MetaDataConfig::MetaDataConfig(const string &xml_string)
{
    try {
        node_ = ElementTree::parse(xml_string);
    }
    catch (const exception &e) {
        throw ParseError(String(_T("XML tree parse error: ")) + WIDEN(e.what()));
    }
}

MetaDataConfig::MetaDataConfig(const Schema &schema)
    : node_(build_tree(schema))
{}

void MetaDataConfig::parse(Schema &reg)
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

Table::Ptr MetaDataConfig::parse_table(ElementTree::ElementPtr node)
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

void MetaDataConfig::parse_relation_side(ElementTree::ElementPtr node,
        const char **attr_names, size_t attr_count,
        String &cname, Relation::AttrMap &attrs)
{
    if (!node->has_attr(_T("class")))
        throw MandatoryAttributeAbsent(node->name_, _T("class"));
    cname = node->get_attr(_T("class"));
    Relation::AttrMap new_attrs;
    for (size_t i = 0; i < attr_count; ++i)
        if (node->has_attr(std2str(attr_names[i])))
            new_attrs[std2str(attr_names[i])] =
                node->get_attr(std2str(attr_names[i]));
    attrs.swap(new_attrs);
}

Relation::Ptr MetaDataConfig::parse_relation(ElementTree::ElementPtr node)
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
        *anames_one[] = {"property", "use-list", },
        *anames_many[] = {"property", "order-by", "key", };
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

int MetaDataConfig::string_type_to_int(const String &type, const String &field_name)
{
    for (int type_it = 1; true; ++type_it) {
        String type_it_name = StrUtils::str_to_lower(
                Value::get_type_name(type_it));
        if (type_it_name == _T("unknowntype"))
            break;
        if (StrUtils::str_to_lower(type) == type_it_name)
            return type_it;
    }
    throw WrongColumnType(type, field_name);
}

void MetaDataConfig::parse_column(ElementTree::ElementPtr node, Table &table_meta)
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

Column MetaDataConfig::fill_column_meta(ElementTree::ElementPtr node)
{
    String name, type, fk_table, fk_field, prop_name, xml_name, index;
    int flags = 0, size = 0, col_type = 0;
    Value default_val;
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
            case Value::FLOAT:
                try {
                    double x;
                    from_string(value, x);
                    default_val = Value(x);
                } catch(const std::exception &) {
                    throw ParseError(String(_T("Wrong default value '")) + value + _T("' for float element '") + name + _T("'"));
                }
                break;
            case Value::DECIMAL:
                try {
                    Decimal x;
                    from_string(value, x);
                    default_val = Value(x);
                } catch(const std::exception &) {
                    throw ParseError(String(_T("Wrong default value '")) + value + _T("' for decimal element '") + name + _T("'"));
                }
                break;
            case Value::INTEGER:
            case Value::LONGINT:
                try {
                    LongInt x;
                    from_string(value, x);
                    default_val = Value(x);
                } catch(const std::exception &) {
                    throw ParseError(String(_T("Wrong default value '")) + value + _T("' for integer element '") + name + _T("'"));
                }
                break;
            case Value::DATETIME:
                if (StrUtils::str_to_lower(node->get_attr(_T("default"))) != String(_T("sysdate")))
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
        if (!(*child)->name_.compare(_T("index"))) {
            index = (*child)->get_text();
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
            fk_table, fk_field, xml_name, prop_name, index);
    return result;
}

void MetaDataConfig::get_foreign_key_data(ElementTree::ElementPtr node, String &fk_table, String &fk_field)
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

ElementTree::ElementPtr MetaDataConfig::column_to_tree(const Column &column)
{
    ElementTree::ElementPtr node = ElementTree::new_element(
            _T("column"));
    node->attrib_[_T("name")] = column.name();
    node->attrib_[_T("type")] = StrUtils::str_to_lower(
            Value::get_type_name(column.type()));
    if (column.size())
        node->attrib_[_T("size")] = to_string(column.size());
    if (!str_empty(column.prop_name()) && column.prop_name() != column.name()
            && column.prop_name() != StrUtils::str_to_lower(column.name()))
        node->attrib_[_T("property")] = column.prop_name();
    if (!str_empty(column.xml_name()) && column.xml_name() != column.prop_name())
        node->attrib_[_T("xml-name")] = column.xml_name();
    if (!column.default_value().is_null()
            && !str_empty(column.default_value().as_string()))
        node->attrib_[_T("default")] = column.default_value().as_string();
    if (!column.is_nullable() && !column.is_pk())
        node->attrib_[_T("null")] = _T("false");
    if (column.is_ro())
        node->sub_element(_T("read-only"));
    if (column.is_pk())
        node->sub_element(_T("primary-key"));
    if (column.has_fk()) {
        ElementTree::ElementPtr fk_node =
            node->sub_element(_T("foreign-key"));
        fk_node->attrib_[_T("table")] = column.fk_table_name();
        if (!str_empty(column.fk_name()))
            fk_node->attrib_[_T("key")] = column.fk_name();
    }
    if (!str_empty(column.index_name()))
        node->sub_element(_T("index"), column.index_name());
    return node;
}

ElementTree::ElementPtr MetaDataConfig::table_to_tree(const Table &table)
{
    ElementTree::ElementPtr node = ElementTree::new_element(
            _T("table"));
    node->attrib_[_T("name")] = table.name();
    if (!str_empty(table.class_name()))
        node->attrib_[_T("class")] = table.class_name();
    if (!str_empty(table.seq_name()))
        node->attrib_[_T("sequence")] = table.seq_name();
    if (!str_empty(table.xml_name()) && table.xml_name() != table.class_name())
        node->attrib_[_T("xml-name")] = table.xml_name();
    else if (table.autoinc())
        node->attrib_[_T("autoinc")] = _T("true");
    Columns::const_iterator it = table.begin(), end = table.end();
    for (; it != end; ++it)
        node->children_.push_back(column_to_tree(*it));
    return node;
}

ElementTree::ElementPtr MetaDataConfig::relation_to_tree(const Relation &rel)
{
    ElementTree::ElementPtr node = ElementTree::new_element(
            _T("relation"));
    // for now just one2many is supported
    YB_ASSERT(rel.type() == Relation::ONE2MANY);
    node->attrib_[_T("type")] = _T("one-to-many");
    node->attrib_[_T("cascade")] = (
            rel.cascade() == (int)Relation::Restrict?
            _T("restrict"): (
                rel.cascade() == (int)Relation::Nullify?
                _T("set-null"): _T("delete")));
    ElementTree::ElementPtr node_one = node->sub_element(_T("one"));
    node_one->attrib_[_T("class")] = rel.side(0);
    if (rel.has_non_empty_attr(0, _T("property")))
        node_one->attrib_[_T("property")] = rel.attr(0, _T("property"));
    if (rel.has_non_empty_attr(0, _T("use-list"))
            && (rel.attr(0, _T("use-list")) == _T("false") ||
                rel.attr(0, _T("use-list")) == _T("0")))
        node_one->attrib_[_T("use-list")] = _T("false");
    ElementTree::ElementPtr node_many = node->sub_element(_T("many"));
    node_many->attrib_[_T("class")] = rel.side(1);
    if (rel.has_non_empty_attr(1, _T("property")))
        node_many->attrib_[_T("property")] = rel.attr(1, _T("property"));
    if (rel.has_non_empty_attr(1, _T("key")))
        node_many->attrib_[_T("key")] = rel.attr(1, _T("key"));
    if (rel.has_non_empty_attr(1, _T("order-by")))
        node_many->attrib_[_T("order-by")] = rel.attr(1, _T("order-by"));
    return node;
}

ElementTree::ElementPtr MetaDataConfig::build_tree(const Schema &schema)
{
    ElementTree::ElementPtr node = ElementTree::new_element(
            _T("schema"));
    Schema::TblMap::const_iterator i = schema.tbl_begin(), iend = schema.tbl_end();
    for (; i != iend; ++i)
        node->children_.push_back(table_to_tree(*i->second));
    Schema::RelVect::const_iterator j = schema.rel_begin(), jend = schema.rel_end();
    for (; j != jend; ++j)
        node->children_.push_back(relation_to_tree(**j));
    return node;
}

const std::string MetaDataConfig::save_xml(bool indent)
{
    return node_->serialize(indent);
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
