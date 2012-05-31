#include <util/decimal.h>
#include <sstream>
#include <stdexcept>
#include <util/str_utils.hpp>

using namespace std;
using namespace Yb::StrUtils;

namespace { // Anonymous namespace

decimal_numerator my_abs(decimal_numerator x)
{
    if (x < 0)
        return -x;
    return x;
}

int sign(decimal_numerator x)
{
    if (x < 0)
        return -1;
    if (x > 0)
        return 1;
    return 0;
}

int log10(decimal_numerator x)
{
    for (int p = 0; ; ++p) {
        decimal_numerator xx = x / 10;
        if (!xx)
            return p;
        x = xx;
    }
}

decimal_numerator add(decimal_numerator x, decimal_numerator y)
{
    if (x && sign(x) == sign(y))
    {
        decimal_numerator ax(my_abs(x)), ay(my_abs(y));
        int lx(log10(ax)), ly(log10(ay));
        if (ly + 1 == MAX_DECIMAL_LENGTH)
        {
            decimal_numerator at(ay);
            int lt(ly);
            ay = ax;
            ly = lx;
            ax = at;
            lx = lt;
        }
        if (lx + 1 == MAX_DECIMAL_LENGTH)
        {
            decimal_numerator d = MAX_DECIMAL_NUMERATOR - ax;
            if (ay > d)
                throw decimal::overflow();
        }
    }
    return x + y;
}

decimal_numerator mul(decimal_numerator x, decimal_numerator y)
{
    if (x && y)
    {
        decimal_numerator ax(my_abs(x)), ay(my_abs(y));
        int lx(log10(ax)), ly(log10(ay));
        if (lx + ly + 1 > MAX_DECIMAL_LENGTH)
            throw decimal::overflow();
        if (lx + ly + 1 == MAX_DECIMAL_LENGTH)
        {
            if (ay > ax)
            {
                decimal_numerator at(ay);
                ay = ax;
                ax = at;
            }
            decimal_numerator n = MAX_DECIMAL_NUMERATOR / ay;
            if (ax > n)
                throw decimal::overflow();
        }
    }
    return x * y;
}

decimal_numerator my_div(decimal_numerator x, decimal_numerator y)
{
    if (!y)
        throw decimal::divizion_by_zero();
    return x / y;
}

int check_precision(int p)
{
    if (p < 0 || p > MAX_DECIMAL_LENGTH)
        throw decimal::invalid_format();
    return p;
}

decimal_numerator check_value(decimal_numerator x)
{
    if (my_abs(x) > MAX_DECIMAL_NUMERATOR)
        throw decimal::invalid_format();
    return x;
}

void check_format(decimal_numerator value, int precision)
{
    check_precision(precision);
    check_value(value);
}

struct scale
{
    decimal_numerator x[MAX_DECIMAL_LENGTH + 1];
    scale();
};

static scale SCALE;

scale::scale()
{
    decimal_numerator n = 1;
    for (int i = 0; i <= MAX_DECIMAL_LENGTH; ++i) {
        x[i] = n;
        n *= 10;
    }
}

void init_from_str(const Yb::Char *s, decimal_numerator &value, int &precision)
{
    while (is_space(*s)) ++s;
    value = 0;
    precision = 0;
    bool negative = false;
    if (*s == _T('-'))
    {
        negative = true;
        ++s;
    }
    else if (*s == _T('+'))
        ++s;
    int digits = 0;
    for (bool decimal = false; ; ++s)
    {
        if (is_digit(*s))
        {
            value = add(mul(value, 10), Yb::char_code(*s) - _T('0'));
            if (decimal)
                ++precision;
            ++digits;
        }
        else if ((*s == _T('.') || *s == _T(',')) && !decimal) // RUSSIAN LOCALE SPECIFIC!
            decimal = true;
        else if (is_space(*s)) // RUSSIAN LOCALE SPECIFIC!
            continue;
        else
            break;
    }
    while (is_space(*s)) ++s;
    if (Yb::char_code(*s) || !digits)
        throw decimal::invalid_format();
    if (negative)
        value = -value;
}

void equalize_precision(
        decimal_numerator &value, int &precision, int x_precision)
{
    value = mul(value, SCALE.x[x_precision - precision]);
    precision = x_precision;
}

void equalize_precision(
        decimal_numerator &x_value, int &x_precision,
        decimal_numerator &y_value, int &y_precision)
{
    if (y_precision < x_precision)
        equalize_precision(y_value, y_precision, x_precision);
    else if (x_precision < y_precision)
        equalize_precision(x_value, x_precision, y_precision);
}

void normalize(decimal_numerator &value, int &precision)
{
    if (!value)
        precision = 0;
    else
        for (; precision > 0 && !(value % 10); --precision)
            value /= 10;
}

} // end of Anonymous namespace

decimal::decimal(int x, int p):
    value_(x),
    precision_(p)
{
    check_format(value_, precision_);
    normalize(value_, precision_);
}

decimal::decimal(decimal_numerator x, int p):
    value_(x),
    precision_(p)
{
    check_format(value_, precision_);
    normalize(value_, precision_);
}

decimal::decimal(const Yb::Char *s):
    value_(0),
    precision_(0)
{
    init_from_str(s, value_, precision_);
    check_format(value_, precision_);
    normalize(value_, precision_);
}

decimal::decimal(const Yb::String &s):
    value_(0),
    precision_(0)
{
    init_from_str(Yb::str_data(s), value_, precision_);
    check_format(value_, precision_);
    normalize(value_, precision_);
}

decimal::decimal(double x):
    value_(0),
    precision_(0)
{
    std::ostringstream s;
    s << std::fixed << x;
    init_from_str(Yb::str_data(WIDEN(s.str())), value_, precision_);
    check_format(value_, precision_);
    normalize(value_, precision_);
}

