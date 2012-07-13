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
#if QT_VERSION >= 0x040500
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
#if QT_VERSION >= 0x040500
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
