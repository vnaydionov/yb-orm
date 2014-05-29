// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__UTIL__EXCEPTION__INCLUDED
#define YB__UTIL__EXCEPTION__INCLUDED

#include "util_config.h"
#include "string_type.h"
#include <stdexcept>

namespace Yb {

class YBUTIL_DECL BaseError: public std::logic_error
{
    static const String format_base(const String &msg);
public:
    BaseError(const String &msg);
};

class YBUTIL_DECL AssertError: public BaseError
{
    static const String format_assert(const char *file,
        int line, const char *expr);
public:
    AssertError(const char *file, int line, const char *expr);
};

#define YB_ASSERT(X) do { if (!(X)) throw ::Yb::AssertError(\
        __FILE__, __LINE__, #X); } while (0)

class YBUTIL_DECL RunTimeError: public BaseError
{
public:
    RunTimeError(const String &msg);
};

class YBUTIL_DECL KeyError: public RunTimeError
{
public:
    KeyError(const String &msg);
};

class YBUTIL_DECL ValueError: public RunTimeError
{
public:
    ValueError(const String &msg);
};

class YBUTIL_DECL ValueBadCast: public ValueError
{
public:
    ValueBadCast(const String &value, const String &type);
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__UTIL__EXCEPTION__INCLUDED
