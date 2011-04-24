
#include <stdlib.h>
#include "str_utils.hpp"

namespace Yb {

namespace StrUtils {

using namespace std;

bool is_lower(char c)
{
    return c >= 'a' && c <= 'z';
}

bool is_upper(char c)
{
    return c >= 'A' && c <= 'Z';
}

bool is_alpha(char c)
{
    return is_upper(c) || is_lower(c);
}

bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

bool is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool is_start_symbol_of_id(char c)
{
    return is_alpha(c) || c == '_';
}

bool is_symbol_of_id(char c)
{
    return is_start_symbol_of_id(c) || is_digit(c);
}

bool is_id(const string &s)
{
    if (s.empty())
        return false;
    string::const_iterator it = s.begin(), end = s.end();
    if (!is_start_symbol_of_id(*it))
        return false;
    for (++it; it != end; ++it)
        if (!is_symbol_of_id(*it))
            return false;
    return true;
}

bool starts_with(const string &s, const string &subs)
{
    if (s.size() < subs.size())
        return false;
    return s.substr(0, subs.size()) == subs;
}

bool ends_with(const string &s, const string &subs)
{
    if (s.size() < subs.size())
        return false;
    return s.substr(s.size() - subs.size()) == subs;
}

const string substr_after(const string &s, const string &subs)
{
    string::size_type pos = s.find(subs);
    if (pos == string::npos)
        return "";
    return s.substr(pos + subs.size());
}

char to_lower(char c)
{
    if (!is_upper(c))
        return c;
    return c + ('a' - 'A');
}

char to_upper(char c)
{
    if (!is_lower(c))
        return c;
    return c + ('A' - 'a');
}

const string translate(const string &s, char (*f)(char))
{
    string r;
    r.reserve(s.size());
    string::const_iterator it = s.begin(), end = s.end();
    for (; it != end; ++it)
        r.push_back(f(*it));
    return r;
}

const string str_to_lower(const string &s)
{
    return translate(s, to_lower);
}

const string str_to_upper(const string &s)
{
    return translate(s, to_upper);
}

const string trim_trailing_space(const string &s)
{
    int i = 0, sz = (int)s.size();
    while (i < sz && is_space(s[i])) ++i;
    int j = sz - 1;
    while (j >= 0 && is_space(s[j])) --j;
    if (i > j)
        return "";
    return s.substr(i, j - i + 1);
}

const string sql_string_escape(const string &s)
{
    string r;
    r.reserve(s.size() * 2);
    for (size_t pos = 0; pos < s.size(); ++pos) {
        if (s[pos] == '\'')
            r.push_back('\'');
        r.push_back(s[pos]);
    }
    return r;
}

const string html_escape(const string &s)
{
    string r;
    r.reserve(s.size() * 2);
    for (size_t i = 0; i < s.size(); ++i)
    {
        char c = s[i];
        switch (c)
        {
            case '<':
                r += "&lt;";
                break;
            case '>':
                r += "&gt;";
                break;
            case '&':
                r += "&amp;";
                break;
            case '"':
                r += "&quot;";
                break;
            default:
                r += c;
        }
    }
    return r;
}

static inline
void
append_path_item(string &item, vector<string> &items)
{
    if (!item.empty()) {
        items.push_back(item);
        item = "";
    }
}

void
split_path(const string &path, vector<string> &items)
{
    string item;
    string::const_iterator it = path.begin(), end = path.end();
    for (; it != end; ++it) {
        if (*it == '/')
            append_path_item(item, items);
        else
            item.push_back(*it);
    }
    append_path_item(item, items);
}

const string quote(const string &s)
{
    return "'" + s + "'";
}

const string dquote(const string &s)
{
    return "\"" + s + "\"";
}

const string brackets(const string &s)
{
    return "(" + s + ")";
}

const string comma(const string &item1, const string &item2)
{
    return item1 + ", " + item2;
}

const string xgetenv(const string &var_name)
{
    char *x;
    string s;
#if defined(_MSC_VER)
    size_t len;
    errno_t err = _dupenv_s(&x, &len, var_name.c_str());
    if (!err) {
        s = string(x);
        free(x);
    }
#else
    x = getenv(var_name.c_str());
    if (x && *x)
        s = string(x);
#endif
    return s;
}

} // end of namespace StrUtils

} // end of namespace Yb

// vim:ts=4:sts=4:sw=4:et

