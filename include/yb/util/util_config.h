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

#ifdef _MSC_VER
#pragma warning(disable:4251 4275)
#endif // _MSC_VER

#if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L \
    || (defined(_MSC_VER) && _MSC_VER >= 1800)
// NOTE: VC++ versions 2010 and newer only operate in C++0x mode,
// but variadic templates are only supported starting from 2013.
#define YB_USE_STDTUPLE
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
