#ifndef _AUTH__MICRO_HTTP_H_
#define _AUTH__MICRO_HTTP_H_

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <util/DataTypes.h>
#include "tcp_socket.h"
#include "logger.h"

typedef std::string (*HttpHandler)(const Yb::StringDict &request);
typedef Yb::Dict<Yb::String, HttpHandler> HttpHandlerMap;

#define OK_RESP "<status>OK</status>"
#define BAD_RESP "<status>NOT</status>"

class ParserEx: public std::runtime_error {
public: ParserEx(const std::string &ctx, const std::string &msg)
    : std::runtime_error(ctx + ": " + msg) {}
};

class HttpServer
{
    int port_;
    const HttpHandlerMap handlers_;
    TcpSocket sock_;
    Yb::ILogger::Ptr log_;
    static bool send_response(TcpSocket &cl_sock, Yb::ILogger &logger,
            int code, const std::string &desc, const std::string &body,
            const Yb::String &cont_type = _T("text/xml"));
    static void process(SOCKET cl_s, Yb::ILogger *log_ptr,
            const HttpHandlerMap *handlers);
    // non-copyable
    HttpServer(const HttpServer &);
    HttpServer &operator=(const HttpServer &);
public:
    HttpServer(int port, const HttpHandlerMap &handlers,
            Yb::ILogger *root_logger);
    void serve();
};

Yb::StringDict parse_params(const Yb::String &s);
Yb::StringDict parse_http(const Yb::String &s);

#endif
// vim:ts=4:sts=4:sw=4:et:
