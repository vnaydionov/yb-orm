// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#define YBUTIL_SOURCE

#include "util/string_type.h"
#if defined(YB_USE_WX)
#include <wx/strconv.h>
#elif defined(YB_USE_QT)
#include <QTextCodec>
#endif
#include <cstring>
#include <locale>
#include <stdexcept>

namespace Yb {

static size_t do_fast_narrow(const std::wstring &wide, std::string &narrow)
{
    const wchar_t *src = wide.c_str();
    const wchar_t *src0 = src;
    char *dst = &narrow[0];
    do {
        wchar_t c = *src;
        if (!c || (c & ~(wchar_t)0x7f))
            break;
        *dst = (char)c;
        ++dst;
        ++src;
    } while (1);
    size_t processed = src - src0;
    if (processed == wide.size())
        narrow.resize(dst - &narrow[0]);
    return processed;
}

static size_t do_fast_widen(const std::string &narrow, std::wstring &wide)
{
    const char *src = narrow.c_str();
    const char *src0 = src;
    wchar_t *dst = &wide[0];
    do {
        char c = *src;
        if (!c || (c & ~(char)0x7f))
            break;
        *dst = (wchar_t)c;
        ++dst;
        ++src;
    } while (1);
    size_t processed = src - src0;
    if (processed == narrow.size())
        wide.resize(dst - &wide[0]);
    return processed;
}

YBUTIL_DECL const std::string fast_narrow(const std::wstring &wide)
{
    if (wide.empty())
        return std::string();
    std::string narrow(4 * wide.size(), '\0'); // max character length in UTF-8
    size_t processed = do_fast_narrow(wide, narrow);
    if (processed == wide.size())
        return narrow;
    throw std::runtime_error("non ascii detected, fast_narrow failed");
}

YBUTIL_DECL const std::wstring fast_widen(const std::string &narrow)
{
    if (narrow.empty())
        return std::wstring();
    std::wstring wide(narrow.size(), L'\0');
    size_t processed = do_fast_widen(narrow, wide);
    if (processed == narrow.size())
        return wide;
    throw std::runtime_error("non ascii detected, fast_widen failed");
}

static const std::string do_narrow(
        const std::wstring &wide, const std::locale &loc)
{
    if (wide.empty())
        return std::string();
    std::string narrow(4*wide.size(), '\0'); // max character length in UTF-8
    size_t processed = do_fast_narrow(wide, narrow);
    if (processed == wide.size())
        return narrow;

    typedef std::wstring::traits_type::state_type state_type;
    typedef std::codecvt<wchar_t, char, state_type> CVT;

    const CVT& cvt = std::use_facet<CVT>(loc);
    //std::string narrow(cvt.max_length()*wide.size(), '\0');
    state_type state = state_type();

    const wchar_t* from_beg = &wide[0];
    const wchar_t* from_end = from_beg + wide.size();
    const wchar_t* from_nxt;
    char* to_beg = &narrow[0];
    char* to_end = to_beg + narrow.size();
    char* to_nxt;

    std::string::size_type sz = 0;
    std::codecvt_base::result r;
    do {
        r = cvt.out(state, from_beg, from_end, from_nxt,
                    to_beg,   to_end,   to_nxt);
        switch (r)
        {
            case std::codecvt_base::error:
                throw std::runtime_error("error converting wstring to string");

            case std::codecvt_base::partial:
                sz += to_nxt - to_beg;
                narrow.resize(2*narrow.size());
                to_beg = &narrow[sz];
                to_end = &narrow[0] + narrow.size();
                break;

            case std::codecvt_base::noconv:
                narrow.resize(sz + (from_end-from_beg)*sizeof(wchar_t));
                std::memcpy(&narrow[sz], from_beg,(from_end-from_beg)*sizeof(wchar_t));
                r = std::codecvt_base::ok;
                break;

            case std::codecvt_base::ok:
                sz += to_nxt - to_beg;
                narrow.resize(sz);
                break;
        }
    } while (r != std::codecvt_base::ok);

    return narrow;
}

static const std::wstring do_widen(
        const std::string &narrow, const std::locale &loc)
{
    if (narrow.empty())
        return std::wstring();
    std::wstring wide(narrow.size(), L'\0');
    size_t processed = do_fast_widen(narrow, wide);
    if (processed == narrow.size())
        return wide;

    typedef std::string::traits_type::state_type state_type;
    typedef std::codecvt<wchar_t, char, state_type> CVT;

    const CVT& cvt = std::use_facet<CVT>(loc);
    state_type state = state_type();

    const char* from_beg = &narrow[0];
    const char* from_end = from_beg + narrow.size();
    const char* from_nxt;
    wchar_t* to_beg = &wide[0];
    wchar_t* to_end = to_beg + wide.size();
    wchar_t* to_nxt;
    std::wstring::size_type sz = 0;
    std::codecvt_base::result r;
    do {
        r = cvt.in(state, from_beg, from_end, from_nxt,
                   to_beg,   to_end,   to_nxt);
        switch (r)
        {
            case std::codecvt_base::error:
                throw std::runtime_error("error converting string to wstring");
            case std::codecvt_base::partial:
                sz += to_nxt - to_beg;
                wide.resize(2*wide.size());
                to_beg = &wide[sz];
                to_end = &wide[0] + wide.size();
                break;
            case std::codecvt_base::noconv:
                wide.resize(sz + (from_end-from_beg));
                std::memcpy(&wide[sz], from_beg, (std::size_t)(from_end-from_beg));
                r = std::codecvt_base::ok;
                break;
            case std::codecvt_base::ok:
                sz += to_nxt - to_beg;
                wide.resize(sz);
                break;
        }
    } while (r != std::codecvt_base::ok);

    return wide;
}

YBUTIL_DECL const std::string get_locale(const std::string &enc_name = "")
{
    if (enc_name.empty())
        return
#ifdef YBUTIL_WINDOWS
            "rus_rus.866"
#else
            "ru_RU.UTF-8"
#endif
        ;
    return enc_name;
}

YBUTIL_DECL const std::string str2std(const String &s, const std::string &enc_name)
{
#if defined(YB_USE_WX)
    if (enc_name.empty())
        return std::string(s.mb_str(wxConvUTF8));
    wxCSConv conv(wxString(enc_name.c_str(), wxConvUTF8).GetData());
    return std::string(s.mb_str(conv));
#elif defined(YB_USE_QT)
    if (enc_name.empty())
        return std::string(s.toLocal8Bit().constData());
    QTextCodec *codec = QTextCodec::codecForName(enc_name.c_str());
    return std::string(codec->fromUnicode(s).constData());
#elif defined(YB_USE_UNICODE)
    std::locale loc(get_locale(enc_name).c_str());
    return do_narrow(s, loc);
#else
    return s;
#endif
}

YBUTIL_DECL const String std2str(const std::string &s, const std::string &enc_name)
{
#if defined(YB_USE_WX)
    if (enc_name.empty())
        return wxString(s.c_str(), wxConvUTF8);
    wxCSConv conv(wxString(enc_name.c_str(), wxConvUTF8).GetData());
    return wxString(s.c_str(), conv);
#elif defined(YB_USE_QT)
    if (enc_name.empty())
        return QString::fromLocal8Bit(s.c_str());
    QTextCodec *codec = QTextCodec::codecForName(enc_name.c_str());
    return codec->toUnicode(s.c_str());
#elif defined(YB_USE_UNICODE)
    std::locale loc(get_locale(enc_name).c_str());
    return do_widen(s, loc);
#else
    return s;
#endif
}

YBUTIL_DECL const std::string str_narrow(
        const std::wstring &wide, const std::string &enc_name)
{
    std::locale loc(get_locale(enc_name).c_str());
    return do_narrow(wide, loc);
}

YBUTIL_DECL const std::wstring str_widen(
        const std::string &narrow, const std::string &enc_name)
{
    std::locale loc(get_locale(enc_name).c_str());
    return do_widen(narrow, loc);
}

YBUTIL_DECL const std::string get_locale_enc()
{
    std::string loc_name = get_locale();
    int pos = loc_name.find('.');
    if (pos != std::string::npos)
        return loc_name.substr(pos + 1);
    return loc_name;
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
