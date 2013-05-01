#include <orm/XMLizer.h>
#include <orm/DomainFactorySingleton.h>

using namespace std;

namespace Yb {

ElementTree::ElementPtr
data_object_to_etree(DataObject::Ptr data, const String &alt_name)
{
    const Table &table = data->table();
    String name = str_empty(alt_name)? table.xml_name(): alt_name;
    ElementTree::ElementPtr node = ElementTree::new_element(name);
    Columns::const_iterator it = table.begin(), end = table.end();
    for (; it != end; ++it) {
        const String col_name = it->xml_name();
        if (!str_empty(col_name) && _T("!") != col_name) {
            Value value = data->get(it->name());
            ElementTree::ElementPtr sub_el = node->sub_element(col_name);
            if (value.is_null())
                sub_el->attrib_[_T("is_null")] = _T("1");
            else
                sub_el->set_text(value.as_string());
        }
    }
    return node;
}

void
replace_child_object_by_field(ElementTree::ElementPtr node,
        const String &field_name, ElementTree::ElementPtr sub_node)
{
    ElementTree::Elements::iterator it = node->children_.begin(),
        end = node->children_.end();
    for (; it != end; ++it)
        if ((*it)->name_ == field_name) {
            *it = sub_node;
            break;
        }
}

void
replace_child_object_by_field(ElementTree::ElementPtr node,
        const String &field_name, DataObject::Ptr data)
{
    replace_child_object_by_field(node, field_name, data_object_to_etree(data));
}

/**
 * @param session OR session
 * @param d start point
 * @param depth == -1 recursion not limited
 *              >= 0 nested levels
 * @return ElementTree::ElementPtr
 */
ElementTree::ElementPtr
deep_xmlize(Session &session, DataObject::Ptr d,
    int depth, const String &alt_name)
{
    ElementTree::ElementPtr node = data_object_to_etree(d, alt_name);
    if (depth == -1 || depth > 0) {
        const Table &tbl = d->table();
        const String &cname = tbl.class_name();
        Columns::const_iterator it = tbl.begin(), end = tbl.end();
        for (; it != end; ++it)
            if (it->has_fk()) {
                const Value &fk_v = d->get(it->name());
                if (fk_v.is_null())
                    continue;
                const Table &fk_table = tbl.schema()[it->fk_table_name()];
                Schema::RelMap::const_iterator
                    j = tbl.schema().rels_lower_bound(cname),
                    jend = tbl.schema().rels_upper_bound(cname);
                for (; j != jend; ++j) {
                    if (j->second->type() == Relation::ONE2MANY &&
                        j->second->side(0) == fk_table.class_name() &&
                        (!j->second->has_attr(1, _T("key")) ||
                         j->second->attr(1, _T("key")) == it->name()))
                    {
                        SharedPtr<XMLizable>::Type domain_obj = 
                            theDomainFactory::instance().create_object(
                                session, fk_table.name(), fk_v.as_longint());
                        ElementTree::ElementPtr ref_node = domain_obj->xmlize(
                            depth == -1? -1: depth - 1, mk_xml_name(
                                j->second->attr(1, _T("property")), _T("")
                            ));
                        replace_child_object_by_field(node,
                                it->xml_name(), ref_node);
                        break;
                    }
                }
            }
    }
    return node;
}

ElementTree::ElementPtr
xmlize_row(const Row &row, const String &entry_name)
{
    ElementTree::ElementPtr entry = ElementTree::new_element(entry_name);
    Row::const_iterator it = row.begin(), end = row.end();
    for (; it != end; ++it)
        entry->sub_element(mk_xml_name(it->first, _T("")),
                it->second.nvl(Value(String(_T("")))).as_string());
    return entry;
}

ElementTree::ElementPtr
xmlize_rows(const Rows &rows, const String &entries_name, const String &entry_name)
{
    ElementTree::ElementPtr entries = ElementTree::new_element(entries_name);
    Rows::const_iterator it = rows.begin(), end = rows.end();
    for (; it != end; ++it)
        entries->children_.push_back(xmlize_row(*it, entry_name));
    return entries;
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
