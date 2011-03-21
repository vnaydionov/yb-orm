
#ifndef YB__STR_UTILS_HPP__INCLUDED
#define YB__STR_UTILS_HPP__INCLUDED

#include <string>
#include <vector>

namespace Yb {

namespace StrUtils {

// Generic helpers

// NOTE: only '[a-zA-Z]' symbols are treated as alphabetical.

bool is_lower(char c);

bool is_upper(char c);

bool is_alpha(char c);

bool is_digit(char c);

bool is_space(char c);

// NOTE: an ID is '[_a-zA-Z][_a-zA-Z0-9]*'

bool is_start_symbol_of_id(char c);

bool is_symbol_of_id(char c);

bool is_id(const std::string &s);

bool starts_with(const std::string &s, const std::string &subs);

bool ends_with(const std::string &s, const std::string &subs);

const std::string substr_after(const std::string &s, const std::string &subs);

char to_lower(char c);

char to_upper(char c);

const std::string translate(const std::string &s, char (*f)(char));

const std::string str_to_lower(const std::string &s);

const std::string str_to_upper(const std::string &s);

const std::string trim_trailing_space(const std::string &s);

const std::string sql_string_escape(const std::string &s);

const std::string html_escape(const std::string &s);

void split_path(const std::string &path, std::vector<std::string> &items);

// Code generation helpers

const std::string quote(const std::string &s);

const std::string dquote(const std::string &s);

const std::string brackets(const std::string &s);

const std::string comma(const std::string &item1, const std::string &item2);

// Misc

const std::string xgetenv(const std::string &var_name);

} // end of namespace StrUtils

} // end of namespace Yb

// vim:ts=4:sts=4:sw=4:et

#endif // YB__STR_UTILS_HPP__INCLUDED

