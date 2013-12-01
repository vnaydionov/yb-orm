// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__UTIL__STRING__INCLUDED
#define YB__UTIL__STRING__INCLUDED

#include <cstring>
#include <string>
#if defined(YB_USE_WX)
#include <wx/string.h>
#elif defined(YB_USE_QT)
#include <QString>
#endif

namespace Yb {

template <class C>
struct CharTraits {
    inline static int char_code(C c) { return c; }
};

template <>
struct CharTraits<char> {
    inline static int char_code(char c) { return (unsigned char )c; }
};

template <class C, class Traits = CharTraits<C> >
struct CharBuf {
    typedef Traits CharBufTraits;

    static int x_strlen(const C *s)
    {
        const C *s0 = s;
        for (; Traits::char_code(*s); ++s);
        return s - s0;
    }

    template <class S>
    static C *x_strcpy(C *d, const S *s)
    {
        C *d0 = d;
        while (1) {
            int x = CharBuf<S>::CharBufTraits::char_code(*s++);
            if (!x)
                break;
            *d++ = C(x);
        }
        *d = C(0);
        return d0;
    }

    mutable size_t len;
    mutable C *data;

    explicit CharBuf(): len(0), data(NULL) {}

    explicit CharBuf(int buf_len): len(buf_len), data(new C[buf_len]) {
        std::memset(data, 0, buf_len * sizeof(C));
    }

    template <class S>
    explicit CharBuf(const S *s)
        : len(CharBuf<S>::x_strlen(s) + 1)
        , data(new C[len])
    {
        x_strcpy(data, s);
    }

    CharBuf(const CharBuf &x) {
        data = x.data;
        len = x.len;
        x.data = NULL;
        x.len = 0;
    }

    CharBuf &operator=(const CharBuf &x) {
        if (this != &x) {
            delete[] data;
            data = x.data;
            len = x.len;
            x.data = NULL;
            x.len = 0;
        }
        return *this;
    }

    ~CharBuf() { delete[] data; }
};

#define YB_STRING_NOSTD
#if defined(YB_USE_WX)
#if wxUSE_UNICODE
#define YB_USE_UNICODE
#define NARROW(x) Yb::str2std(x)
#define WIDEN(x) Yb::std2str(x)
#else
#undef YB_USE_UNICODE
#define NARROW(x) x
#define WIDEN(x) x
#endif
typedef wxChar Char;
typedef wxString String;
inline int char_code(Char c) { return c; }
inline const String str_n(int n, Char c) { return wxString(c, (size_t)n); }
inline const String str_from_chars(const Char *s) { return wxString(s); }
inline size_t str_length(const String &s) { return s.Len(); }
inline bool str_empty(const String &s) { return s.IsEmpty(); }
inline const Char *str_data(const String &s) { return s.GetData(); }
inline int str_find(const String &s, const String &sub, int start = 0)
{
    size_t pos = s.find(sub, start);
    return wxString::npos == pos? -1: pos;
}
inline int str_find(const String &s, Char c, int start = 0)
{
    size_t pos = s.find(c, start);
    return wxString::npos == pos? -1: pos;
}
inline const String str_substr(const String &s, int start, int count = -1)
{
    return s.Mid(start, count == -1? wxSTRING_MAXLEN: (size_t)count);
}
#elif defined(YB_USE_QT)
#define YB_USE_UNICODE
#define NARROW(x) Yb::str2std(x)
#define WIDEN(x) Yb::std2str(x)
#undef _T
#define _T(x) x
typedef QChar Char;
typedef QString String;
template <>
struct CharTraits<QChar> {
    inline static int char_code(QChar c) { return c.unicode(); }
};
inline int char_code(Char c) { return c.unicode(); }
inline const String str_n(int n, Char c) { return String(n, c); }
inline const String str_from_chars(const Char *s)
{
    return QString(s, CharBuf<QChar>::x_strlen(s));
}
inline size_t str_length(const String &s) { return s.length(); }
inline bool str_empty(const String &s) { return s.isEmpty(); }
inline const Char *str_data(const String &s) { return s.data(); }
inline int str_find(const String &s, const String &sub, int start = 0)
{
    return s.indexOf(sub, start);
}
inline int str_find(const String &s, Char c, int start = 0)
{
    return s.indexOf(c, start);
}
inline const String str_substr(const String &s, int start, int count = -1)
{
    return s.mid(start, count);
}
#else
#undef _T
#if defined(YB_USE_UNICODE)
#define NARROW(x) Yb::str2std(x)
#define WIDEN(x) Yb::std2str(x)
#define _T(x) L ## x
typedef wchar_t Char;
typedef std::wstring String;
#else
#define NARROW(x) x
#define WIDEN(x) x
#define _T(x) x
#undef YB_STRING_NOSTD
typedef char Char;
typedef std::string String;
#endif
inline int char_code(Char c) { return (unsigned char)c; }
inline const String str_n(int n, Char c) { return String((size_t)n, c); }
inline const String str_from_chars(const Char *s) { return String(s); }
inline size_t str_length(const String &s) { return s.size(); }
inline bool str_empty(const String &s) { return s.empty(); }
inline const Char *str_data(const String &s) { return s.c_str(); }
inline int str_find(const String &s, const String &sub, int start = 0)
{
    size_t pos = s.find(sub, start);
    return String::npos == pos? -1: pos;
}
inline int str_find(const String &s, Char c, int start = 0)
{
    size_t pos = s.find(c, start);
    return String::npos == pos? -1: pos;
}
inline const String str_substr(const String &s, int start, int count = -1)
{
    return s.substr(start, count == -1? String::npos: (size_t)count);
}
#endif
inline String &str_append(String &s, const String &t) { return (s += t); }
inline String &str_append(String &s, Char c) { return (s += c); }

const std::string str2std(const String &s, const std::string &enc_name = "");
const String std2str(const std::string &s, const std::string &enc_name = "");
const std::string str_narrow(const std::wstring &wide,
        const std::string &enc_name = "");
const std::wstring str_widen(const std::string &narrow,
        const std::string &enc_name = "");
const std::string get_locale_enc();
const std::string fast_narrow(const std::wstring &wide);
const std::wstring fast_widen(const std::string &narrow);

template <class T>
const std::string to_stdstring(const T &x);
template <class T>
const String to_string(const T &x);

template <class T>
T &from_stdstring(const std::string &s, T &x);
template <class T>
T &from_string(const String &s, T &x);

inline void str_swap(String &a, String &b) {
    using namespace std;
    swap(a, b);
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__UTIL__STRING__INCLUDED
