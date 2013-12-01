#ifndef YB__UTIL__SINGLETON___INCLUDED
#define YB__UTIL__SINGLETON___INCLUDED

#include <memory>
#include <util/Thread.h>

class TestSingleton;

namespace Yb {

template <class T>
class SingletonHolder
{
    friend class ::TestSingleton;

    std::auto_ptr<T> instance_;
    std::auto_ptr<Mutex> mutex_;

    static SingletonHolder singleton;

    SingletonHolder(const SingletonHolder &);
    SingletonHolder &operator=(const SingletonHolder &);
public:
    SingletonHolder()
    {}
    T *get()
    {
        if (!mutex_.get())
            mutex_.reset(new Mutex);
        ScopedLock lock(*mutex_);
        if (!instance_.get())
            instance_.reset(new T());
        return instance_.get();
    }
    static T &instance()
    {
        return *singleton.get();
    }
};

template <class T>
SingletonHolder<T> SingletonHolder<T>::singleton;

} // end of namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__UTIL__SINGLETON___INCLUDED
