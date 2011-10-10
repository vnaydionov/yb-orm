#ifndef YB__ORM__DOMAIN_FACTORY__INCLUDED
#define YB__ORM__DOMAIN_FACTORY__INCLUDED

#include <map>
#include <stdexcept>
#include <boost/shared_ptr.hpp>
#include "DomainObj.h"

namespace Yb {

typedef boost::shared_ptr<DomainObject> DomainObjectPtr;

class NoCreator: public std::logic_error
{
public:
    NoCreator(const std::string &entity_name) :
        std::logic_error("Domain object creator for entity '" +
                         entity_name + "' not found")
    {}
};

class ICreator
{
public:
    virtual DomainObjectPtr create(Session &session, LongInt id) const = 0;
};

template <typename T>
class DomainCreator: public ICreator
{
public:
    virtual DomainObjectPtr create(Session &session, LongInt id) const
    {
        return DomainObjectPtr(new T(session, id));
    }
};

typedef boost::shared_ptr<ICreator> CreatorPtr;

class DomainFactory
{
public:
    typedef std::map<std::string, CreatorPtr> Map; 

    void register_creator(const std::string &name, CreatorPtr creator)
    {
        creator_map_.insert(Map::value_type(name, creator));
    }
    
    DomainObjectPtr create_object(Session &session, 
            const std::string &entity_name, LongInt id) const
    {
        Map::const_iterator it = creator_map_.find(entity_name);
        if (it == creator_map_.end())
            throw NoCreator(entity_name);
        return it->second->create(session, id);
    }
private:
    Map creator_map_;
};
    
    
} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__DOMAIN_FACTORY__INCLUDED
