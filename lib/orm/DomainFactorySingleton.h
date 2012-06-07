#ifndef YB__ORM__DOMAIN_FACTORY_SINGLETON__INCLUDED
#define YB__ORM__DOMAIN_FACTORY_SINGLETON__INCLUDED

#include <util/Singleton.h>
#include <orm/DomainFactory.h>

namespace Yb {
typedef SingletonHolder<DomainFactory> theDomainFactory;
} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__DOMAIN_FACTORY_SINGLETON__INCLUDED
