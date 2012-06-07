#ifndef _AUTH__MICRO_HTTP_H_
#define _AUTH__MICRO_HTTP_H_

#include <vector>
#include <string>
#include <map>
#include <memory>
#include "tcp_socket.h"
#include "logger.h"

typedef std::vector<std::string> Strings;
typedef std::map<std::string, std::string> StringMap;
typedef std::string (*HttpHandler)(StringMap &request);
typedef std::map<std::string, HttpHandler> HttpHandlerMap;

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
            const std::string &cont_type = "text/xml");
    static void process(SOCKET cl_s, Yb::ILogger *log_ptr,
            const HttpHandlerMap *handlers);
    // non-copyable
    HttpServer(const HttpServer &);
    HttpServer &operator=(const HttpServer &);
public:
    HttpServer(int port, const HttpHandlerMap &handlers);
    void serve();
};

int hex_digit(char c);
std::string url_decode(const std::string &s);
StringMap parse_params(const std::string &s);
StringMap parse_http(const std::string &s);

#endif
// vim:ts=4:sts=4:sw=4:et:
