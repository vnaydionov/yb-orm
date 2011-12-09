#include "XMLNode.h"
#include "DomainFactorySingleton.h"

using namespace std;

namespace Yb {

void XMLNode::init_by_data_object(DataObject::Ptr data, const String &alt_name)
{
    const Table &table = data->table();
    name_ = alt_name.empty()? table.get_xml_name(): alt_name;
    Columns::const_iterator it = table.begin(), end = table.end();
    for (; it != end; ++it) {
        const String col_name = it->get_xml_name();
        if (!col_name.empty())
            children_.push_back(XMLNode(col_name, data->get(it->get_name())));
    }
}

void XMLNode::replace_child_object_by_field(const String &field_name, DataObject::Ptr data) 
{
    NodeList::iterator it = children_.begin(), end = children_.end();
    for (; it != end; ++it)
        if (it->name_ == field_name) {
            it->value_ = Value(); // reset the value
            it->init_by_data_object(data);
        }
}

void XMLNode::replace_child_object_by_field(const String &field_name, const XMLNode &node) 
{
    NodeList::iterator it = children_.begin(), end = children_.end();
    for (; it != end; ++it)
        if (it->name_ == field_name) {
            it->value_ = Value(); // reset the value
            *it = node;
        }
}

const string XMLNode::get_xml()  const
{ 
    Yb::Writer::Document doc;
    xmlize(doc);
    return doc.end_document();
}

void XMLNode::xmlize(Yb::Writer::Document &doc) const
{
    if(name_.empty())
        return;
    Yb::Writer::Element elem(doc, name_);

    if (value_.is_null() && children_.empty())
        elem.add_attribute(_T("is_null"), true);
    if (!value_.is_null())
        elem.set_content(value_.as_string());

    NodeList::const_iterator it = children_.begin(), end = children_.end();
    for (; it != end; ++it)
        it->xmlize(doc);
}

/**
 * @param session OR session
 * @param d start point
 * @param depth == -1 recursion not limited
 *              >= 0 nested levels
 * @return XMLNode
 */
const XMLNode
deep_xmlize(Session &session, DataObject::Ptr d,
    int depth, const String &alt_name)
{
    XMLNode node(d, alt_name);
    if (depth == -1 || depth > 0) {
        const Table &tbl = d->table();
        const String &cname = tbl.get_class_name();
        Columns::const_iterator it = tbl.begin(), end = tbl.end();
        for (; it != end; ++it)
            if (it->has_fk() && !d->get(it->get_name()).is_null()) {
                const Table &fk_table = tbl.schema().
                    table(it->get_fk_table_name());
                Schema::RelMap::const_iterator
                    j = tbl.schema().rels_lower_bound(cname),
                    jend = tbl.schema().rels_upper_bound(cname);
                for (; j != jend; ++j) {
                    if (j->second->type() == Relation::ONE2MANY &&
                        j->second->side(0) == fk_table.get_class_name())
                    {
                        boost::shared_ptr<XMLizable> domain_obj = 
                            theDomainFactory::instance().create_object(
                                session, it->get_fk_table_name(),
                                d->get(it->get_name()).as_longint());
                        XMLNode ref_node(domain_obj->xmlize(
                            depth == -1? -1: depth - 1, mk_xml_name(
                                j->second->attr(1, _T("property")), _T("")
                            )));
                        node.replace_child_object_by_field(
                            it->get_xml_name(), ref_node);
                    }
                    break;
                }
            }
    }
    return node;
}

const XMLNode
xmlize_row(const Row &row, const String &entry_name)
{
    XMLNode entry(entry_name, _T(""));
    Row::const_iterator it = row.begin(), end = row.end();
    for (; it != end; ++it)
        entry.add_node(XMLNode(mk_xml_name(it->first, _T("")),
                    it->second.nvl(_T("")).as_string()));
    return entry;
}

const XMLNode
xmlize_rows(const Rows &rows, const String &entries_name, const String &entry_name)
{
    XMLNode entries(entries_name, _T(""));
    Rows::const_iterator it = rows.begin(), end = rows.end();
    for (; it != end; ++it)
        entries.add_node(xmlize_row(*it, entry_name));
    return entries;
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
