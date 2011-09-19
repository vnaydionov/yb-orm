#ifndef YB__UTIL__EXCEPTION__INCLUDED
#define YB__UTIL__EXCEPTION__INCLUDED

#include <string>
#include <stdexcept>

namespace Yb {

class BaseError: public std::logic_error {
    static const std::string format_base(const std::string &msg);
public:
    BaseError(const std::string &msg);
};

class AssertError: public BaseError {
    static const std::string format_assert(const char *file,
        int line, const char *expr);
public:
    AssertError(const char *file, int line, const char *expr);
};

#define YB_ASSERT(X) do { if (!(X)) throw AssertError(__FILE__, __LINE__, \
    #X); } while (0)

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__UTIL__EXCEPTION__INCLUDED
