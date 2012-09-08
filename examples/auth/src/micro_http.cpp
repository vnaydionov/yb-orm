#include "micro_http.h"
#include <sstream>
#include <util/Thread.h>
#include <util/Utility.h>
#include <util/DataTypes.h>
#include <util/str_utils.hpp>

using namespace std;
using namespace Yb::StrUtils;
using namespace Yb;

HttpServer::HttpServer(int port, const HttpHandlerMap &handlers,
            ILogger *root_logger)
    : port_(port)
    , handlers_(handlers)
    , log_(root_logger->new_logger("http_serv").release())
{}

bool
HttpServer::send_response(TcpSocket &cl_sock, ILogger &logger,
        int code, const string &desc, const string &body,
        const String &cont_type)
{
    try {
        ostringstream out;
        out << "HTTP/1.0 " << code << " " << desc
            << "\nContent-Type: " << NARROW(cont_type)
            << "\nContent-Length: " << body.size()
            << "\n\n" << body;
        cl_sock.write(out.str());
        cl_sock.close(true);
        return true;
    }
    catch (const std::exception &ex) {
        logger.error(string("exception: ") + ex.what());
    }
    return false;
}

void
HttpServer::process(SOCKET cl_s, ILogger *log_ptr, const HttpHandlerMap *handlers)
{
    TcpSocket cl_sock(cl_s);
    ILogger::Ptr logger = log_ptr->new_logger("worker");
    // read and process request
    try {
        // read request header
        string buf = cl_sock.readline();
        logger->debug(buf);
        int cont_len = -1;
        String cont_type;
        while (1) { // skip to request's body
            string s = cl_sock.readline();
            if (s == "\r\n" || s == "\n")
                break;
            logger->debug(s);
            String s2 = WIDEN(s);
            if (starts_with(str_to_upper(s2), _T("CONTENT-LENGTH: "))) {
                try {
                    Strings parts;
                    split_str_by_chars(s2, _T(" "), parts, 2);
                    from_string(parts[1], cont_len);
                }
                catch (const std::exception &ex) {
                    logger->warning(
                        string("couldn't parse CONTENT-LENGTH: ") + ex.what());
                }
            }
            if (starts_with(str_to_upper(s2), _T("CONTENT-TYPE: "))) {
                try {
                    Strings parts;
                    split_str_by_chars(s2, _T(": "), parts, 2);
                    cont_type = trim_trailing_space(parts[1]);
                }
                catch (const std::exception &ex) {
                    logger->warning(
                        string("couldn't parse CONTENT-TYPE: ") + ex.what());
                }
            }
        }
        // parse request
        StringDict rez = parse_http(WIDEN(buf));
        rez[_T("&content-type")] = cont_type;
        String method = rez.get(_T("&method"));
        if (method != _T("GET") && method != _T("POST")) {
            logger->error("unsupported method \""
                    + NARROW(method) + "\"");
            send_response(cl_sock, *logger, 400, "Bad request", BAD_RESP);
        }
        else {
            if (method == _T("POST") && cont_len > 0) {
                String post_data = WIDEN(cl_sock.read(cont_len));
                if (cont_type == _T("application/x-www-form-urlencoded"))
                    rez.update(parse_params(post_data));
                else
                    rez[_T("&post-data")] = post_data;
            }
            String uri = rez.get(_T("&uri"));
            if (!handlers->has(uri)) {
                logger->error("URI " + NARROW(uri) + " not found!");
                send_response(cl_sock, *logger, 404, "Not found", BAD_RESP);
            }
            else {
                // handle the request
                HttpHandler func_ptr = handlers->get(uri);
                send_response(cl_sock, *logger, 200, "OK", func_ptr(rez));
            }
        }
    }
    catch (const SocketEx &ex) {
        logger->error(string("socket error: ") + ex.what());
        //if connection is closed by peer - this is killing the whole process:
        //send_response(cl_sock, *logger, 400, "Short read", BAD_RESP);
        cl_sock.close(true);
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

typedef void (*WorkerFunc)(SOCKET, ILogger *, const HttpHandlerMap *);

class WorkerThread: public Thread {
    SOCKET s_;
    ILogger *log_;
    const HttpHandlerMap *handlers_;
    WorkerFunc worker_;
    void on_run() { worker_(s_, log_, handlers_); }
public:
    WorkerThread(SOCKET s, ILogger *log, const HttpHandlerMap *handlers,
            WorkerFunc worker)
        : s_(s), log_(log), handlers_(handlers), worker_(worker)
    {}
};

void
HttpServer::serve()
{
    TcpSocket::init_socket_lib();
    log_->info("start server on port " + to_stdstring(port_));
    sock_ = TcpSocket(TcpSocket::create());
    sock_.bind(port_);
    sock_.listen();
    typedef SharedPtr<WorkerThread>::Type WorkerThreadPtr;
    typedef std::vector<WorkerThreadPtr> Workers;
    Workers workers;
    while (1) {
        // accept request
        try {
            SOCKET cl_sock = sock_.accept();
            log_->info("accepted");
            WorkerThreadPtr worker(new WorkerThread(
                    cl_sock, log_.get(), &handlers_, HttpServer::process));
            workers.push_back(worker);
            worker->start();
            Workers workers_dead, workers_alive;
            for (Workers::iterator i = workers.begin(); i != workers.end(); ++i)
                if ((*i)->finished())
                    workers_dead.push_back(*i);
                else
                    workers_alive.push_back(*i);
            for (Workers::iterator i = workers_dead.begin(); i != workers_dead.end(); ++i) {
                (*i)->terminate();
                (*i)->wait();
            }
            workers.swap(workers_alive);
        }
        catch (const std::exception &ex) {
            log_->error(string("exception: ") + ex.what());
        }
    }
}

StringDict parse_params(const String &msg)
{
    StringDict params;
    Strings param_parts;
    split_str_by_chars(msg, _T("&"), param_parts);
    for (size_t i = 0; i < param_parts.size(); ++i) {
        Strings value_parts;
        split_str_by_chars(param_parts[i], _T("="), value_parts, 2);
        if (value_parts.size() < 1)
            throw ParserEx("parse_params", "value_parts.size() < 1");
        String n = value_parts[0];
        String v;
        if (value_parts.size() == 2)
            v = WIDEN(url_decode(value_parts[1]));
        StringDict::iterator it = params.find(n);
        if (it == params.end())
            params[n] = v;
        else
            params[n] += v;
    }
    return params;
}

StringDict parse_http(const String &msg)
{
    Strings head_parts;
    split_str_by_chars(msg, _T(" \t\r\n"), head_parts);
    if (head_parts.size() != 3)
        throw ParserEx("parse_http", "head_parts.size() != 3");
    Strings uri_parts;
    split_str_by_chars(head_parts[1], _T("?"), uri_parts, 2);
    if (uri_parts.size() < 1)
        throw ParserEx("parse_http", "uri_parts.size() < 1");
    StringDict params;
    if (uri_parts.size() == 2)
        params = parse_params(uri_parts[1]);
    params[_T("&method")] = head_parts[0];
    params[_T("&version")] = head_parts[2];
    params[_T("&uri")] = uri_parts[0];
    return params;
}
    
// vim:ts=4:sts=4:sw=4:et:
