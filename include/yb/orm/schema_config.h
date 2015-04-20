// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__ORM__SCHEMA_CONFIG__INCLUDED
#define YB__ORM__SCHEMA_CONFIG__INCLUDED

#include <stdexcept>
#include <algorithm>
#include "util/element_tree.h"
#include "util/string_utils.h"
#include "orm_config.h"
#include "schema.h"

class TestXMLConfig;

namespace Yb {

class YBORM_DECL XMLConfigError: public MetaDataError
{
public:
    XMLConfigError(const String &msg);
};

class YBORM_DECL MandatoryAttributeAbsent: public XMLConfigError
{
public:
    MandatoryAttributeAbsent(const String &element, const String &attr);
};

class YBORM_DECL WrongColumnType: public XMLConfigError
{
public:
    WrongColumnType(const String &type, const String &field_name);
};

class YBORM_DECL ParseError: public XMLConfigError
{
public:
    ParseError(const String &msg);
};

class YBORM_DECL InvalidCombination: public XMLConfigError
{
public:
    InvalidCombination(const String &msg);
};

class YBORM_DECL MetaDataConfig
{
public:
    MetaDataConfig(const std::string &xml_string);
    MetaDataConfig(const Schema &schema);
    void parse(Schema &reg);
    const std::string save_xml(bool indent = false);
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
    static ElementTree::ElementPtr column_to_tree(const Column &column);
    static ElementTree::ElementPtr table_to_tree(const Table &table);
    static ElementTree::ElementPtr relation_to_tree(const Relation &rel);
    static ElementTree::ElementPtr build_tree(const Schema &schema);

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

YBORM_DECL bool load_xml_file(const String &name, std::string &where);

YBORM_DECL void load_schema(const String &name, Schema &reg, bool check = true);

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__SCHEMA_CONFIG__INCLUDED
