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

HttpHeaderNotFound::~HttpHeaderNotFound() throw()
{
}

HttpMessage::HttpMessage(int proto_ver)
    : proto_ver_(proto_ver)
{
    if (proto_ver != HTTP_1_1 && proto_ver != HTTP_1_0)
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
    std::swap(header_name, parts[0]);
    std::swap(header_value, parts[1]);
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
        params_ = parse_params(uri_parts[1]);
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
HttpRequest::parse_params(const Yb::String &msg)
{
    Yb::StringDict params;
    Yb::Strings param_parts;
    split_str_by_chars(msg, _T("&"), param_parts);
    for (size_t i = 0; i < param_parts.size(); ++i) {
        Yb::Strings value_parts;
        split_str_by_chars(param_parts[i], _T("="), value_parts, 2);
        if (value_parts.size() < 1)
            throw HttpParserError("parse_params", "value_parts.size() < 1");
        Yb::String n = value_parts[0];
        Yb::String v;
        if (value_parts.size() == 2)
            v = WIDEN(url_decode(value_parts[1]));
        Yb::StringDict::iterator it = params.find(n);
        if (it == params.end())
            params[n] = v;
        else
            params[n] += v;
    }
    return params;
}

const Yb::String
HttpRequest::url_encode(const std::string &s, bool path_mode)
{
    Yb::String result;
    const char *replace;
    if (path_mode)
        replace = "!*'();@&=+$,?%#[]";
    else
        replace = "!*'();:@&=+$,/?%#[]{}\"";
    char buf[20];
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = s[i];
        if (c <= 32 || c >= 127 || strchr(replace, c)) {
            sprintf(buf, "%%%02X", c);
            result += WIDEN(buf);
        }
        else
            result += Yb::Char(c);
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
        result += HttpRequest::url_encode(NARROW(it->first));
        result += _T("=");
        result += HttpRequest::url_encode(NARROW(it->second));
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

HttpResponse::HttpResponse(int proto_ver, int resp_code, const Yb::String &resp_desc)
    : HttpMessage(proto_ver)
    , resp_code_(resp_code)
    , resp_desc_(resp_desc)
{
    if (resp_code < 100 || resp_code >= 600)
        throw HttpParserError("HttpResponse", "Invalid response code: " +
                              NARROW(Yb::to_string(resp_code)));
    if (!Yb::str_length(resp_desc))
        throw HttpParserError("HttpResponse", "Empty HTTP resp_desc");
}

const std::string
HttpResponse::serialize() const
{
    std::ostringstream out;
    out << NARROW(get_proto_str()) << " "
        << resp_code_ << " "
        << NARROW(resp_desc_) << "\n"
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

// vim:ts=4:sts=4:sw=4:et:
