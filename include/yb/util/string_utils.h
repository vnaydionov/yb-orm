// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__UTIL__STRING_UTILS__INCLUDED
#define YB__UTIL__STRING_UTILS__INCLUDED

#include <string>
#include <vector>
#include "util_config.h"
#include "data_types.h"

namespace Yb {

namespace StrUtils {

// Generic helpers

// NOTE: only '[a-zA-Z]' symbols are treated as alphabetical.

YBUTIL_DECL bool is_lower(Yb::Char c);

YBUTIL_DECL bool is_upper(Yb::Char c);

YBUTIL_DECL bool is_alpha(Yb::Char c);

YBUTIL_DECL bool is_digit(Yb::Char c);

YBUTIL_DECL bool is_space(Yb::Char c);

// NOTE: an ID is '[_a-zA-Z][_a-zA-Z0-9]*'

YBUTIL_DECL bool is_start_symbol_of_id(Yb::Char c);

YBUTIL_DECL bool is_symbol_of_id(Yb::Char c);

YBUTIL_DECL bool is_id(const Yb::String &s);

YBUTIL_DECL bool is_sql_id(const Yb::String &s);

YBUTIL_DECL bool look_like_absolute_path(const String &s);

YBUTIL_DECL bool starts_with(const Yb::String &s, const Yb::String &subs);

YBUTIL_DECL bool ends_with(const Yb::String &s, const Yb::String &subs);

YBUTIL_DECL const Yb::String substr_after(const Yb::String &s, const Yb::String &subs);

YBUTIL_DECL Yb::Char to_lower(Yb::Char c);

YBUTIL_DECL Yb::Char to_upper(Yb::Char c);

YBUTIL_DECL const Yb::String translate(const Yb::String &s, Yb::Char (*f)(Yb::Char));

YBUTIL_DECL const Yb::String str_to_lower(const Yb::String &s);

YBUTIL_DECL const Yb::String str_to_upper(const Yb::String &s);

YBUTIL_DECL const Yb::String trim_trailing_space(const Yb::String &s);

YBUTIL_DECL const Yb::String c_string_escape(const Yb::String &s);

YBUTIL_DECL const Yb::String sql_string_escape(const Yb::String &s);

YBUTIL_DECL const Yb::String html_escape(const Yb::String &s);

YBUTIL_DECL void split_path(const Yb::String &path, std::vector<Yb::String> &items);

YBUTIL_DECL std::vector<Yb::String> &split_str(const Yb::String &s,
        const Yb::String &delim,
        std::vector<Yb::String> &parts);

YBUTIL_DECL void split_str_by_chars(const Yb::String &s, const Yb::String &delim,
        std::vector<Yb::String> &parts, int limit = -1);

YBUTIL_DECL const Yb::String join_str(const Yb::String &delim,
        const std::vector<Yb::String> &parts);

YBUTIL_DECL int hex_digit(Yb::Char ch);

YBUTIL_DECL const std::string url_decode(const Yb::String &s);

YBUTIL_DECL const Yb::String url_encode(const std::string &s, bool path_mode = false);

YBUTIL_DECL void parse_url_proto(const String &url,
        String &proto, String &proto_ext, String &url_tail);

YBUTIL_DECL void parse_url_tail(const String &url_tail, StringDict &params);

YBUTIL_DECL const StringDict parse_url(const String &url);

YBUTIL_DECL const String format_url(const StringDict &params, bool hide_passwd = true);

// Code generation helpers

YBUTIL_DECL const Yb::String quote(const Yb::String &s);

YBUTIL_DECL const Yb::String dquote(const Yb::String &s);

YBUTIL_DECL const Yb::String brackets(const Yb::String &s);

YBUTIL_DECL const Yb::String comma(const Yb::String &item1, const Yb::String &item2);

// Misc

YBUTIL_DECL const Yb::String xgetenv(const Yb::String &var_name);

YBUTIL_DECL const Yb::String dict2str(const Yb::StringDict &params);

} // end of namespace StrUtils

} // end of namespace Yb

#endif // YB__UTIL__STRING_UTILS__INCLUDED
// vim:ts=4:sts=4:sw=4:et:
