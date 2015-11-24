#ifndef _AUTH__MICRO_HTTP_H_
#define _AUTH__MICRO_HTTP_H_

#include <util/data_types.h>
#include <util/string_utils.h>
#include <util/nlogger.h>
#include "tcp_socket.h"

Yb::StringDict parse_http_request_line(const Yb::String &req);
Yb::StringDict parse_http_uri(const Yb::String &uri);

class HttpParserError: public std::runtime_error
{
public: HttpParserError(const std::string &ctx, const std::string &msg)
    : std::runtime_error(ctx + ": " + msg) {}
};

class HttpHeaders
{
public:
    HttpHeaders()
        : is_a_request_(true)
        , proto_ver_(10)
        , resp_code_(0)
    {}

    HttpHeaders(const Yb::String &method, const Yb::String &uri, int proto_ver)
        : is_a_request_(true)
        , method_(Yb::StrUtils::str_to_upper(method))
        , uri_(uri)
        , proto_ver_(proto_ver)
        , resp_code_(0)
    {
        if (!Yb::str_length(method))
            throw HttpParserError("HttpHeaders", "Empty HTTP method");
        if (!Yb::str_length(uri))
            throw HttpParserError("HttpHeaders", "Empty URI");
        if (proto_ver != 11 && proto_ver != 10)
            throw HttpParserError("HttpHeaders", "Invalid protocol version: " +
                                  boost::lexical_cast<std::string>(proto_ver));
        Yb::Strings uri_parts;
        Yb::StrUtils::split_str_by_chars(uri, _T("?"), uri_parts, 2);
        if (uri_parts.size() < 1)
            throw HttpParserError("HttpHeaders", "uri_parts.size() < 1");
        path_ = uri_parts[0];
        if (uri_parts.size() > 1)
            params_ = parse_params(uri_parts[1]);
    }

    HttpHeaders(int proto_ver, int resp_code, const Yb::String &resp_desc)
        : is_a_request_(false)
        , proto_ver_(proto_ver)
        , resp_code_(resp_code)
        , resp_desc_(resp_desc)
    {
        if (proto_ver != 11 && proto_ver != 10)
            throw HttpParserError("HttpHeaders", "Invalid protocol version: " +
                                  boost::lexical_cast<std::string>(proto_ver));
        if (resp_code < 100 || resp_code >= 600)
            throw HttpParserError("HttpHeaders", "Invalid response code: " +
                                  boost::lexical_cast<std::string>(resp_code));
        if (!Yb::str_length(resp_desc))
            throw HttpParserError("HttpHeaders", "Empty HTTP resp_desc");
    }

    const Yb::String &get_method() const { return method_; }

    const Yb::String &get_uri() const { return uri_; }

    const Yb::String &get_path() const { return path_; }

    int get_proto_ver() const { return proto_ver_; }

    int get_resp_code() const { return resp_code_; }

    const Yb::String &get_resp_desc() const { return resp_desc_; }

    void set_request_body(const std::string &body, bool parse_body=false)
    {
        body_ = body;
        if (parse_body)
        {
            params_.update(parse_params(WIDEN(body_)));
        }
    }

    void set_response_body(const std::string &body,
                  const Yb::String &content_type=_T(""),
                  bool set_content_length=true)
    {
        body_ = body;
        if (!Yb::str_empty(content_type)) {
            set_header(_T("Content-Type"), content_type);
            if (set_content_length)
                set_header(_T("Content-Length"),
                           Yb::to_string(body.size()));
        }
    }

    const std::string &get_body() const { return body_; }

    void set_header(const Yb::String &header, const Yb::String &value)
    {
        headers_[normalize_header_name(header)] = value;
    }

    const Yb::String &get_header(const Yb::String &header) const
    {
        Yb::StringDict::const_iterator it = headers_.find(
            normalize_header_name(header));
        if (headers_.end() == it)
            throw HttpParserError("get_header", "Header not found: " +
                                  NARROW(header));
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

    const Yb::StringDict &get_headers() const { return headers_; }

    const Yb::StringDict &get_params() const { return params_; }

    const Yb::String get_proto_str() const
    {
        return _T("HTTP/") + Yb::to_string(proto_ver_ / 10) +
            _T(".") + Yb::to_string(proto_ver_ % 10);
    }

    static int parse_version(const Yb::String &proto_str)
    {
        Yb::String v = Yb::StrUtils::str_to_upper(proto_str);
        if (v != _T("HTTP/1.0") && v != _T("HTTP/1.1"))
        {
            throw HttpParserError("parse_version", "Unrecognized HTTP version: " +
                                  NARROW(v));
        }
        return (Yb::char_code(v[5]) - '0') * 10 +
               (Yb::char_code(v[7]) - '0');
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

    const std::string serialize() const
    {
        std::ostringstream out;
        if (is_a_request_)
        {
            out << NARROW(method_) << " "
                << NARROW(uri_) << " "
                << NARROW(get_proto_str()) << "\n"
                << serialize_headers() << "\n" << body_;
        }
        else
        {
            out << NARROW(get_proto_str()) << " "
                << resp_code_ << " "
                << NARROW(resp_desc_) << "\n"
                << serialize_headers() << "\n" << body_;
        }
        return out.str();
    }

    static Yb::StringDict parse_params(const Yb::String &s);
    static Yb::String serialize_params(const Yb::StringDict &d);

    static const Yb::String normalize_header_name(const Yb::String &name);
private:
    bool is_a_request_;
    Yb::String method_;
    Yb::String uri_;
    Yb::String path_;
    int proto_ver_;  // 10 11 ..
    int resp_code_;  // 200 404 ...
    Yb::String resp_desc_;
    Yb::StringDict headers_;
    Yb::StringDict params_;
    std::string body_;
};

class HttpServerBase
{
public:
    HttpServerBase(const std::string &ip_addr, int port, int back_log,
            Yb::ILogger *root_logger,
            const Yb::String &content_type, const std::string &bad_resp);
    void serve();

protected:
    virtual bool has_handler_for_path(const Yb::String &path) = 0;
    virtual const HttpHeaders call_handler(const HttpHeaders &request) = 0;

private:
    std::string ip_addr_;
    int port_;
    int back_log_;
    Yb::String content_type_;
    std::string bad_resp_;
    Yb::ILogger::Ptr log_;
    TcpSocket sock_;
    time_t prev_clean_ts;

    static void process(HttpServerBase *server, SOCKET cl_s);
    void process_client_request(SOCKET cl_s);
    static HttpHeaders make_response(int code, const Yb::String &desc,
                                     const std::string &body,
                                     const Yb::String &cont_type);
    static bool send_response(TcpSocket &cl_sock, Yb::ILogger &logger,
                              const HttpHeaders &response);
    // non-copyable
    HttpServerBase(const HttpServerBase &);
    HttpServerBase &operator=(const HttpServerBase &);
};

template <class Handler>
class HttpServer: public HttpServerBase
{
public:
    typedef Yb::Dict<Yb::String, Handler> HandlerMap;
    HttpServer(const std::string &ip_addr, int port, int back_log,
            const HandlerMap &handlers, Yb::ILogger *root_logger,
            const Yb::String &content_type = _T("text/xml"),
            const std::string &bad_resp = "<status>NOT</status>"):
        HttpServerBase(ip_addr, port, back_log, root_logger,
                       content_type, bad_resp),
        handlers_(handlers)
    {}
protected:
    virtual bool has_handler_for_path(const Yb::String &path)
    {
        return handlers_.has(path);
    }
    virtual const HttpHeaders call_handler(const HttpHeaders &request)
    {
        Handler func_ptr = handlers_.get(request.get_path());
        return func_ptr(request);
    }
private:
    const HandlerMap handlers_;
};

#endif // _AUTH__MICRO_HTTP_H_
// vim:ts=4:sts=4:sw=4:et:
