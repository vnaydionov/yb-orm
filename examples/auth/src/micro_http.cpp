#include "micro_http.h"
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <util/str_utils.hpp>

using namespace std;
using namespace Yb::StrUtils;

HttpServer::HttpServer(int port, const HttpHandlerMap &handlers)
    : port_(port)
    , handlers_(handlers)
    , log_(g_log->new_logger("http_serv").release())
{}

bool
HttpServer::send_response(TcpSocket &cl_sock, Yb::ILogger &logger,
        int code, const string &desc, const string &body,
        const string &cont_type)
{
    try {
        ostringstream out;
        out << "HTTP/1.0 " << code << " " << desc
            << "\nContent-Type: " << cont_type
            << "\nContent-Length: " << body.size()
            << "\n\n" << body;
        cl_sock.write(out.str());
        cl_sock.close(true);
        return true;
    }
    catch (const std::exception &ex) {
        logger.error(string("exception: ") + ex.what());
        return false;
    }
}

void
HttpServer::process(SOCKET cl_s, Yb::ILogger *log_ptr, const HttpHandlerMap *handlers)
{
    TcpSocket cl_sock(cl_s);
    Yb::ILogger::Ptr logger = log_ptr->new_logger("worker");
    // read and process request
    try {
        // read request header
        string buf = cl_sock.readline();
        logger->debug(buf);
        int cont_len = -1;
        while (1) { // skip to request's body
            string s = cl_sock.readline();
            if (s == "\r\n" || s == "\n")
                break;
            logger->debug(s);
            if (starts_with(str_to_upper(s), "CONTENT-LENGTH: ")) {
                try {
                    Strings parts;
                    split_str_by_chars(s, " ", parts, 2);
                    cont_len = boost::lexical_cast<unsigned>(
                        trim_trailing_space(parts[1]));
                }
                catch (const std::exception &ex) {
                    logger->warning(
                        string("couldn't parse CONTENT-LENGTH: ") + ex.what());
                }
            }
        }
        // parse request
        StringMap rez = parse_http(buf);
        if (rez["&method"].compare("GET") && rez["&method"].compare("POST")) {
            logger->error("unsupported method \"" + rez["&method"] + "\"");
            send_response(cl_sock, *logger, 400, "Bad request", BAD_RESP);
        }
        else {
            if (!rez["&method"].compare("POST") && cont_len > 0) {
                string post_data = cl_sock.read(cont_len);
                StringMap params = parse_params(post_data);
                for (StringMap::iterator i = params.begin(); i != params.end(); ++i)
                    rez[i->first] = i->second;
            }
            HttpHandlerMap::const_iterator it = handlers->find(rez["&uri"]);
            if (it == handlers->end()) {
                logger->error("URI " + rez["&uri"] + " not found!");
                send_response(cl_sock, *logger, 404, "Not found", BAD_RESP);
            }
            else {
                // handle the request
                HttpHandler func_ptr = it->second;
                send_response(cl_sock, *logger, 200, "OK", func_ptr(rez));
            }
        }
    }
    catch (const SocketEx &ex) {
        logger->error(string("socket error: ") + ex.what());
        send_response(cl_sock, *logger, 400, "Short read", BAD_RESP);
    }
    catch (const ParserEx &ex) {
        logger->error(string("parser error: ") + ex.what());
        send_response(cl_sock, *logger, 400, "Bad request", BAD_RESP);
    }
    catch (const std::exception &ex) {
        logger->error(string("exception: ") + ex.what());
        send_response(cl_sock, *logger, 500, "Internal server error", BAD_RESP);
    }
}

typedef void (*WorkerFunc)(SOCKET, Yb::ILogger *, const HttpHandlerMap *);

class WorkerCall {
    SOCKET s_;
    Yb::ILogger *log_;
    const HttpHandlerMap *handlers_;
    WorkerFunc worker_;
public:
    WorkerCall(SOCKET s, Yb::ILogger *log, const HttpHandlerMap *handlers,
            WorkerFunc worker)
        : s_(s), log_(log), handlers_(handlers), worker_(worker)
    {}
    void operator ()() {
        worker_(s_, log_, handlers_);
    }
};

void
HttpServer::serve()
{
    TcpSocket::init_socket_lib();
    log_->info("start server on port " +
            boost::lexical_cast<string>(port_));
    sock_ = TcpSocket(TcpSocket::create());
    sock_.bind(port_);
    sock_.listen();
    while (1) {
        // accept request
        try {
            SOCKET cl_sock = sock_.accept();
            log_->info("accepted");
            WorkerCall wcall(cl_sock, log_.get(), &handlers_, HttpServer::process);
            boost::thread worker(wcall);
        }
        catch (const std::exception &ex) {
            log_->error(string("exception: ") + ex.what());
        }
    }
}

int hex_digit(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    throw ParserEx("hex_digit", "invalid digit");
}

string url_decode(const string &s)
{
    string result;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] != '%')
            result.push_back(s[i]);
        else {
            ++i;
            if (s.size() - i < 2)
                throw ParserEx("url_decode", "short read");
            unsigned char c = hex_digit(s[i]) * 16 +
                hex_digit(s[i + 1]);
            ++i;
            result.push_back(c);
        }
    }
    return result;
}

StringMap parse_params(const string &msg)
{
    StringMap params;
    Strings param_parts;
    split_str_by_chars(msg, "&", param_parts);
    for (size_t i = 0; i < param_parts.size(); ++i) {
        Strings value_parts;
        split_str_by_chars(param_parts[i], "=", value_parts, 2);
        if (value_parts.size() < 1)
            throw ParserEx("parse_params", "value_parts.size() < 1");
        string n = value_parts[0], v;
        if (value_parts.size() == 2)
            v = url_decode(value_parts[1]);
        StringMap::iterator it = params.find(n);
        if (it == params.end())
            params[n] = v;
        else
            params[n] += v;
    }
    return params;
}

StringMap parse_http(const string &msg)
{
    Strings head_parts;
    split_str_by_chars(msg, " \t\r\n", head_parts);
    if (head_parts.size() != 3)
        throw ParserEx("parse_http", "head_parts.size() != 3");
    Strings uri_parts;
    split_str_by_chars(head_parts[1], "?", uri_parts, 2);
    if (uri_parts.size() < 1)
        throw ParserEx("parse_http", "uri_parts.size() < 1");
    StringMap params;
    if (uri_parts.size() == 2)
        params = parse_params(uri_parts[1]);
    params["&method"] = head_parts[0];
    params["&version"] = head_parts[2];
    params["&uri"] = uri_parts[0];
    return params;
}
    
// vim:ts=4:sts=4:sw=4:et:
