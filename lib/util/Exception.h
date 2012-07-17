// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__UTIL__EXCEPTION__INCLUDED
#define YB__UTIL__EXCEPTION__INCLUDED

#include <util/String.h>
#include <stdexcept>

namespace Yb {

class BaseError: public std::logic_error {
    static const String format_base(const String &msg);
public:
    BaseError(const String &msg);
};

class AssertError: public BaseError {
    static const String format_assert(const char *file,
        int line, const char *expr);
public:
    AssertError(const char *file, int line, const char *expr);
};

#define YB_ASSERT(X) do { if (!(X)) throw ::Yb::AssertError(__FILE__, __LINE__, \
    #X); } while (0)

typedef BaseError RunTimeError;

class KeyError : public RunTimeError
{
public:
    KeyError(const String &msg): RunTimeError(msg) {}
};

class ValueError : public RunTimeError
{
public:
    ValueError(const String &msg): RunTimeError(msg) {}
};

class ValueBadCast : public ValueError
{
public:
    ValueBadCast(const String &value, const String &type)
        :ValueError(_T("Can't cast value '") + value + _T("' to type ") + type)
    {}
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__UTIL__EXCEPTION__INCLUDED
