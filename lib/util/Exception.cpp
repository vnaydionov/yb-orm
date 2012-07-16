#include <util/Exception.h>
#include <sstream>
#include <util/stacktrace.h>

namespace Yb {

const String BaseError::format_base(const String &msg)
{
    std::ostringstream out;
    print_stacktrace(out, 100, 2);
    if (out.str().compare("<stack trace not implemented>\n") != 0)
        return msg + _T("\nBacktrace:\n") + WIDEN(out.str());
    return msg;
}

BaseError::BaseError(const String &msg)
    : std::logic_error(NARROW(format_base(msg)))
{}

const String AssertError::format_assert(const char *file, int line,
        const char *expr)
{
    std::ostringstream out;
    out << "Assertion failed in file " << file << ":" << line
        << " (" << expr << ")";
    return WIDEN(out.str());
}

AssertError::AssertError(const char *file, int line, const char *expr)
    : BaseError(format_assert(file, line, expr))
{}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
