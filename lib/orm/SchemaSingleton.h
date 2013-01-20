#ifndef YB__ORM__SCHEMA_SINGLETON__INCLUDED
#define YB__ORM__SCHEMA_SINGLETON__INCLUDED

#include <util/Singleton.h>
#include <orm/MetaData.h>
#include <orm/DomainObj.h>

namespace Yb {

typedef SingletonHolder<Schema> theSchema;
typedef theSchema theMetaData; // deprecated

inline Schema &init_schema() {
    Schema &schema = theSchema::instance();
    DomainObject::save_registered(schema);
    schema.fill_fkeys();
    return schema;
}
inline Schema &init_default_meta() { return init_schema(); } // deprecated

}

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__SCHEMA_SINGLETON__INCLUDED