decimal &decimal::operator += (const decimal &x)
{
    decimal t(x);
    equalize_precision(value_, precision_, t.value_, t.precision_);
    value_ = add(value_, t.value_);
    normalize(value_, precision_);
    return *this;
}

decimal &decimal::operator -= (const decimal &x)
{
    decimal t(x);
    equalize_precision(value_, precision_, t.value_, t.precision_);
    value_ = add(value_, -t.value_);
    normalize(value_, precision_);
    return *this;
}

decimal &decimal::operator *= (const decimal &x)
{
    value_ = mul(value_, x.value_);
    precision_ += x.precision_;
    normalize(value_, precision_);
    return *this;
}

decimal &decimal::operator /= (const decimal &x)
{
    if (!x.value_)
        throw divizion_by_zero();
    if (!value_) {
        *this = decimal();
        return *this;
    }
    decimal_numerator xx = value_;
    int p = 0;
    while (xx != 0) {
        try {
            xx = mul(xx, 10);
            ++p;
        }
        catch (const decimal::overflow &) {
            break;
        }
    }
    *this = decimal(my_div(xx, x.value_), precision_ - x.precision_ + p);
    return *this;
}

decimal &decimal::operator ++ ()
{
    return *this += 1;
}

const decimal decimal::operator ++ (int)
{
    decimal t(*this);
    ++*this;
    return t;
}

decimal &decimal::operator -- ()
{
    return *this -= 1;
}

const decimal decimal::operator -- (int)
{
    decimal t(*this);
    --*this;
    return t;
}

const decimal decimal::operator - () const
{
    decimal t(*this);
    t.value_ = -t.value_;
    return t;
}

bool decimal::is_positive() const
{
    return value_ > 0;
}

int decimal::cmp(const decimal &x) const
{
    decimal t(*this), u(x);
    equalize_precision(t.value_, t.precision_, u.value_, u.precision_);
    return t.value_ == u.value_? 0: (t.value_ < u.value_? -1: 1);
}

decimal &decimal::round(int p)
{
    check_precision(p);
    if (!value_ || p >= precision_)
        return *this;
    *this = decimal(
        my_div(my_abs(value_) + 5 * SCALE.x[precision_ - p - 1],
            SCALE.x[precision_ - p]) * sign(value_),
        p);
    return *this;
}

const decimal decimal::round(int p) const
{
    decimal t(*this);
    t.round(p);
    return t;
}

decimal_numerator decimal::ipart() const
{
    return my_abs(value_) / SCALE.x[precision_];
}

decimal_numerator decimal::fpart(int p) const
{
    decimal_numerator x = my_abs(round(p).value_);
    return mul(x % SCALE.x[precision_], SCALE.x[p - precision_]);
}

const Yb::String decimal::str() const
{
    Yb::String s;
    decimal_numerator x = value_;
    bool negative = false;
    if (x < 0)
    {
        x = -x;
        negative = true;
    }
    for (int n = 0; ; ++n)
    {
        if (n == precision_ && n > 0)
            s = _T(".") + s;
        Yb::Char buf[2];
        buf[0] = Yb::Char((int)(_T('0') + x % 10));
        buf[1] = Yb::Char(0);
        s = Yb::String(buf) + s;
        x = x / 10;
        if (!x && n >= precision_)
            break;
    }
    if (negative)
        s = _T("-") + s;
    return s;
}

const decimal operator + (const decimal &x, const decimal &y)
{
    decimal t(x);
    t += y;
    return t;
}

const decimal operator - (const decimal &x, const decimal &y)
{
    decimal t(x);
    t -= y;
    return t;
}

const decimal operator * (const decimal &x, const decimal &y)
{
    decimal t(x);
    t *= y;
    return t;
}

const decimal operator / (const decimal &x, const decimal &y)
{
    decimal t(x);
    t /= y;
    return t;
}

std::ostream &operator << (std::ostream &o, const decimal &x)
{
    int prec = o.precision();
    if (prec < 0)
        prec = 0;
    else if (prec > MAX_DECIMAL_LENGTH)
        prec = MAX_DECIMAL_LENGTH;
    decimal y = x;
    bool rounded = false;
    if (!(o.flags() & ios_base::fixed))
    {
        y = x.round(prec);
        rounded = true;
    }
    Yb::String s = y.str();
    if (o.flags() & ios_base::showpos)
    {
        if (x > decimal(0))
            s = _T("+") + s;
        if (x == decimal(0))
            s = _T(" ") + s;
    }
    if (rounded && o.precision() > y.get_precision())
    {
        if (y.get_precision() == 0)
            s += _T('.');
        s += Yb::str_n(o.precision() - y.get_precision(), _T('0'));
    }
    o << NARROW(s);
    return o;
}

std::istream &operator >> (std::istream &i, decimal &x)
{
    std::string buf;
    i >> buf;
    try {
        x = decimal(WIDEN(buf));
    }
    catch (const decimal::exception &) {
        i.setstate(ios_base::failbit);
    }
    return i;
}

std::wostream &operator << (std::wostream &o, const decimal &x)
{
    std::ostringstream out;
    out << x;
    o << Yb::fast_widen(out.str());
    return o;
}

std::wistream &operator >> (std::wistream &i, decimal &x)
{
    std::wstring buf;
    i >> buf;
    try {
        x = decimal(WIDEN(Yb::fast_narrow(buf)));
    }
    catch (const decimal::exception &) {
        i.setstate(ios_base::failbit);
    }
    catch (const std::runtime_error &) {
        i.setstate(ios_base::failbit);
    }
    return i;
}

// vim:ts=4:sts=4:sw=4:et:
