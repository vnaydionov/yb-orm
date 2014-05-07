// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__UTIL__UTIL_CONFIG__INCLUDED
#define YB__UTIL__UTIL_CONFIG__INCLUDED

#if defined(_WIN64)
#  define YBUTIL_WIN64
#elif defined(__WIN32__) || defined(_WIN32)
#  define YBUTIL_WIN32
#endif

#if defined(YBUTIL_WIN32) || defined(YBUTIL_WIN64)
#  define YBUTIL_WINDOWS
#endif

#if defined(YBUTIL_WINDOWS) && defined(YBUTIL_DLL)
#  if defined(YBUTIL_SOURCE)
#    define YBUTIL_DECL __declspec(dllexport)
#  else
#    define YBUTIL_DECL __declspec(dllimport)
#  endif
#else
#  define YBUTIL_DECL
#endif

namespace Yb {

#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef __int64 LongInt;
#else
typedef long long LongInt;
#endif

} // end of namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__UTIL__UTIL_CONFIG__INCLUDED
