#ifndef YB__AUTOXMLIZABLE__INCLUDED
#define YB__AUTOXMLIZABLE__INCLUDED

#include "orm/XMLNode.h"

namespace Yb {
namespace Domain {

class AutoXMLizable {
public:
    virtual const ORMapper::XMLNode auto_xmlize(int deep) const = 0;
    virtual ~AutoXMLizable()
    {}
};

} // namespace SQL
} // namespace Yb

// vim:ts=4:sts=4:sw=4:et

#endif // YB__SESSION__INCLUDED

