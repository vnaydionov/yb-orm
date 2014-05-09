// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#define YBUTIL_SOURCE

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "util/string_utils.h"

namespace Yb {

namespace StrUtils {

using namespace std;

YBUTIL_DECL bool is_lower(Yb::Char c)
{
    return c >= _T('a') && c <= _T('z');
}

YBUTIL_DECL bool is_upper(Yb::Char c)
{
    return c >= _T('A') && c <= _T('Z');
}

YBUTIL_DECL bool is_alpha(Yb::Char c)
{
    return is_upper(c) || is_lower(c);
}

YBUTIL_DECL bool is_digit(Yb::Char c)
{
    return c >= _T('0') && c <= _T('9');
}

YBUTIL_DECL bool is_space(Yb::Char c)
{
    return c == _T(' ') || c == _T('\t') || c == _T('\n') || c == _T('\r');
}

YBUTIL_DECL bool is_start_symbol_of_id(Yb::Char c)
{
    return is_alpha(c) || c == _T('_');
}

YBUTIL_DECL bool is_symbol_of_id(Yb::Char c)
{
    return is_start_symbol_of_id(c) || is_digit(c);
}

YBUTIL_DECL bool is_id(const Yb::String &s)
{
    if (str_empty(s))
        return false;
    Yb::String::const_iterator it = s.begin(), end = s.end();
    if (!is_start_symbol_of_id(*it))
        return false;
    for (++it; it != end; ++it)
        if (!is_symbol_of_id(*it))
            return false;
    return true;
}

YBUTIL_DECL bool look_like_absolute_path(const Yb::String &s)
{
    return !str_empty(s) && (
            char_code(s[0]) == '/' || (str_length(s) > 2 && is_alpha(s[0])
                && char_code(s[1]) == ':' && char_code(s[2]) == '/'));
}

YBUTIL_DECL bool starts_with(const Yb::String &s, const Yb::String &subs)
{
    if (s.size() < subs.size())
        return false;
    return str_substr(s, 0, subs.size()) == subs;
}

YBUTIL_DECL bool ends_with(const Yb::String &s, const Yb::String &subs)
{
    if (s.size() < subs.size())
        return false;
    return str_substr(s, s.size() - subs.size()) == subs;
}

YBUTIL_DECL const Yb::String substr_after(const Yb::String &s, const Yb::String &subs)
{
    int pos = str_find(s, subs);
    if (pos == -1)
        return Yb::String();
    return str_substr(s, pos + subs.size());
}

YBUTIL_DECL Yb::Char to_lower(Yb::Char c)
{
    if (!is_upper(c))
        return c;
    return Yb::Char(char_code(c) + (_T('a') - _T('A')));
}

YBUTIL_DECL Yb::Char to_upper(Yb::Char c)
{
    if (!is_lower(c))
        return c;
    return Yb::Char(char_code(c) + (_T('A') - _T('a')));
}

YBUTIL_DECL const Yb::String translate(const Yb::String &s, Yb::Char (*f)(Yb::Char))
{
    Yb::String r;
    r.reserve(s.size());
    Yb::String::const_iterator it = s.begin(), end = s.end();
    for (; it != end; ++it)
        str_append(r, f(*it));
    return r;
}

YBUTIL_DECL const Yb::String str_to_lower(const Yb::String &s)
{
    return translate(s, to_lower);
}

YBUTIL_DECL const Yb::String str_to_upper(const Yb::String &s)
{
    return translate(s, to_upper);
}

YBUTIL_DECL const Yb::String trim_trailing_space(const Yb::String &s)
{
    int i = 0, sz = (int)s.size();
    while (i < sz && is_space(s[i])) ++i;
    int j = sz - 1;
    while (j >= 0 && is_space(s[j])) --j;
    if (i > j)
        return Yb::String();
    return str_substr(s, i, j - i + 1);
}

YBUTIL_DECL const Yb::String sql_string_escape(const Yb::String &s)
{
    Yb::String r;
    r.reserve(str_length(s) * 2);
    for (size_t pos = 0; pos < str_length(s); ++pos) {
        if (s[pos] == _T('\''))
            str_append(r, _T('\''));
        str_append(r, s[pos]);
    }
    return r;
}

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif // _MSC_VER

YBUTIL_DECL const Yb::String c_string_escape(const Yb::String &s)
{
    Yb::String r;
    r.reserve(str_length(s) * 2);
    for (size_t pos = 0; pos < str_length(s); ++pos) {
        if (char_code(s[pos]) == _T('\''))
            str_append(r, _T("\\\'"));
        else if (char_code(s[pos]) == _T('\"'))
            str_append(r, _T("\\\""));
        else if (char_code(s[pos]) == _T('\n'))
            str_append(r, _T("\\n"));
        else if (char_code(s[pos]) == _T('\r'))
            str_append(r, _T("\\r"));
        else if (char_code(s[pos]) == _T('\t'))
            str_append(r, _T("\\t"));
        else if (char_code(s[pos]) == _T('\\'))
            str_append(r, _T("\\\\"));
#if defined(YB_USE_UNICODE)
        else if (char_code(s[pos]) < _T('\x20')
                || char_code(s[pos]) >= _T('\x7F'))
#else
        else if ((char_code(s[pos]) >= _T('\0')
                  && char_code(s[pos]) < _T('\x20'))
                 || char_code(s[pos]) == _T('\x7F'))
#endif
        {
            char buf[20];
#if defined(YB_USE_UNICODE)
            sprintf(buf, "\\u%04x", char_code(s[pos]));
#else
            sprintf(buf, "\\x%02x", char_code(s[pos]));
#endif
            str_append(r, WIDEN(buf));
        }
        else
            str_append(r, s[pos]);
    }
    return r;
}

YBUTIL_DECL const Yb::String html_escape(const Yb::String &s)
{
    Yb::String r;
    r.reserve(str_length(s) * 2);
    for (size_t i = 0; i < str_length(s); ++i)
    {
        Yb::Char c = s[i];
        switch (char_code(c))
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

YBUTIL_DECL void
split_path(const Yb::String &path, vector<Yb::String> &items)
{
    split_str_by_chars(path, _T("/"), items);
}

YBUTIL_DECL vector<Yb::String> &split_str(const Yb::String &s,
        const Yb::String &delim, vector<Yb::String> &parts)
{
    int start = 0;
    while (1) {
        int pos = str_find(s, delim, start);
        if (-1 == pos) {
            parts.push_back(str_substr(s, start));
            break;
        }
        parts.push_back(str_substr(s, start, pos - start));
        start = pos + str_length(delim);
    }
    return parts;
}

YBUTIL_DECL void split_str_by_chars(const Yb::String &s, const Yb::String &delim,
        vector<Yb::String> &parts, int limit)
{
    const size_t sz0 = parts.size();
    Yb::String p;
    for (size_t i = 0; i < str_length(s); ++i) {
        if (str_find(delim, s[i]) != -1) {
            if (limit > 0 && !str_empty(p) &&
                    parts.size() - sz0 >= (size_t)(limit - 1))
                str_append(p, s[i]);
            else if (!str_empty(p)) {
                parts.push_back(p);
                p = _T("");
            }
        }
        else
            str_append(p, s[i]);
    }
    if (!str_empty(p))
        parts.push_back(p);
}

YBUTIL_DECL const Yb::String join_str(const Yb::String &delim,
        const vector<Yb::String> &parts)
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

YBUTIL_DECL int hex_digit(Char ch)
{
    int c = char_code(ch);
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    throw ValueError(_T("invalid hex digit"));
}

YBUTIL_DECL const string url_decode(const String &s)
{
    string result;
    for (size_t i = 0; i < str_length(s); ++i) {
        if (char_code(s[i]) == '+')
            result.push_back(' ');
        else if (char_code(s[i]) != '%')
            result.push_back((unsigned char)char_code(s[i]));
        else {
            ++i;
            if (str_length(s) - i < 2)
                throw ValueError(_T("url_decode: short read"));
            unsigned char c = hex_digit(s[i]) * 16 +
                hex_digit(s[i + 1]);
            ++i;
            result.push_back(c);
        }
    }
    return result;
}

YBUTIL_DECL const String url_encode(const string &s, bool path_mode)
{
    String result;
    const char *replace;
    if (path_mode)
        replace = "!*'();@&=+$,?%#[]";
    else
        replace = "!*'();:@&=+$,/?%#[]";
    char buf[20];
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = s[i];
        if (c <= 32 || c >= 127 || strchr(replace, c)) {
            sprintf(buf, "%%%02X", c);
            result += WIDEN(buf);
        }
        else
            result += Char(c);
    }
    return result;
}

YBUTIL_DECL void parse_url_proto(const String &url,
        String &proto, String &proto_ext, String &url_tail)
{
    Strings url_parts, proto_parts;
    split_str(url, _T("://"), url_parts);
    if (url_parts.size() != 2)
        throw ValueError(_T("url parse error"));
    url_tail = url_parts[1];
    split_str(url_parts[0], _T("+"), proto_parts);
    if (proto_parts.size() == 1) {
        proto = proto_parts[0];
        proto_ext = String();
    }
    else if (proto_parts.size() == 2) {
        proto = proto_parts[0];
        proto_ext = proto_parts[1];
    }
    else
        throw ValueError(_T("url parse error"));
}

YBUTIL_DECL void parse_url_tail(const String &url_tail, StringDict &params)
{
    String host_etc;
    Strings user_host_parts;
    split_str(url_tail, _T("@"), user_host_parts);
    if (user_host_parts.size() == 1) {
        host_etc = user_host_parts[0];
    }
    else if (user_host_parts.size() == 2) {
        Strings user_parts;
        split_str(user_host_parts[0], _T(":"), user_parts);
        if (user_parts.size() == 1) {
            params.set(_T("&user"), user_parts[0]);
        }
        else if (user_parts.size() == 2) {
            params.set(_T("&user"), user_parts[0]);
            params.set(_T("&passwd"), user_parts[1]);
        }
        else
            throw ValueError(_T("url parse error"));
        host_etc = user_host_parts[1];
    }
    else
        throw ValueError(_T("url parse error"));

    Strings anchor_parts;
    split_str(host_etc, _T("#"), anchor_parts);
    if (anchor_parts.size() == 1) {
        host_etc = anchor_parts[0];
    }
    else if (anchor_parts.size() == 2) {
        host_etc = anchor_parts[0];
        params.set(_T("&anchor"), WIDEN(url_decode(anchor_parts[1])));
    }
    else
        throw ValueError(_T("url parse error"));

    Strings query_parts, items;
    split_str(host_etc, _T("?"), query_parts);
    if (query_parts.size() == 1) {
        host_etc = query_parts[0];
    }
    else if (query_parts.size() == 2) {
        host_etc = query_parts[0];
        split_str_by_chars(query_parts[1], _T("&"), items);
        Strings::const_iterator it = items.begin(), end = items.end();
        for (; it != end; ++it) {
            Strings kv;
            split_str_by_chars(*it, _T("="), kv, 2);
            if (kv.size() > 0) {
                String key = WIDEN(url_decode(kv[0])), value;
                if (kv.size() > 1)
                    value = WIDEN(url_decode(kv[1]));
                params.set(key, value);
            }
        }
    }
    else
        throw ValueError(_T("url parse error"));

    if (look_like_absolute_path(host_etc)) {
        params.set(_T("&path"), WIDEN(url_decode(host_etc)));
    }
    else {
        int pos = str_find(host_etc, _T("/"));
        if (pos >= 0) {
            params.set(_T("&path"),
                    WIDEN(url_decode(str_substr(host_etc, pos))));
            host_etc = str_substr(host_etc, 0, pos);
        }
        pos = str_find(host_etc, _T(":"));
        if (pos >= 0) {
            params.set(_T("&port"), str_substr(host_etc, pos + 1));
            host_etc = str_substr(host_etc, 0, pos);
        }
        params.set(_T("&host"), host_etc);
    }
}

YBUTIL_DECL const StringDict parse_url(const String &url)
{
    String proto, proto_ext, url_tail;
    parse_url_proto(url, proto, proto_ext, url_tail);
    StringDict params;
    parse_url_tail(url_tail, params);
    params.set(_T("&proto"), proto);
    if (!str_empty(proto_ext))
        params.set(_T("&proto_ext"), proto_ext);
    return params;
}

YBUTIL_DECL const String format_url(const StringDict &params, bool hide_passwd)
{
    String r = params[_T("&proto")];
    if (!params.empty_key(_T("&proto_ext")))
        r += _T("+") + params[_T("&proto_ext")];
    r += _T("://");
    if (!params.empty_key(_T("&user"))) {
        r += params[_T("&user")];
        if (!hide_passwd && !params.empty_key(_T("&passwd")))
            r += _T(":") + params[_T("&passwd")];
        r += _T("@");
    }
    if (!params.empty_key(_T("&host"))) {
        r += params[_T("&host")];
        if (!params.empty_key(_T("&port")))
            r += _T(":") + params[_T("&port")];
    }
    if (!params.empty_key(_T("&path")))
        r += url_encode(NARROW(params[_T("&path")]), true);
    String q;
    Strings keys = params.keys();
    for (size_t i = 0; i < keys.size(); ++i)
        if (!starts_with(keys[i], _T("&"))) {
            if (!str_empty(q))
                q += _T("&");
            q += url_encode(NARROW(keys[i]))
                + _T("=") + url_encode(NARROW(params[keys[i]]));
        }
    if (!str_empty(q))
        r += _T("?") + q;
    if (!params.empty_key(_T("&anchor")))
        r += _T("#") + url_encode(NARROW(params[_T("&anchor")]));
    return r;
}

YBUTIL_DECL const Yb::String quote(const Yb::String &s)
{
    return _T("'") + s + _T("'");
}

YBUTIL_DECL const Yb::String dquote(const Yb::String &s)
{
    return _T("\"") + s + _T("\"");
}

YBUTIL_DECL const Yb::String brackets(const Yb::String &s)
{
    return _T("(") + s + _T(")");
}

YBUTIL_DECL const Yb::String comma(const Yb::String &item1, const Yb::String &item2)
{
    return item1 + _T(", ") + item2;
}

YBUTIL_DECL const Yb::String xgetenv(const Yb::String &var_name)
{
    char *x = NULL;
    Yb::String s;
#if defined(_MSC_VER)
    size_t len;
    errno_t err = _dupenv_s(&x, &len, NARROW(var_name).c_str());
    if (!err && x) {
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

YBUTIL_DECL const Yb::String dict2str(const Yb::StringDict &params) {
    ostringstream out;
    out << "{";
    StringDict::const_iterator it = params.begin(), end = params.end();
    for (bool first = true; it != end; ++it, first = false) {
        if (!first)
            out << ", ";
        out << NARROW(it->first) << ": "
            << NARROW(dquote(c_string_escape(it->second)));
    }
    out << "}";
    return WIDEN(out.str());
}

} // end of namespace StrUtils

} // end of namespace Yb

// vim:ts=4:sts=4:sw=4:et:
