// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__UTIL__UTILITY__INCLUDED
#define YB__UTIL__UTILITY__INCLUDED

#if defined(YB_USE_WX)
#include <wx/version.h>
#if wxCHECK_VERSION(2, 9, 0)
#include <wx/sharedptr.h>
#else
#include <boost/shared_ptr.hpp>
#endif
#elif defined(YB_USE_QT)
#include <QChar>
#if QT_VERSION >= 0x040600
#include <QSharedPointer>
#else
#include <boost/shared_ptr.hpp>
#endif
#else
#include <boost/shared_ptr.hpp>
#endif

namespace Yb {

#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef __int64 LongInt;
#else
typedef long long LongInt;
#endif

#if defined(YB_USE_WX)
#if wxCHECK_VERSION(2, 9, 0)
template<class X> struct SharedPtr { typedef wxSharedPtr<X> Type; };
template<class X>
inline X *shptr_get(const wxSharedPtr<X> &p) { return p.get(); }
#else
template<class X> struct SharedPtr { typedef boost::shared_ptr<X> Type; };
template<class X>
inline X *shptr_get(const boost::shared_ptr<X> &p) { return p.get(); }
#endif
#elif defined(YB_USE_QT)
#if QT_VERSION >= 0x040600
template<class X> struct SharedPtr { typedef QSharedPointer<X> Type; };
template<class X>
inline X *shptr_get(const QSharedPointer<X> &p) { return p.data(); }
#else
template<class X> struct SharedPtr { typedef boost::shared_ptr<X> Type; };
template<class X>
inline X *shptr_get(const boost::shared_ptr<X> &p) { return p.get(); }
#endif
#else
template<class X> struct SharedPtr { typedef boost::shared_ptr<X> Type; };
template<class X>
inline X *shptr_get(const boost::shared_ptr<X> &p) { return p.get(); }
#endif

template <class T__>
class IntrusivePtr
{
    T__ *p_;
public:
    IntrusivePtr(): p_(0) {}
    explicit IntrusivePtr(T__ *p): p_(p) {
        if (p_)
            p_->add_ref();
    }
    IntrusivePtr(const IntrusivePtr &other): p_(other.p_) {
        if (p_)
            p_->add_ref();
    }
    IntrusivePtr &operator = (const IntrusivePtr &other) {
        if (this != &other && p_ != other.p_) {
            if (p_)
                p_->release();
            p_ = other.p_;
            if (p_)
                p_->add_ref();
        }
        return *this;
    }
    ~IntrusivePtr() {
        if (p_)
            p_->release();
    }
    T__ *release() {
        T__ *p = p_;
        if (p_)
            p_->release();
        return p;
    }
    T__ &operator * () const { return *p_; }
    T__ *operator -> () const { return p_; }
    T__ *get() const { return p_; }
};

template <class T__>
inline bool operator == (const IntrusivePtr<T__> &a, const IntrusivePtr<T__> &b)
{ return a.get() == b.get(); }

template <class T__>
inline bool operator != (const IntrusivePtr<T__> &a, const IntrusivePtr<T__> &b)
{ return a.get() != b.get(); }

template <class T__>
inline bool operator < (const IntrusivePtr<T__> &a, const IntrusivePtr<T__> &b)
{ return a.get() < b.get(); }

template <class T__>
inline bool operator > (const IntrusivePtr<T__> &a, const IntrusivePtr<T__> &b)
{ return a.get() > b.get(); }

template <class T__>
inline bool operator <= (const IntrusivePtr<T__> &a, const IntrusivePtr<T__> &b)
{ return a.get() <= b.get(); }

template <class T__>
inline bool operator >= (const IntrusivePtr<T__> &a, const IntrusivePtr<T__> &b)
{ return a.get() >= b.get(); }

class RefCountBase {
protected:
    int ref_count_;
public:
    RefCountBase(): ref_count_(0) {}
    virtual ~RefCountBase() {}
    void add_ref() { ++ref_count_; }
    void release() { if (!--ref_count_) delete this; }
};

class NonCopyable {
    NonCopyable(const NonCopyable &);
    const NonCopyable &operator=(const NonCopyable &);
protected:
    NonCopyable() {}
    ~NonCopyable() {}
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__UTIL__UTILITY__INCLUDED
