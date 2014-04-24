// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__UTIL__CONFIG__INCLUDED
#define YB__UTIL__CONFIG__INCLUDED

#if defined(_WIN64)
#  define YBUTIL_WIN64
#elif defined(__WIN32__) || defined(_WIN32)
#  define YBUTIL_WIN32
#endif

#if (defined(YBUTIL_WIN32) || defined(YBUTIL_WIN64)) && defined(YBUTIL_DLL)
#  if defined(YBUTIL_SOURCE)
#    define YBUTIL_DECL __declspec(dllexport)
#  else
#    define YBUTIL_DECL __declspec(dllimport)
#  endif
#else
#  define YBUTIL_DECL
#endif

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__UTIL__CONFIG__INCLUDED
