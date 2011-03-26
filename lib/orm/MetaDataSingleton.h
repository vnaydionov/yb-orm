#ifndef YB__ORM__META_DATA_SINGLETON__INCLUDED
#define YB__ORM__META_DATA_SINGLETON__INCLUDED

#include <util/Singleton.h>
#include "MetaData.h"

namespace Yb {
typedef SingletonHolder<TableMetaDataRegistry> theMetaData;
}

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__META_DATA_SINGLETON__INCLUDED
