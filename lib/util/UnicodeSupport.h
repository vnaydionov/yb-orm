// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__UTIL__UNICODE_SUPPORT__INCLUDED
#define YB__UTIL__UNICODE_SUPPORT__INCLUDED

#include <string>
#include <sstream>
#include <cstdio>

#undef _T
#ifdef _UNICODE
#define _T(x) L ## x
#else
#define _T(x) x
#endif

#ifdef _UNICODE
#define NARROW(x) ::Yb::narrow(x)
#define WIDEN(x) ::Yb::widen(x)
#else
#define NARROW(x) x
#define WIDEN(x) x
#endif

namespace Yb {

#ifdef _UNICODE
typedef wchar_t Char;
typedef std::wstring String;
typedef std::wistream IStream;
typedef std::wostream OStream;
typedef std::wstringstream StringStream;
typedef std::wostringstream OStringStream;
#else
typedef char Char;
typedef std::string String;
typedef std::istream IStream;
typedef std::ostream OStream;
typedef std::stringstream StringStream;
typedef std::ostringstream OStringStream;
#endif

const std::string narrow(const std::wstring &wide, const std::locale &loc);
const std::string narrow(const std::wstring &wide, const std::string &loc_name = "");
const std::wstring widen(const std::string &narrow, const std::locale &loc);
const std::wstring widen(const std::string &narrow, const std::string &loc_name = "");

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__UTIL__UNICODE_SUPPORT__INCLUDED
