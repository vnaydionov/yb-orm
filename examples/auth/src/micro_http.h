#ifndef _AUTH__MICRO_HTTP_H_
#define _AUTH__MICRO_HTTP_H_

#include <util/data_types.h>
#include <util/nlogger.h>
#include "tcp_socket.h"

class HttpParserError: public std::runtime_error {
public: HttpParserError(const std::string &ctx, const std::string &msg)
    : std::runtime_error(ctx + ": " + msg) {}
};

class HttpServerBase
{
public:
    HttpServerBase(int port, Yb::ILogger *root_logger,
            const Yb::String &content_type, const Yb::String &bad_resp);
    void serve();
protected:
    virtual bool has_uri(const Yb::String &uri) = 0;
    virtual const std::string call_uri(const Yb::String &uri, 
            const Yb::StringDict &request) = 0;
private:
    int port_;
    Yb::String content_type_, bad_resp_;
    Yb::ILogger::Ptr log_;
    TcpSocket sock_;

    static bool send_response(TcpSocket &cl_sock, Yb::ILogger &logger,
            int code, const std::string &desc, const std::string &body,
            const Yb::String &cont_type);
    static void process(HttpServerBase *server, SOCKET cl_s);
    // non-copyable
    HttpServerBase(const HttpServerBase &);
    HttpServerBase &operator=(const HttpServerBase &);
};

template <class Handler>
class HttpServer: public HttpServerBase
{
public:
    typedef Yb::Dict<Yb::String, Handler> HandlerMap;
    HttpServer(int port, const HandlerMap &handlers,
            Yb::ILogger *root_logger,
            const Yb::String &content_type = _T("text/xml"),
            const Yb::String &bad_resp = _T("<status>NOT</status>")):
        HttpServerBase(port, root_logger, content_type, bad_resp),
        handlers_(handlers)
    {}
protected:
    virtual bool has_uri(const Yb::String &uri) {
        return handlers_.has(uri);
    }
    virtual const std::string call_uri(const Yb::String &uri, 
            const Yb::StringDict &request) {
        Handler func_ptr = handlers_.get(uri);
        return func_ptr(request);
    }
private:
    const HandlerMap handlers_;
};

Yb::StringDict parse_params(const Yb::String &s);
Yb::StringDict parse_http(const Yb::String &s);

#endif // _AUTH__MICRO_HTTP_H_
// vim:ts=4:sts=4:sw=4:et:
