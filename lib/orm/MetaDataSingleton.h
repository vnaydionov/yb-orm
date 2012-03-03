#ifndef YB__ORM__META_DATA_SINGLETON__INCLUDED
#define YB__ORM__META_DATA_SINGLETON__INCLUDED

#include <util/Singleton.h>
#include <orm/MetaData.h>
#include <orm/DomainObj.h>

namespace Yb {
typedef SingletonHolder<Schema> theMetaData;

inline Schema &init_default_meta() {
    Schema &meta = Yb::theMetaData::instance();
    DomainObject::save_registered(meta);
    meta.fill_fkeys();
    return meta;
}

}

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__META_DATA_SINGLETON__INCLUDED
