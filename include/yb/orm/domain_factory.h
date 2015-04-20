// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__ORM__DOMAIN_FACTORY__INCLUDED
#define YB__ORM__DOMAIN_FACTORY__INCLUDED

#include <map>
#include <stdexcept>
#include "util/utility.h"
#include "orm_config.h"
#include "domain_object.h"

namespace Yb {

class YBORM_DECL NoCreator: public ORMError
{
public:
    NoCreator(const String &entity_name);
};

class YBORM_DECL ICreator
{
public:
    virtual DomainObjectPtr create(Session &session, LongInt id) const = 0;
    virtual ~ICreator();
};

typedef SharedPtr<ICreator>::Type CreatorPtr;

template <typename T>
class DomainCreator: public ICreator
{
public:
    virtual DomainObjectPtr create(Session &session, LongInt id) const
    {
        return DomainObjectPtr(new T(session, id));
    }
};

class YBORM_DECL DomainFactory
{
public:
    void register_creator(const String &name, CreatorPtr creator);
    DomainObjectPtr create_object(Session &session,
            const String &entity_name, LongInt id);
private:
    static bool add_pending_reg(const String &name, CreatorPtr creator);
    void process_pending();
    void do_register_creator(const String &name, CreatorPtr creator);
    typedef std::map<String, CreatorPtr> Map;
    Map creator_map_;
    static int init_flag_;
    static void *pending_;
};

YBORM_DECL DomainFactory &theDomainFactory();

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__DOMAIN_FACTORY__INCLUDED
