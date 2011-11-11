// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__UTIL__UNICODE_SUPPORT__INCLUDED
#define YB__UTIL__UNICODE_SUPPORT__INCLUDED

#include <string>
#include <sstream>
#include <cstdio>

#if defined(__WIN32__) || defined(_WIN32)
#include <windows.h>
#else
#undef _T
#ifdef _UNICODE
typedef wchar_t TCHAR;
#define _T(x) L ## x
#else
typedef char TCHAR;
#define _T(x) x
#endif
#endif

#ifdef _UNICODE
#define NARROW(x) ::Yb::narrow(x)
#define WIDEN(x) ::Yb::widen(x)
#else
#define NARROW(x) x
#define WIDEN(x) x
#endif

namespace Yb {

typedef TCHAR Char;
#ifdef _UNICODE
typedef std::wstring String;
typedef std::wistream IStream;
typedef std::wostream OStream;
typedef std::wstringstream StringStream;
typedef std::wostringstream OStringStream;
#define stprintf std::swprintf
#else
typedef std::string String;
typedef std::istream IStream;
typedef std::ostream OStream;
typedef std::stringstream StringStream;
typedef std::ostringstream OStringStream;
#define stprintf std::sprintf
#endif

const std::string narrow(const std::wstring &wide, const std::locale &loc);
const std::string narrow(const std::wstring &wide, const std::string &loc_name = "");
const std::wstring widen(const std::string &narrow, const std::locale &loc);
const std::wstring widen(const std::string &narrow, const std::string &loc_name = "");

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__UTIL__UNICODE_SUPPORT__INCLUDED
