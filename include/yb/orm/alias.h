// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__ORM__ALIAS__INCLUDED
#define YB__ORM__ALIAS__INCLUDED

#include <string>
#include <vector>
#include <set>
#include <map>
#include <string.h>
#include "orm_config.h"

namespace Yb {

typedef std::vector<std::string> string_vector;
typedef std::vector<int> int_vector;
typedef std::vector<std::pair<std::string, std::string> > strpair_vector;
typedef std::set<std::string> string_set;
typedef std::map<std::string, std::string> string_map;
typedef std::map<std::string, int_vector> str2intvec_map;
typedef std::map<std::string, string_vector> str2strvec_map;

inline char lower(char c) {
    if ('A' <= c && c <= 'Z')
        return c + ('a' - 'A');
    return c;
}

YBORM_DECL const std::string mk_alias(const std::string &tbl,
                           const std::string &col,
                           size_t max_len,
                           size_t cnt);

YBORM_DECL bool is_camel(const std::string &s);

YBORM_DECL void split_words(const std::string &s, string_vector &out);

inline bool is_vowel(char c) {
    return strchr("aouei", c) != NULL;
}

inline bool skip_letter(char p, char c, char n) {
    return ((is_vowel(c) && p)
                || (c == 'y' && n && !is_vowel(n) && p && !is_vowel(p))
                || (c == 'y' && !n && p && !is_vowel(p))
                || (c == 'h' && n && !is_vowel(n))
                || (c == 'h' && p && !is_vowel(p)));
}

YBORM_DECL const std::string shorten(const std::string &s);

YBORM_DECL void get_conflicts(const string_map &aliases, string_set &out);

YBORM_DECL void mk_table_aliases(const str2strvec_map &tbl_words,
        const str2intvec_map &word_pos, string_map &out);

YBORM_DECL void fallback_table_aliases(const string_set &confl_tbls,
        const str2strvec_map &tbl_words,
        const str2intvec_map &word_pos, string_map &out);

YBORM_DECL void inc_word_pos(const string_set &confl_tbls,
        const str2strvec_map &tbl_words,
        str2intvec_map &word_pos);

YBORM_DECL void table_aliases(const string_set &tbls_set, string_map &out);

YBORM_DECL void col_aliases(const strpair_vector &p, size_t max_len, 
        string_vector &out);

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__ALIAS__INCLUDED
