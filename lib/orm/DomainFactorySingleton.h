
#ifndef YB__CORE__DOMAIN_FACTORY_SINGLETON__INCLUDED
#define YB__CORE__DOMAIN_FACTORY_SINGLETON__INCLUDED

#include <util/Singleton.h>
#include "orm/DomainFactory.h"

typedef Yb::SingletonHolder<Yb::ORMapper::DomainFactory> theDomainFactory;

// vim:ts=4:sts=4:sw=4:et

#endif // YB__CORE__METADATA_SINGLETON__INCLUDED

