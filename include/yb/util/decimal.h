// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__UTIL__DECIMAL__INCLUDED
#define YB__UTIL__DECIMAL__INCLUDED

#include <iosfwd>
#include "util_config.h"
#include "string_type.h"
#include "exception.h"

namespace Yb {

typedef LongInt decimal_numerator;

#if defined(_MSC_VER) || defined(__BORLANDC__)
#define MAX_DECIMAL_NUMERATOR 999999999999999999
#else
#define MAX_DECIMAL_NUMERATOR 999999999999999999LL
#endif

const int MAX_DECIMAL_LENGTH = 18;

class YBUTIL_DECL DecimalException: public ValueError
{
public:
    DecimalException(const String &msg);
};

class YBUTIL_DECL DecimalOverflow: public DecimalException
{
public:
    DecimalOverflow();
};

class YBUTIL_DECL DecimalDivByZero: public DecimalException
{
public:
    DecimalDivByZero();
};

class YBUTIL_DECL DecimalInvalidFormat: public DecimalException
{
public:
    DecimalInvalidFormat();
};

class YBUTIL_DECL Decimal
{
public:
    Decimal(int x = 0, int p = 0);
    explicit Decimal(decimal_numerator x, int p = 0);
    explicit Decimal(const Char *s);
    explicit Decimal(const String &s);
    explicit Decimal(double x);

    Decimal &operator += (const Decimal &x);
    Decimal &operator -= (const Decimal &x);
    Decimal &operator *= (const Decimal &x);
    Decimal &operator /= (const Decimal &x);
    Decimal &operator ++ ();
    const Decimal operator ++ (int);
    Decimal &operator -- ();
    const Decimal operator -- (int);
    const Decimal operator - () const;
    bool is_positive() const;
    int cmp(const Decimal &x) const;

    Decimal &round(int p = 0);
    const Decimal round(int p = 0) const;
    decimal_numerator ipart() const;
    decimal_numerator fpart(int p) const;
    inline int get_precision() const { return precision_; }
    inline decimal_numerator get_value() const { return value_; }
    const String str() const;

private:
    decimal_numerator value_;
    int precision_;
};

YBUTIL_DECL const Decimal operator + (const Decimal &, const Decimal &);
YBUTIL_DECL const Decimal operator - (const Decimal &, const Decimal &);
YBUTIL_DECL const Decimal operator * (const Decimal &, const Decimal &);
YBUTIL_DECL const Decimal operator / (const Decimal &, const Decimal &);
YBUTIL_DECL std::ostream &operator << (std::ostream &o, const Decimal &x);
YBUTIL_DECL std::istream &operator >> (std::istream &i, Decimal &x);
YBUTIL_DECL std::wostream &operator << (std::wostream &o, const Decimal &x);
YBUTIL_DECL std::wistream &operator >> (std::wistream &i, Decimal &x);

inline bool operator == (const Decimal &x, const Decimal &y) { return !x.cmp(y); }
inline bool operator != (const Decimal &x, const Decimal &y) { return x.cmp(y) != 0; }
inline bool operator < (const Decimal &x, const Decimal &y) { return x.cmp(y) < 0; }
inline bool operator > (const Decimal &x, const Decimal &y) { return x.cmp(y) > 0; }
inline bool operator >= (const Decimal &x, const Decimal &y) { return x.cmp(y) >= 0; }
inline bool operator <= (const Decimal &x, const Decimal &y) { return x.cmp(y) <= 0; }

inline const Decimal decimal2(const String &s) { return Decimal(s).round(2); }
inline const Decimal decimal4(const String &s) { return Decimal(s).round(4); }

} // end of namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__UTIL__DECIMAL__INCLUDED
