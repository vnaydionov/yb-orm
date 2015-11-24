// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#define YBORM_SOURCE

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "orm/alias.h"
#include <algorithm>
#include <stdexcept>
#include <memory>
#include <stdio.h>

namespace Yb {

YBORM_DECL const std::string mk_alias(const std::string &tbl,
                           const std::string &col,
                           size_t max_len,
                           size_t cnt)
{
    std::string r;
    r.reserve(tbl.size() + 1 + col.size());
    r.append(tbl);
    r.append(1, '_');
    r.append(col);
    for (std::string::iterator i = r.begin(), end = r.end();
            i != end; ++i)
    {
        *i = lower(*i);
    }
    if (r.size() > max_len) {
        char buf[25];
        sprintf(buf, "%u", (unsigned )cnt);
        size_t cnt_len = strlen(buf);
        r[max_len - cnt_len - 1] = '_';
        strcpy(&r[max_len - cnt_len], buf);
        r.resize(max_len);
    }
    return r;
}

YBORM_DECL bool is_camel(const std::string &s) {
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

YBORM_DECL void split_words(const std::string &s, string_vector &out) {
    bool camel = is_camel(s);
    std::string w;
    std::string::const_iterator i = s.begin(), end = s.end();
    for (; i != end; ++i) {
        if ('_' == *i || '-' == *i || ' ' == *i || '$' == *i) {
            if (w.size() > 0)
                out.push_back(w);
            w.clear();
        }
        else if (camel && 'A' <= *i && *i <= 'Z') {
            if (w.size() > 0)
                out.push_back(w);
            w.clear();
            w.push_back(lower(*i));
        }
        else {
            w.push_back(lower(*i));
        }
    }
    if (w.size() > 0)
        out.push_back(w);
    if (out.size() > 1) {
        for (size_t j = 0; j < out.size();) {
            if (out[j].size() == 1)
                out.erase(out.begin() + j);
            else
                ++j;
        }
    }
}

YBORM_DECL const std::string shorten(const std::string &s) {
    std::string r;
    r.reserve(s.size());
    const char *i = s.data(), *end = i + s.size();
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

static std::auto_ptr<string_set> sql_kwords;

static void init_sql_kwords() {
    if (!sql_kwords.get()) {
        std::auto_ptr<string_set> kw(new string_set);
        kw->insert("add");
        kw->insert("all");
        kw->insert("alter");
        kw->insert("and");
        kw->insert("as");
        kw->insert("asc");
        kw->insert("auto_increment");
        kw->insert("avg");
        kw->insert("bigint");
        kw->insert("binary");
        kw->insert("blob");
        kw->insert("bool");
        kw->insert("boolean");
        kw->insert("by");
        kw->insert("byte");
        kw->insert("char");
        kw->insert("check");
        kw->insert("clob");
        kw->insert("column");
        kw->insert("constraint");
        kw->insert("count");
        kw->insert("create");
        kw->insert("cursor");
        kw->insert("date");
        kw->insert("datetime");
        kw->insert("decimal");
        kw->insert("default");
        kw->insert("delete");
        kw->insert("desc");
        kw->insert("disable");
        kw->insert("distinct");
        kw->insert("double");
        kw->insert("drop");
        kw->insert("enable");
        kw->insert("end");
        kw->insert("exists");
        kw->insert("explain");
        kw->insert("float");
        kw->insert("for");
        kw->insert("foreign");
        kw->insert("from");
        kw->insert("full");
        kw->insert("function");
        kw->insert("go");
        kw->insert("grant");
        kw->insert("group");
        kw->insert("having");
        kw->insert("in");
        kw->insert("index");
        kw->insert("inner");
        kw->insert("insert");
        kw->insert("int");
        kw->insert("integer");
        kw->insert("into");
        kw->insert("join");
        kw->insert("key");
        kw->insert("left");
        kw->insert("like");
        kw->insert("limit");
        kw->insert("lob");
        kw->insert("long");
        kw->insert("loop");
        kw->insert("max");
        kw->insert("min");
        kw->insert("modify");
        kw->insert("not");
        kw->insert("null");
        kw->insert("number");
        kw->insert("numeric");
        kw->insert("nvarchar");
        kw->insert("nvarchar2");
        kw->insert("of");
        kw->insert("off");
        kw->insert("offset");
        kw->insert("on");
        kw->insert("or");
        kw->insert("order");
        kw->insert("outer");
        kw->insert("over");
        kw->insert("plan");
        kw->insert("precision");
        kw->insert("primary");
        kw->insert("procedure");
        kw->insert("raw");
        kw->insert("references");
        kw->insert("revoke");
        kw->insert("round");
        kw->insert("row");
        kw->insert("select");
        kw->insert("sequence");
        kw->insert("set");
        kw->insert("show");
        kw->insert("signed");
        kw->insert("smallint");
        kw->insert("sum");
        kw->insert("table");
        kw->insert("time");
        kw->insert("timestamp");
        kw->insert("tinyint");
        kw->insert("to");
        kw->insert("trigger");
        kw->insert("unique");
        kw->insert("unsigned");
        kw->insert("update");
        kw->insert("using");
        kw->insert("values");
        kw->insert("varbinary");
        kw->insert("varchar");
        kw->insert("varchar2");
        kw->insert("view");
        kw->insert("where");
        if (!sql_kwords.get())
            sql_kwords.reset(kw.release());
    }
}

YBORM_DECL void get_conflicts(const string_map &aliases, string_set &out) {
    init_sql_kwords();
    string_map values;
    string_map::const_iterator i = aliases.begin(), end = aliases.end();
    for (; i != end; ++i) {
        if (sql_kwords->find(i->second) != sql_kwords->end()) {
            out.insert(i->first);
        }
        else {
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
}

YBORM_DECL void mk_table_aliases(const str2strvec_map &tbl_words,
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

YBORM_DECL void fallback_table_aliases(const string_set &confl_tbls,
        const str2strvec_map &tbl_words,
        const str2intvec_map &word_pos, string_map &out) {
    mk_table_aliases(tbl_words, word_pos, out);
    string_set::const_iterator it = confl_tbls.begin(), end = confl_tbls.end();
    for (size_t i = 0; it != end; ++it, ++i) {
        char buf[25];
        sprintf(buf, "%u", (unsigned )(i + 1));
        out[*it] += std::string(buf);
    }
}

YBORM_DECL void inc_word_pos(const string_set &confl_tbls,
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
            if (wpos->second[j] < (int)twords->second[j].size()) {
                wpos->second[j] += 1;
                break;
            }
        }
    }
}

YBORM_DECL void table_aliases(const string_set &tbls_set, string_map &out)
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

YBORM_DECL void col_aliases(const strpair_vector &p, size_t max_len, 
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
// vim:ts=4:sts=4:sw=4:et:
