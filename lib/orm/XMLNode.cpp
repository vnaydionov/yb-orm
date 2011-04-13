#include "XMLNode.h"
#include "DomainFactorySingleton.h"

using namespace std;

namespace Yb {

void XMLNode::init_by_row_data(const RowData &data, const string &alt_name)
{
    const TableMetaData &table = data.get_table();
    name_ = alt_name.empty()? table.get_xml_name(): alt_name;
    TableMetaData::Map::const_iterator it = table.begin(), end = table.end();
    for ( ; it != end; ++it) {
        const string col_name = it->second.get_xml_name();
        if (!col_name.empty())
            children_.push_back(XMLNode(col_name, data.get(it->second.get_name())));
    }
}

void XMLNode::replace_child_object_by_field(const string &field_name, const RowData &data) 
{
    NodeList::iterator it = children_.begin(), end = children_.end();
    for (; it != end; ++it)
        if (it->name_ == field_name) {
            it->value_ = Value(); // reset the value
            it->init_by_row_data(data);
        }
}

void XMLNode::replace_child_object_by_field(const string &field_name, const XMLNode &node) 
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
        elem.add_attribute("is_null", true);
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
deep_xmlize(SessionBase &session, const RowData &d, int depth)
{   
    XMLNode node(d);
    if (depth == -1 || depth > 0) {
        const TableMetaData &table = d.get_table();
        TableMetaData::Map::const_iterator it = table.begin(), end = table.end();
        for (; it != end; ++it)
            if (it->second.has_fk() && !d.get(it->second.get_name()).is_null()) {
                boost::shared_ptr<AutoXMLizable> domain_obj = 
                    theDomainFactory::instance().create_object(session, it->second.get_fk_table_name(), 
                        d.get(it->second.get_name()).as_longint()); 
                XMLNode ref_node(domain_obj->auto_xmlize((depth == -1 ) ? -1: depth - 1));
                node.replace_child_object_by_field(it->second.get_xml_name(), ref_node);
            }
    }
    return node;
}

const XMLNode
xmlize_row(const Row &row, const string &entry_name)
{
    XMLNode entry(entry_name, "");
    Row::const_iterator it = row.begin(), end = row.end();
    for (; it != end; ++it)
        entry.add_node(XMLNode(mk_xml_name(it->first, ""),
                    it->second.nvl("").as_string()));
    return entry;
}

const XMLNode
xmlize_rows(const Rows &rows, const string &entries_name, const string &entry_name)
{
    XMLNode entries(entries_name, "");
    Rows::const_iterator it = rows.begin(), end = rows.end();
    for (; it != end; ++it)
        entries.add_node(xmlize_row(*it, entry_name));
    return entries;
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
