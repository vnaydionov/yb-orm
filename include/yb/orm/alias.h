#ifndef YB__ALIAS_H__INCLUDED
#define YB__ALIAS_H__INCLUDED

#include <string>
#include <vector>
#include <set>
#include <map>
#include <string.h>

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

const std::string mk_alias(const std::string &tbl,
                           const std::string &col,
                           size_t max_len,
                           size_t cnt);

bool is_camel(const std::string &s);

void split_words(const std::string &s, string_vector &out);

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

const std::string shorten(const std::string &s);

void get_conflicts(const string_map &aliases, string_set &out);

void mk_table_aliases(const str2strvec_map &tbl_words,
        const str2intvec_map &word_pos, string_map &out);

void fallback_table_aliases(const string_set &confl_tbls,
        const str2strvec_map &tbl_words,
        const str2intvec_map &word_pos, string_map &out);

void inc_word_pos(const string_set &confl_tbls,
        const str2strvec_map &tbl_words,
        str2intvec_map &word_pos);

void table_aliases(const string_set &tbls_set, string_map &out);

void col_aliases(const strpair_vector &p, size_t max_len, 
        string_vector &out);

} // end of namespace Yb

#endif // YB__ALIAS_H__INCLUDED
