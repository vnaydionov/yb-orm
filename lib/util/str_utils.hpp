
#ifndef YB__STR_UTILS_HPP__INCLUDED
#define YB__STR_UTILS_HPP__INCLUDED

#include <string>
#include <vector>
#include <util/DataTypes.h>

namespace Yb {

namespace StrUtils {

// Generic helpers

// NOTE: only '[a-zA-Z]' symbols are treated as alphabetical.

bool is_lower(Yb::Char c);

bool is_upper(Yb::Char c);

bool is_alpha(Yb::Char c);

bool is_digit(Yb::Char c);

bool is_space(Yb::Char c);

// NOTE: an ID is '[_a-zA-Z][_a-zA-Z0-9]*'

bool is_start_symbol_of_id(Yb::Char c);

bool is_symbol_of_id(Yb::Char c);

bool is_id(const Yb::String &s);

bool look_like_absolute_path(const String &s);

bool starts_with(const Yb::String &s, const Yb::String &subs);

bool ends_with(const Yb::String &s, const Yb::String &subs);

const Yb::String substr_after(const Yb::String &s, const Yb::String &subs);

Yb::Char to_lower(Yb::Char c);

Yb::Char to_upper(Yb::Char c);

const Yb::String translate(const Yb::String &s, Yb::Char (*f)(Yb::Char));

const Yb::String str_to_lower(const Yb::String &s);

const Yb::String str_to_upper(const Yb::String &s);

const Yb::String trim_trailing_space(const Yb::String &s);

const Yb::String c_string_escape(const Yb::String &s);

const Yb::String sql_string_escape(const Yb::String &s);

const Yb::String html_escape(const Yb::String &s);

void split_path(const Yb::String &path, std::vector<Yb::String> &items);

std::vector<Yb::String> &split_str(const Yb::String &s,
        const Yb::String &delim,
        std::vector<Yb::String> &parts);

void split_str_by_chars(const Yb::String &s, const Yb::String &delim,
        std::vector<Yb::String> &parts, int limit = -1);

const Yb::String join_str(const Yb::String &delim,
        const std::vector<Yb::String> &parts);

int hex_digit(Yb::Char ch);

const std::string url_decode(const Yb::String &s);

const Yb::String url_encode(const std::string &s, bool path_mode = false);

void parse_url_proto(const String &url,
        String &proto, String &proto_ext, String &url_tail);

void parse_url_tail(const String &url_tail, StringDict &params);

const StringDict parse_url(const String &url);

const String format_url(const StringDict &params, bool hide_passwd = true);

// Code generation helpers

const Yb::String quote(const Yb::String &s);

const Yb::String dquote(const Yb::String &s);

const Yb::String brackets(const Yb::String &s);

const Yb::String comma(const Yb::String &item1, const Yb::String &item2);

// Misc

const Yb::String xgetenv(const Yb::String &var_name);

} // end of namespace StrUtils

} // end of namespace Yb

// vim:ts=4:sts=4:sw=4:et

#endif // YB__STR_UTILS_HPP__INCLUDED

