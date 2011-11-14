// -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#ifndef YB__ORM__XML_NODE__INCLUDED
#define YB__ORM__XML_NODE__INCLUDED

#include <vector>
#include <util/xml_writer.h>
#include "DataObject.h"
#include "SqlDriver.h"

namespace Yb {

class XMLNode
{
    typedef std::vector<XMLNode> NodeList;
public:
    XMLNode()
    {}
    XMLNode(const String &name, const Value &val)
        : name_(name), value_(val)
    {}
    XMLNode(DataObject::Ptr data, const String &alt_name = _T(""))
    {
        init_by_data_object(data, alt_name);
    }
public:
    void init_by_data_object(DataObject::Ptr data, const String &alt_name = _T(""));
    void replace_child_object_by_field(const String &field_name, DataObject::Ptr data);
    void replace_child_object_by_field(const String &field_name, const XMLNode &node);
    const std::string get_xml() const;
    void xmlize(Yb::Writer::Document &doc) const;
    void add_node(const XMLNode & node)
    {
        children_.push_back(node);
    }
private:
    String name_;
    Value value_;
    NodeList children_;
};

/**
 * @param session OR session
 * @param d start point
 * @param depth == -1 recursion not limited
 *              >= 1 nested levels
 * @return XMLNode
 */
const XMLNode deep_xmlize(Session &session,
    DataObject::Ptr d, int depth = 0, const String &alt_name = _T(""));

const XMLNode xmlize_row(const Row &row, const String &entry_name);
const XMLNode xmlize_rows(const Rows &rows,
        const String &entries_name, const String &entry_name);

class XMLizable
{
public:
    virtual ~XMLizable() {}
    virtual const XMLNode xmlize(int depth,
        const String &alt_name = _T("")) const = 0;
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__XMLNODE__INCLUDED
