#ifndef YB__UTIL__DECIMAL__INCLUDED
#define YB__UTIL__DECIMAL__INCLUDED

#include <exception>
#include <string>
#include <iosfwd>
#include <util/String.h>
#include <util/Exception.h>

#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef __int64 decimal_numerator;
#else
typedef long long decimal_numerator;
#endif

#if defined(_MSC_VER) || defined(__BORLANDC__)
#define MAX_DECIMAL_NUMERATOR 999999999999999999
#else
#define MAX_DECIMAL_NUMERATOR 999999999999999999LL
#endif

const int MAX_DECIMAL_LENGTH = 18;

class decimal
{
public:
    class exception: public Yb::RunTimeError
    {
    public:
        exception(const Yb::String &msg): Yb::RunTimeError(msg) {}
    };

    class overflow: public exception
    {
    public:
        overflow(): exception(_T("Decimal exception: overflow")) {}
    };

    class divizion_by_zero: public exception
    {
    public:
        divizion_by_zero(): exception(_T("Decimal exception: divizion by zero")) {}
    };

    class invalid_format: public exception
    {
    public:
        invalid_format(): exception(_T("Decimal exception: invalid format")) {}
    };

public:
    decimal(int x = 0, int p = 0);
    explicit decimal(decimal_numerator x, int p = 0);
    explicit decimal(const Yb::Char *s);
    explicit decimal(const Yb::String &s);
    explicit decimal(double x);

    decimal &operator += (const decimal &x);
    decimal &operator -= (const decimal &x);
    decimal &operator *= (const decimal &x);
    decimal &operator /= (const decimal &x);
    decimal &operator ++ ();
    const decimal operator ++ (int);
    decimal &operator -- ();
    const decimal operator -- (int);
    const decimal operator - () const;
    bool is_positive() const;
    int cmp(const decimal &x) const;

    decimal &round(int p = 0);
    const decimal round(int p = 0) const;
    decimal_numerator ipart() const;
    decimal_numerator fpart(int p) const;
    inline int get_precision() const { return precision_; }
    inline decimal_numerator get_value() const { return value_; }
    const Yb::String str() const;

private:
    decimal_numerator value_;
    int precision_;
};

const decimal operator + (const decimal &, const decimal &);
const decimal operator - (const decimal &, const decimal &);
const decimal operator * (const decimal &, const decimal &);
const decimal operator / (const decimal &, const decimal &);
std::ostream &operator << (std::ostream &o, const decimal &x);
std::istream &operator >> (std::istream &i, decimal &x);
std::wostream &operator << (std::wostream &o, const decimal &x);
std::wistream &operator >> (std::wistream &i, decimal &x);

inline bool operator == (const decimal &x, const decimal &y) { return !x.cmp(y); }
inline bool operator != (const decimal &x, const decimal &y) { return x.cmp(y) != 0; }
inline bool operator < (const decimal &x, const decimal &y) { return x.cmp(y) < 0; }
inline bool operator > (const decimal &x, const decimal &y) { return x.cmp(y) > 0; }
inline bool operator >= (const decimal &x, const decimal &y) { return x.cmp(y) >= 0; }
inline bool operator <= (const decimal &x, const decimal &y) { return x.cmp(y) <= 0; }

inline const decimal decimal2(const Yb::String &s) { return decimal(s).round(2); }
inline const decimal decimal4(const Yb::String &s) { return decimal(s).round(4); }

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__UTIL__DECIMAL__INCLUDED
