// -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#ifndef YB__ORM__XMLIZER__INCLUDED
#define YB__ORM__XMLIZER__INCLUDED

#include <vector>
#include <util/ElementTree.h>
#include <util/Utility.h>
#include <orm/DataObject.h>
#include <orm/SqlDriver.h>

namespace Yb {

ElementTree::ElementPtr data_object_to_etree(DataObject::Ptr data,
        const String &alt_name = _T(""));

void replace_child_object_by_field(ElementTree::ElementPtr node,
        const String &field_name, ElementTree::ElementPtr sub_node);

void replace_child_object_by_field(ElementTree::ElementPtr node,
        const String &field_name, DataObject::Ptr data);

/**
 * @param session OR session
 * @param d start point
 * @param depth == -1 recursion not limited
 *              >= 1 nested levels
 * @return ElementTree::ElementPtr
 */
ElementTree::ElementPtr deep_xmlize(Session &session,
        DataObject::Ptr d, int depth = 0, const String &alt_name = _T(""));

ElementTree::ElementPtr xmlize_row(const Row &row, const String &entry_name);

ElementTree::ElementPtr xmlize_rows(const Rows &rows,
        const String &entries_name, const String &entry_name);

class XMLizable: public RefCountBase
{
public:
    typedef IntrusivePtr<XMLizable> Ptr;
    virtual ~XMLizable();
    virtual ElementTree::ElementPtr xmlize(int depth,
        const String &alt_name = _T("")) const = 0;
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__XMLIZER__INCLUDED
