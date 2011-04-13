#ifndef YB__ORM__XML_NODE__INCLUDED
#define YB__ORM__XML_NODE__INCLUDED

#include <vector>
#include <util/xml_writer.h>
#include "Mapper.h"
#include "EngineBase.h"

namespace Yb {

class XMLNode
{
    typedef std::vector<XMLNode> NodeList;
public:
    XMLNode()
    {}
    XMLNode(const std::string &name, const Value &val)
        : name_(name), value_(val)
    {}
    XMLNode(const RowData &data, const std::string &alt_name = "")
    {
        init_by_row_data(data, alt_name);
    }
public:
    void init_by_row_data(const RowData &data, const std::string &alt_name = "");
    void replace_child_object_by_field(const std::string &field_name, const RowData &data);
    void replace_child_object_by_field(const std::string &field_name, const XMLNode &node);
    const std::string get_xml() const;
    void xmlize(Yb::Writer::Document &doc) const;
    void add_node(const XMLNode & node)
    {
        children_.push_back(node);
    }
private:
    std::string name_;
    Value value_;
    NodeList children_;
};

/**
 * @param mapper OR mapper
 * @param d start point
 * @param depth == -1 recursion not limited
 *              >= 1 nested levels
 * @return XMLNode
 */
const XMLNode deep_xmlize(Mapper &mapper, const RowData &d, int depth = 0);

const XMLNode xmlize_row(const Row &row, const std::string &entry_name);
const XMLNode xmlize_rows(const Rows &rows,
        const std::string &entries_name, const std::string &entry_name);

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__XMLNODE__INCLUDED
