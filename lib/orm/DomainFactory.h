
#ifndef YB__DOMAIN_FACTORY__INCLUDED
#define YB__DOMAIN_FACTORY__INCLUDED

#include <map>
#include <stdexcept>
#include <boost/shared_ptr.hpp>
#include "AutoXMLizable.h"

namespace Yb {

namespace  ORMapper {

typedef boost::shared_ptr<Domain::AutoXMLizable> AutoXMLizablePtr;

class NoCreator: public std::logic_error
{
public:
    NoCreator(const std::string &entity_name)
        : logic_error("Domain object creator for entity '" +
                entity_name + "' not found")
    {}
};

class ICreator
{
public:
    virtual AutoXMLizablePtr create(ORMapper::Mapper &mapper, long long id) const = 0;  
};

template <typename T>
class DomainCreator: public ICreator
{
public:
    virtual AutoXMLizablePtr create(ORMapper::Mapper &mapper, long long id) const
    {
        return boost::shared_ptr<Domain::AutoXMLizable>(new T(mapper, id));
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
    
    AutoXMLizablePtr create_object(ORMapper::Mapper &mapper, 
            const std::string &entity_name, long long id) const
    {
        Map::const_iterator it = creator_map_.find(entity_name);
        if (it == creator_map_.end())
            throw NoCreator(entity_name);
        return it->second->create(mapper, id);
    }
private:
    Map creator_map_;
};
    
} // namespace ORMapper
} // namespace Yb

// vim:ts=4:sts=4:sw=4:et

#endif // YB__SESSION__INCLUDED

