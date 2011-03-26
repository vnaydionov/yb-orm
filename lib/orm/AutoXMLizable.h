#ifndef YB__ORM__AUTO_XMLIZABLE__INCLUDED
#define YB__ORM__AUTO_XMLIZABLE__INCLUDED

#include "XMLNode.h"

namespace Yb {

class AutoXMLizable {
public:
    virtual const XMLNode auto_xmlize(int deep) const = 0;
    virtual ~AutoXMLizable()
    {}
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__AUTO_XMLIZABLE__INCLUDED
