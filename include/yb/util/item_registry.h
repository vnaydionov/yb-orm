// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__UTIL__ITEM_REGISTRY__INCLUDED
#define YB__UTIL__ITEM_REGISTRY__INCLUDED

#include <map>
#include "util/utility.h"
#include "util/thread.h"
#include "util/data_types.h"

namespace Yb {

template <class Item>
class ItemRegistry: NonCopyable
{
public:
    typedef
#if defined(__BORLANDC__)
        SharedPtr<Item>::Type
#else
        typename SharedPtr<Item>::Type
#endif
            ItemPtr;
    typedef std::map<String, ItemPtr> Map;

    ItemRegistry() {}

    Item *find_item(const String &name)
    {
        ScopedLock lock(mutex_);
        typename Map::const_iterator it = map_.find(name);
        if (it != map_.end())
            return shptr_get(it->second);
        return NULL;
    }

    bool register_item(const String &name, std::auto_ptr<Item> item)
    {
        ScopedLock lock(mutex_);
        typename Map::const_iterator it = map_.find(name);
        if (it != map_.end())
            return false;
        map_.insert(
#if defined(__BORLANDC__)
            Map::value_type
#else
            typename Map::value_type
#endif
            (name, ItemPtr(item.release())));
        map_.insert(
#if defined(__BORLANDC__)
            Map::value_type
#else
            typename Map::value_type
#endif
            (name, ItemPtr(item.release())));
        return true;
    }

    const Strings list_items()
    {
        ScopedLock lock(mutex_);
        Strings names;
        typename Map::const_iterator it = map_.begin(), end = map_.end();
        for (; it != end; ++it)
            names.push_back(it->first);
        return names;
    }

    bool empty() {
        ScopedLock lock(mutex_);
        return map_.empty();
    }
private:
    Map map_;
    Mutex mutex_;
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__UTIL__ITEM_REGISTRY__INCLUDED
