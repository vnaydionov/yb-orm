#ifndef YB__ORM__DOMAIN_FACTORY__INCLUDED
#define YB__ORM__DOMAIN_FACTORY__INCLUDED

#include <map>
#include <stdexcept>
#include <util/Utility.h>
#include <util/Singleton.h>
#include <orm/DomainObject.h>

namespace Yb {

class NoCreator: public std::logic_error
{
public:
    NoCreator(const String &entity_name) :
        std::logic_error(NARROW(_T("Domain object creator for entity '") +
                         entity_name + _T("' not found")))
    {}
};

class ICreator
{
public:
    virtual DomainObjectPtr create(Session &session, LongInt id) const = 0;
    virtual ~ICreator() {}
};

template <typename T>
class DomainCreator: public ICreator
{
public:
    virtual DomainObjectPtr create(Session &session, LongInt id) const
    {
        return DomainObjectPtr(new T(session, id));
    }
    virtual ~DomainCreator() {}
};

typedef SharedPtr<ICreator>::Type CreatorPtr;

class DomainFactory
{
public:
    typedef std::map<String, CreatorPtr> Map; 

    void register_creator(const String &name, CreatorPtr creator)
    {
        if (!add_pending_reg(name, creator))
            do_register_creator(name, creator);
    }

    DomainObjectPtr create_object(Session &session, 
            const String &entity_name, LongInt id)
    {
        process_pending();
        Map::const_iterator it = creator_map_.find(entity_name);
        if (it == creator_map_.end())
            throw NoCreator(entity_name);
        return it->second->create(session, id);
    }
private:
    static bool add_pending_reg(const String &name, CreatorPtr creator);
    void process_pending();
    void do_register_creator(const String &name, CreatorPtr creator)
    {
        creator_map_.insert(Map::value_type(name, creator));
    }
    Map creator_map_;
    static char init_[16];
};

typedef SingletonHolder<DomainFactory> theDomainFactory;

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__DOMAIN_FACTORY__INCLUDED
