#ifndef _AUTH__HTTP_MESSAGE_H_
#define _AUTH__HTTP_MESSAGE_H_

#include <util/data_types.h>

enum {
    HTTP_1_0 = 10,
    HTTP_1_1 = 11,
};


class HttpParserError: public std::runtime_error
{
public:
    HttpParserError(const std::string &ctx, const std::string &msg)
        : std::runtime_error(ctx + ": " + msg)
    {}
};

class HttpHeaderNotFound: public HttpParserError
{
    std::string header_name_;
public:
    HttpHeaderNotFound(const std::string &header_name)
        : HttpParserError("get_header", "Header not found: " + header_name)
        , header_name_(header_name)
    {}
    const std::string &header_name() const { return header_name_; }
    virtual ~HttpHeaderNotFound() throw();
};


class HttpMessage
{
public:
    explicit HttpMessage(int proto_ver);

    virtual const std::string serialize() const = 0;

    int proto_ver() const { return proto_ver_; }

    const Yb::String get_proto_str() const
    {
        return _T("HTTP/") + Yb::to_string(proto_ver_ / 10) +
            _T(".") + Yb::to_string(proto_ver_ % 10);
    }

    void set_body(const std::string &body)
    {
        body_ = body;
    }

    void set_body(std::string &body)
    {
        std::swap(body_, body);
    }

    const std::string &body() const { return body_; }

    void set_header(const Yb::String &header, const Yb::String &value)
    {
        headers_[normalize_header_name(header)] = value;
    }

    void set_headers(const Yb::StringDict &h)
    {
        headers_ = h;
    }

    void set_headers(Yb::StringDict &h)
    {
        std::swap(headers_, h);
    }

    const Yb::String &get_header(const Yb::String &header) const
    {
        Yb::StringDict::const_iterator it = headers_.find(
            normalize_header_name(header));
        if (headers_.end() == it)
            throw HttpHeaderNotFound(NARROW(header));
        return it->second;
    }

    const Yb::String get_header(const Yb::String &header, const Yb::String &default_value) const
    {
        Yb::StringDict::const_iterator it = headers_.find(
            normalize_header_name(header));
        if (headers_.end() == it)
            return default_value;
        return it->second;
    }

    const Yb::StringDict &headers() const { return headers_; }

    const Yb::String &get_content_type() const {
        return get_header(_T("Content-Type"));
    }

    int get_content_length() const {
        int len;
        Yb::from_string(get_header(_T("Content-Length")), len);
        return len;
    }

    const std::string serialize_headers() const
    {
        std::ostringstream out;
        Yb::StringDict::const_iterator it = headers_.begin(), end = headers_.end();
        for (; it != end; ++it)
        {
            out << NARROW(it->first) << ": " << NARROW(it->second) << "\n";
        }
        return out.str();
    }


    static int parse_version(const Yb::String &proto_str);

    static const Yb::String normalize_header_name(const Yb::String &name);

    static void parse_header_line(const Yb::String &line,
            Yb::String &header_name, Yb::String &header_value);
private:
    int proto_ver_;  // HTTP_1_0 HTTP_1_1 ..
    Yb::StringDict headers_;
    std::string body_;
};


class HttpRequest: public HttpMessage
{
public:
    HttpRequest(const Yb::String &method, const Yb::String &uri, int proto_ver);

    virtual const std::string serialize() const;

    void urlparse_body() {
        params_.update(parse_params(WIDEN(body())));
    }

    const Yb::String &method() const { return method_; }

    const Yb::String &uri() const { return uri_; }

    const Yb::String &path() const { return path_; }

    const Yb::StringDict &params() const { return params_; }


    static const Yb::StringDict parse_params(const Yb::String &s);

    static const Yb::String url_encode(const std::string &s, bool path_mode=false);

    static const Yb::String serialize_params(const Yb::StringDict &d);

    static const HttpRequest parse_request_line(const Yb::String &line);
private:
    Yb::String method_;
    Yb::String uri_;
    Yb::String path_;
    Yb::StringDict params_;
};


class HttpResponse: public HttpMessage
{
public:
    HttpResponse(int proto_ver, int resp_code, const Yb::String &resp_desc);

    virtual const std::string serialize() const;

    void set_response_body(const std::string &body,
                           const Yb::String &content_type,
                           bool set_content_length=true);

    int resp_code() const { return resp_code_; }

    const Yb::String &resp_desc() const { return resp_desc_; }

private:
    int resp_code_;  // 200 404 ...
    Yb::String resp_desc_;
};

#endif // _AUTH__HTTP_MESSAGE_H_
// vim:ts=4:sts=4:sw=4:et:
