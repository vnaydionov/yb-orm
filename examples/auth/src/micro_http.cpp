#include "micro_http.h"
#include <sstream>
#include <util/thread.h>
#include <util/utility.h>
#include <util/string_utils.h>

using namespace std;
using namespace Yb;
using namespace Yb::StrUtils;

HttpServerBase::HttpServerBase(int port, ILogger *root_logger,
        const String &content_type, const String &bad_resp)
    : port_(port)
    , content_type_(content_type)
    , bad_resp_(bad_resp)
    , log_(root_logger->new_logger("http_serv").release())
{}

bool
HttpServerBase::send_response(TcpSocket &cl_sock, ILogger &logger,
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
HttpServerBase::process(HttpServerBase *server, SOCKET cl_s)
{
    TcpSocket cl_sock(cl_s);
    ILogger::Ptr logger = server->log_->new_logger("worker");
    string bad_resp = NARROW(server->bad_resp_);
    String cont_type_resp = server->content_type_;
    // read and process request
    try {
        // read request header
        string buf = cl_sock.readline();
        logger->debug(buf);
        int cont_len = -1;
        String cont_type;
        while (1) { // skip to request's body
            string s = cl_sock.readline();
            if (!s.size())
                throw HttpParserError("process", "short read");
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
        StringDict req = parse_http(WIDEN(buf));
        req[_T("&content-type")] = cont_type;
        String method = req.get(_T("&method"));
        if (method != _T("GET") && method != _T("POST")) {
            logger->error("unsupported method \""
                    + NARROW(method) + "\"");
            send_response(cl_sock, *logger,
                    400, "Bad request", bad_resp, cont_type_resp);
        }
        else {
            if (method == _T("POST") && cont_len > 0) {
                String post_data = WIDEN(cl_sock.read(cont_len));
                if (cont_type == _T("application/x-www-form-urlencoded"))
                    req.update(parse_params(post_data));
                else
                    req[_T("&post-data")] = post_data;
            }
            String uri = req.get(_T("&uri"));
            if (!server->has_uri(uri)) {
                logger->error("URI " + NARROW(uri) + " not found!");
                send_response(cl_sock, *logger,
                        404, "Not found", bad_resp, cont_type_resp);
            }
            else {
                // handle the request
                send_response(cl_sock, *logger, 200, "OK",
                        server->call_uri(uri, req), cont_type_resp);
            }
        }
    }
    catch (const SocketEx &ex) {
        logger->error(string("socket error: ") + ex.what());
        try {
            send_response(cl_sock, *logger,
                    400, "Short read", bad_resp, cont_type_resp);
        }
        catch (const std::exception &ex2) {
            logger->error(string("unable to send: ") + ex2.what());
        }
    }
    catch (const HttpParserError &ex) {
        logger->error(string("parser error: ") + ex.what());
        try {
            send_response(cl_sock, *logger,
                    400, "Bad request", bad_resp, cont_type_resp);
        }
        catch (const std::exception &ex2) {
            logger->error(string("unable to send: ") + ex2.what());
        }
    }
    catch (const std::exception &ex) {
        logger->error(string("exception: ") + ex.what());
        try {
            send_response(cl_sock, *logger,
                    500, "Internal server error", bad_resp, cont_type_resp);
        }
        catch (const std::exception &ex2) {
            logger->error(string("unable to send: ") + ex2.what());
        }
    }
    cl_sock.close(true);
}

typedef void (*WorkerFunc)(HttpServerBase *, SOCKET);

class WorkerThread: public Thread {
    HttpServerBase *serv_;
    SOCKET s_;
    WorkerFunc worker_;
    void on_run() { worker_(serv_, s_); }
public:
    WorkerThread(HttpServerBase *serv, SOCKET s, WorkerFunc worker)
        : serv_(serv), s_(s), worker_(worker)
    {}
};

void
HttpServerBase::serve()
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
            string ip_addr;
            int ip_port;
            SOCKET cl_sock = sock_.accept(&ip_addr, &ip_port);
            log_->info("accepted from " + ip_addr + ":" + to_stdstring(ip_port));
            WorkerThreadPtr worker(new WorkerThread(
                        this, cl_sock, HttpServerBase::process));
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
            throw HttpParserError("parse_params", "value_parts.size() < 1");
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
        throw HttpParserError("parse_http", "head_parts.size() != 3");
    Strings uri_parts;
    split_str_by_chars(head_parts[1], _T("?"), uri_parts, 2);
    if (uri_parts.size() < 1)
        throw HttpParserError("parse_http", "uri_parts.size() < 1");
    StringDict params;
    if (uri_parts.size() == 2)
        params = parse_params(uri_parts[1]);
    params[_T("&method")] = head_parts[0];
    params[_T("&version")] = head_parts[2];
    params[_T("&uri")] = uri_parts[0];
    return params;
}

// vim:ts=4:sts=4:sw=4:et:
