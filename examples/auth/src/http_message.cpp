#include "http_message.h"
#include <stdio.h>
#include <sstream>
#include <util/string_utils.h>

using Yb::StrUtils::split_str_by_chars;
using Yb::StrUtils::url_decode;
using Yb::StrUtils::to_upper;
using Yb::StrUtils::str_to_upper;
using Yb::StrUtils::str_to_lower;
using Yb::StrUtils::trim_trailing_space;
using Yb::StrUtils::starts_with;

HttpHeaderNotFound::~HttpHeaderNotFound() throw()
{
}

HttpMessage::HttpMessage(int proto_ver)
    : proto_ver_(proto_ver)
{
    if (proto_ver != HTTP_1_1 && proto_ver != HTTP_1_0
            && proto_ver != HTTP_X)
        throw HttpParserError("HttpMessage", "Invalid protocol version: " +
                              NARROW(Yb::to_string(proto_ver)));
}

int
HttpMessage::parse_version(const Yb::String &proto_str)
{
    Yb::String v = str_to_upper(proto_str);
    if (v != _T("HTTP/1.0") && v != _T("HTTP/1.1"))
    {
        throw HttpParserError("parse_version", "Unrecognized HTTP version: " +
                              NARROW(v));
    }
    return (Yb::char_code(v[5]) - '0') * 10 +
           (Yb::char_code(v[7]) - '0');
}

const Yb::String
HttpMessage::normalize_header_name(const Yb::String &name)
{
    Yb::String s = str_to_lower(trim_trailing_space(name));
    for (int i = 0; i < (int)Yb::str_length(s); ++i)
    {
        if (!i || Yb::char_code(s[i - 1]) == '-')
            s[i] = to_upper(s[i]);
    }
    return s;
}

void
HttpMessage::parse_header_line(const Yb::String &line,
        Yb::String &header_name, Yb::String &header_value)
{
    Yb::Strings parts;
    split_str_by_chars(line, _T(":"), parts, 2);
    if (parts.size() != 2)
        throw HttpParserError("parse_header_line", "Header format is wrong");
    header_name = trim_trailing_space(parts[0]);
    header_value = trim_trailing_space(parts[1]);
}

HttpRequest::HttpRequest(const Yb::String &method, const Yb::String &uri, int proto_ver)
    : HttpMessage(proto_ver)
    , method_(Yb::StrUtils::str_to_upper(method))
    , uri_(uri)
{
    if (!Yb::str_length(method))
        throw HttpParserError("HttpRequest", "Empty HTTP method");
    if (!Yb::str_length(uri))
        throw HttpParserError("HttpRequest", "Empty URI");
    Yb::Strings uri_parts;
    Yb::StrUtils::split_str_by_chars(uri, _T("?"), uri_parts, 2);
    if (uri_parts.size() < 1)
        throw HttpParserError("HttpMessage", "uri_parts.size() < 1");
    path_ = uri_parts[0];
    if (uri_parts.size() > 1)
        params_ = parse_query_string(uri_parts[1]);
}

const std::string
HttpRequest::serialize() const
{
    std::ostringstream out;
    out << NARROW(method_) << " "
        << NARROW(uri_) << " "
        << NARROW(get_proto_str()) << "\n"
        << serialize_headers() << "\n" << body();
    return out.str();
}

const Yb::StringDict
HttpRequest::parse_query_string(const Yb::String &msg)
{
    Yb::StringDict params;
    Yb::Strings param_parts;
    split_str_by_chars(msg, _T("&"), param_parts);
    for (size_t i = 0; i < param_parts.size(); ++i) {
        Yb::Strings value_parts;
        split_str_by_chars(param_parts[i], _T("="), value_parts, 2);
        if (value_parts.size() < 1)
            throw HttpParserError("parse_query_string", "value_parts.size() < 1");
        Yb::String n = value_parts[0];
        Yb::String v;
        if (value_parts.size() == 2)
            v = WIDEN(HttpRequest::url_unquote(value_parts[1]));
        Yb::StringDict::iterator it = params.find(n);
        if (it == params.end())
            params[n] = v;
        else
            params[n] += v;
    }
    return params;
}

const Yb::String
HttpRequest::url_quote(const std::string &s, bool path_mode)
{
    Yb::String result;
    const char *reserved;
    if (path_mode)
        reserved = "!*'();@&=+$,?%#[]";
    else
        reserved = "!*'();:@&=+$,/?%#[]{}\"";
    char buf[20];
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = s[i];
        if (c <= 32 || c >= 127 || c == '%' || strchr(reserved, c)) {
            snprintf(buf, sizeof(buf), "%%%02X", c);
            buf[sizeof(buf) - 1] = 0;
            result += WIDEN(buf);
        }
        else
            result += Yb::Char(c);
    }
    return result;
}

const std::string
HttpRequest::url_unquote(const Yb::String &s)
{
    return url_decode(s);
}

const Yb::String
HttpRequest::serialize_params(const StringPairVector &v)
{
    Yb::String result;
    StringPairVector::const_iterator it = v.begin(), end = v.end();
    for (; it != end; ++it) {
        if (Yb::str_length(result))
            result += _T("&");
        result += HttpRequest::url_quote(NARROW(it->first));
        result += _T("=");
        result += HttpRequest::url_quote(NARROW(it->second));
    }
    return result;
}

const Yb::String
HttpRequest::serialize_params(const Yb::StringDict &d)
{
    Yb::String result;
    Yb::StringDict::const_iterator it = d.begin(), end = d.end();
    for (; it != end; ++it) {
        if (Yb::str_length(result))
            result += _T("&");
        result += HttpRequest::url_quote(NARROW(it->first));
        result += _T("=");
        result += HttpRequest::url_quote(NARROW(it->second));
    }
    return result;
}

const HttpRequest
HttpRequest::parse_request_line(const Yb::String &line)
{
    Yb::Strings head_parts;
    split_str_by_chars(line, _T(" \t\r\n"), head_parts);
    if (head_parts.size() != 3)
        throw HttpParserError("parse_request_line", "head_parts.size() != 3");
    HttpRequest request_obj(head_parts[0],
                            head_parts[1],
                            HttpMessage::parse_version(head_parts[2]));
    return request_obj;
}

HttpResponse::HttpResponse(int proto_ver, int http_status, const Yb::String &reason)
    : HttpMessage(proto_ver)
    , http_status_(http_status)
    , reason_(reason)
{
    if (proto_ver_ != HTTP_X && (http_status < 100 || http_status >= 600))
        throw HttpParserError("HttpResponse", "Invalid response code: " +
                              NARROW(Yb::to_string(http_status)));
    if (proto_ver_ != HTTP_X && !Yb::str_length(reason))
        throw HttpParserError("HttpResponse", "Empty HTTP reason");
}

const std::string
HttpResponse::serialize() const
{
    std::ostringstream out;
    out << NARROW(get_proto_str()) << " "
        << http_status_ << " "
        << NARROW(reason_) << "\n"
        << serialize_headers() << "\n" << body();
    return out.str();
}

void
HttpResponse::set_response_body(const std::string &body,
                                const Yb::String &content_type,
                                bool set_content_length)
{
    set_body(body);
    if (!Yb::str_empty(content_type))
        set_header(_T("Content-Type"), content_type);
    if (body.size() && set_content_length)
        set_header(_T("Content-Length"), Yb::to_string(body.size()));
}

const std::pair<int, Yb::String>
HttpResponse::parse_status(const Yb::String &line)
{
    std::vector<Yb::String> parts;
    split_str_by_chars(trim_trailing_space(line), _T(" "), parts, 3);
    if (parts.size() == 3
            && starts_with(str_to_upper(parts[0]), _T("HTTP/")))
    {
        int http_code;
        Yb::from_string(parts[1], http_code);
        Yb::String reason_phrase = trim_trailing_space(parts[2]);
        return std::make_pair(http_code, reason_phrase);
    }
    return std::make_pair(0, Yb::String());
}

void
HttpResponse::put_header_line(const std::string &line)
{
    std::pair<int, Yb::String> st = parse_status(WIDEN(line));
    if (st.first) {
        proto_ver_ = HTTP_1_1;
        http_status_ = st.first;
        reason_ = st.second;
        headers_ = Yb::StringDict();
        body_ = "";
    }
    else {
        Yb::String header, value;
        try {
            parse_header_line(WIDEN(line), header, value);
        }
        catch (const std::exception &)
        {}
        if (Yb::str_length(header))
            set_header(header, value);
    }
}

void
HttpResponse::put_body_piece(const std::string &line)
{
    body_ += line;
}

// vim:ts=4:sts=4:sw=4:et:
