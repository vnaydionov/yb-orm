
#include <stdlib.h>
#include "str_utils.hpp"

namespace Yb {

namespace StrUtils {

using namespace std;

bool is_lower(Yb::Char c)
{
    return c >= _T('a') && c <= _T('z');
}

bool is_upper(Yb::Char c)
{
    return c >= _T('A') && c <= _T('Z');
}

bool is_alpha(Yb::Char c)
{
    return is_upper(c) || is_lower(c);
}

bool is_digit(Yb::Char c)
{
    return c >= _T('0') && c <= _T('9');
}

bool is_space(Yb::Char c)
{
    return c == _T(' ') || c == _T('\t') || c == _T('\n') || c == _T('\r');
}

bool is_start_symbol_of_id(Yb::Char c)
{
    return is_alpha(c) || c == _T('_');
}

bool is_symbol_of_id(Yb::Char c)
{
    return is_start_symbol_of_id(c) || is_digit(c);
}

bool is_id(const Yb::String &s)
{
    if (s.empty())
        return false;
    Yb::String::const_iterator it = s.begin(), end = s.end();
    if (!is_start_symbol_of_id(*it))
        return false;
    for (++it; it != end; ++it)
        if (!is_symbol_of_id(*it))
            return false;
    return true;
}

bool starts_with(const Yb::String &s, const Yb::String &subs)
{
    if (s.size() < subs.size())
        return false;
    return s.substr(0, subs.size()) == subs;
}

bool ends_with(const Yb::String &s, const Yb::String &subs)
{
    if (s.size() < subs.size())
        return false;
    return s.substr(s.size() - subs.size()) == subs;
}

const Yb::String substr_after(const Yb::String &s, const Yb::String &subs)
{
    Yb::String::size_type pos = s.find(subs);
    if (pos == Yb::String::npos)
        return Yb::String();
    return s.substr(pos + subs.size());
}

Yb::Char to_lower(Yb::Char c)
{
    if (!is_upper(c))
        return c;
    return c + (_T('a') - _T('A'));
}

Yb::Char to_upper(Yb::Char c)
{
    if (!is_lower(c))
        return c;
    return c + (_T('A') - _T('a'));
}

const Yb::String translate(const Yb::String &s, Yb::Char (*f)(Yb::Char))
{
    Yb::String r;
    r.reserve(s.size());
    Yb::String::const_iterator it = s.begin(), end = s.end();
    for (; it != end; ++it)
        r.push_back(f(*it));
    return r;
}

const Yb::String str_to_lower(const Yb::String &s)
{
    return translate(s, to_lower);
}

const Yb::String str_to_upper(const Yb::String &s)
{
    return translate(s, to_upper);
}

const Yb::String trim_trailing_space(const Yb::String &s)
{
    int i = 0, sz = (int)s.size();
    while (i < sz && is_space(s[i])) ++i;
    int j = sz - 1;
    while (j >= 0 && is_space(s[j])) --j;
    if (i > j)
        return Yb::String();
    return s.substr(i, j - i + 1);
}

const Yb::String sql_string_escape(const Yb::String &s)
{
    Yb::String r;
    r.reserve(s.size() * 2);
    for (size_t pos = 0; pos < s.size(); ++pos) {
        if (s[pos] == _T('\''))
            r.push_back(_T('\''));
        r.push_back(s[pos]);
    }
    return r;
}

const Yb::String html_escape(const Yb::String &s)
{
    Yb::String r;
    r.reserve(s.size() * 2);
    for (size_t i = 0; i < s.size(); ++i)
    {
        Yb::Char c = s[i];
        switch (c)
        {
            case _T('<'):
                r += _T("&lt;");
                break;
            case _T('>'):
                r += _T("&gt;");
                break;
            case _T('&'):
                r += _T("&amp;");
                break;
            case _T('"'):
                r += _T("&quot;");
                break;
            default:
                r += c;
        }
    }
    return r;
}

void
split_path(const Yb::String &path, vector<Yb::String> &items)
{
    split_str_by_chars(path, _T("/"), items);
}

vector<Yb::String> &split_str(const Yb::String &s,
        const Yb::String &delim, vector<Yb::String> &parts)
{
    size_t start = 0;
    while (1) {
        size_t pos = s.find(delim, start);
        if (pos == Yb::String::npos) {
            parts.push_back(Yb::String(s, start));
            break;
        }
        parts.push_back(Yb::String(s, start, pos - start));
        start = pos + delim.size();
    }
    return parts;
}

void split_str_by_chars(const Yb::String &s, const Yb::Char *delim,
        vector<Yb::String> &parts, int limit)
{
    const size_t sz0 = parts.size();
    Yb::String p;
    for (size_t i = 0; i < s.size(); ++i) {
        if (Yb::StrChr(delim, s[i])) {
            if (limit > 0 && !p.empty() &&
                    parts.size() - sz0 >= (size_t)(limit - 1))
                p.push_back(s[i]);
            else if (!p.empty()) {
                parts.push_back(p);
                p.clear();
            }
        }
        else
            p.push_back(s[i]);
    }
    if (!p.empty())
        parts.push_back(p);
}

const Yb::String join_str(const Yb::String &delim, const vector<Yb::String> &parts)
{
    Yb::String result;
    if (parts.size()) {
        result = parts[0];
        for (size_t i = 1; i < parts.size(); ++i) {
            result += delim;
            result += parts[i];
        }
    }
    return result;
}

const Yb::String quote(const Yb::String &s)
{
    return _T("'") + s + _T("'");
}

const Yb::String dquote(const Yb::String &s)
{
    return _T("\"") + s + _T("\"");
}

const Yb::String brackets(const Yb::String &s)
{
    return _T("(") + s + _T(")");
}

const Yb::String comma(const Yb::String &item1, const Yb::String &item2)
{
    return item1 + _T(", ") + item2;
}

const Yb::String xgetenv(const Yb::String &var_name)
{
    char *x;
    Yb::String s;
#if defined(_MSC_VER)
    size_t len;
    errno_t err = _dupenv_s(&x, &len, NARROW(var_name).c_str());
    if (!err) {
        s = Yb::String(WIDEN(x));
        free(x);
    }
#else
    x = getenv(NARROW(var_name).c_str());
    if (x && *x)
        s = Yb::String(WIDEN(x));
#endif
    return s;
}

} // end of namespace StrUtils

} // end of namespace Yb

// vim:ts=4:sts=4:sw=4:et

