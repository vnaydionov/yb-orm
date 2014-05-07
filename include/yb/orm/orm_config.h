// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__ORM__ORM_CONFIG__INCLUDED
#define YB__ORM__ORM_CONFIG__INCLUDED

#if defined(_WIN64)
#  define YBORM_WIN64
#elif defined(__WIN32__) || defined(_WIN32)
#  define YBORM_WIN32
#endif

#if defined(YBORM_WIN32) || defined(YBORM_WIN64)
#  define YBORM_WINDOWS
#endif

#if defined(YBORM_WINDOWS) && defined(YBORM_DLL)
#  if defined(YBORM_SOURCE)
#    define YBORM_DECL __declspec(dllexport)
#  else
#    define YBORM_DECL __declspec(dllimport)
#  endif
#else
#  define YBORM_DECL
#endif

#ifdef _MSC_VER
#pragma warning(disable:4251 4275)
#endif // _MSC_VER

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__ORM_CONFIG__INCLUDED
