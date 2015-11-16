#include "orm/alias.h"
#include <algorithm>
#include <stdexcept>
#include <stdio.h>

namespace Yb {

const std::string mk_alias(const std::string &tbl,
                           const std::string &col,
                           size_t max_len,
                           size_t cnt)
{
    std::string r;
    r.reserve(tbl.size() + 1 + col.size());
    r.append(tbl);
    r.append(1, '_');
    r.append(col);
    if (r.size() > max_len) {
        char buf[25];
        sprintf(buf, "%u", cnt);
        size_t cnt_len = strlen(buf);
        r[max_len - cnt_len - 1] = '_';
        strcpy(&r[max_len - cnt_len], buf);
        r.resize(max_len);
    }
    return r;
}

bool is_camel(const std::string &s) {
    std::string::const_iterator i = s.begin(), end = s.end();
    bool small_seen = false, cap_seen = false;
    for (; i != end; ++i) {
        if ('a' <= *i && *i <= 'z') {
            if (cap_seen)
                return true;
            small_seen = true;
        }
        else if ('A' <= *i && *i <= 'Z') {
            if (small_seen)
                return true;
            cap_seen = true;
        }
    }
    return false;
}

void split_words(const std::string &s, string_vector &out) {
    std::string w;
    std::string::const_iterator i = s.begin(), end = s.end();
    for (; i != end; ++i) {
        if ('_' == *i || '-' == *i || ' ' == *i || '$' == *i) {
            if (w.size() > 1)
                out.push_back(w);
            w.clear();
        }
        else if ('A' <= *i && *i <= 'Z') {
            if (w.size() > 1)
                out.push_back(w);
            w.clear();
            w.push_back(lower(*i));
        }
        else {
            w.push_back(*i);
        }
    }
    if (w.size() > 1)
        out.push_back(w);
}

const std::string shorten(const std::string &s) {
    std::string r;
    r.reserve(s.size());
    std::string::const_iterator i = s.begin(), end = s.end();
    char p = 0, c = lower(*i);
    for (; i != end; ++i) {
        char n = lower(*(i + 1));
        if (!skip_letter(p, c, n))
            r.push_back(c);
        p = c;
        c = n;
    }
    return r;
}

void get_conflicts(const string_map &aliases, string_set &out) {
    string_map values;
    string_map::const_iterator i = aliases.begin(), end = aliases.end();
    for (; i != end; ++i) {
        string_map::iterator j = values.find(i->second);
        if (j != values.end()) {
            out.insert(j->second);
            out.insert(i->first);
        }
        else {
            values[i->second] = i->first;
        }
    }
}

void mk_table_aliases(const str2strvec_map &tbl_words,
        const str2intvec_map &word_pos, string_map &out) {
    str2strvec_map::const_iterator i = tbl_words.begin(),
        end = tbl_words.end();
    for (; i != end; ++i) {
        str2intvec_map::const_iterator p = word_pos.find(i->first);
        if (p == word_pos.end())
            throw std::runtime_error("inconsistent word_pos (2)");
        std::string alias;
        for (size_t j = 0; j < i->second.size(); ++j) {
            alias.append(i->second[j].substr(
                        0, p->second[j]));
        }
        out[i->first] = alias;
    }
}

void fallback_table_aliases(const string_set &confl_tbls,
        const str2strvec_map &tbl_words,
        const str2intvec_map &word_pos, string_map &out) {
    mk_table_aliases(tbl_words, word_pos, out);
    string_set::const_iterator it = confl_tbls.begin(), end = confl_tbls.end();
    for (size_t i = 0; it != end; ++it, ++i) {
        char buf[25];
        sprintf(buf, "%u", i + 1);
        out[*it] += std::string(buf);
    }
}

void inc_word_pos(const string_set &confl_tbls,
        const str2strvec_map &tbl_words,
        str2intvec_map &word_pos) {
    string_set::const_iterator it = confl_tbls.begin(), end = confl_tbls.end();
    for (; it != end; ++it) {
        const std::string &tbl = *it;
        str2strvec_map::const_iterator twords = tbl_words.find(tbl);
        if (twords == tbl_words.end())
            throw std::runtime_error("inconsistent tbl_words");
        str2intvec_map::iterator wpos = word_pos.find(tbl);
        if (wpos == word_pos.end())
            throw std::runtime_error("inconsistent word_pos");
        if (wpos->second.size() != twords->second.size())
            throw std::runtime_error("inconsistent wpos and twords.");
        for (size_t j = 0; j < wpos->second.size(); ++j) {
            if (wpos->second[j] < twords->second[j].size()) {
                wpos->second[j] += 1;
                break;
            }
        }
    }
}

void table_aliases(const string_set &tbls_set, string_map &out)
{
    str2strvec_map tbl_words;
    str2intvec_map word_pos;
    string_set::const_iterator i = tbls_set.begin(), iend = tbls_set.end();
    for (; i != iend; ++i) {
        string_vector words;
        split_words(*i, words);
        string_vector::iterator j = words.begin(), jend = words.end();
        for (; j != jend; ++j)
            *j = shorten(*j);
        int_vector wpos(words.size(), 1);
        tbl_words[*i] = words;
        word_pos[*i] = wpos;
    }
    string_set confl_tbls;
    for (int c = 0; c < 5; ++c) {
        if (c)
            inc_word_pos(confl_tbls, tbl_words, word_pos);
        string_map aliases;
        mk_table_aliases(tbl_words, word_pos, aliases);
        string_set confl_tbls_new;
        get_conflicts(aliases, confl_tbls_new);
        if (!confl_tbls_new.size()) {
            out.swap(aliases);
            return;
        }
        confl_tbls.swap(confl_tbls_new);
    }
    string_map aliases;
    fallback_table_aliases(confl_tbls, tbl_words, word_pos, aliases);
    out.swap(aliases);
}

void col_aliases(const strpair_vector &p, size_t max_len, 
        string_vector &out) {
    string_set tbls;
    strpair_vector::const_iterator i = p.begin(), iend = p.end();
    for (; i != iend; ++i)
        tbls.insert(i->first);
    string_map t_aliases;
    table_aliases(tbls, t_aliases);
    i = p.begin(), iend = p.end();
    for (size_t c = 0; i != iend; ++i, ++c) {
        out.push_back(mk_alias(t_aliases[i->first],
                    i->second, max_len, c + 1));
    }
}

} // end of namespace Yb
