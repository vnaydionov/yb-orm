#include "Exception.h"
#include <sstream>
#include "stacktrace.h"

using namespace std;

namespace Yb {

const string BaseError::format_base(const string &msg)
{
    ostringstream out;
    print_stacktrace(out, 100, 2);
    if (out.str().compare("<stack trace not implemented>\n") != 0)
        return msg + "\nBacktrace:\n" + out.str();
    return msg;
}

BaseError::BaseError(const string &msg)
    : logic_error(format_base(msg))
{}

const string AssertError::format_assert(const char *file, int line,
        const char *expr)
{
    ostringstream out;
    out << "Assertion failed in file " << file << ":" << line
        << " (" << expr << ")";
    return out.str();
}

AssertError::AssertError(const char *file, int line, const char *expr)
    : BaseError(format_assert(file, line, expr))
{}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
