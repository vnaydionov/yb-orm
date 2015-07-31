#include "micro_http.h"
#include <sstream>
#include <util/thread.h>
#include <util/utility.h>
#include <util/string_utils.h>

using namespace std;
using namespace Yb;
using namespace Yb::StrUtils;

HttpServerBase::HttpServerBase(int port, ILogger *root_logger,
        const String &content_type, const std::string &bad_resp)
    : port_(port)
    , content_type_(content_type)
    , bad_resp_(bad_resp)
    , log_(root_logger->new_logger("http_serv").release())
{}

HttpHeaders
HttpServerBase::make_response(int code, const Yb::String &desc,
        const std::string &body, const Yb::String &cont_type)
{
    HttpHeaders response(10, code, desc);
    response.set_response_body(body, cont_type);
    return response;
}

bool
HttpServerBase::send_response(TcpSocket &cl_sock, ILogger &logger,
        const HttpHeaders &response)
{
    try {
        cl_sock.write(response.serialize());
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
    server->process_client_request(cl_s);
}

void
HttpServerBase::process_client_request(SOCKET cl_s)
{
    TcpSocket cl_sock(cl_s);
    ILogger::Ptr logger = log_->new_logger("worker");
    string &bad_resp = bad_resp_;
    String cont_type_resp = content_type_;
    // read and process request
    try {
        // read request header
        string buf = cl_sock.readline();
        logger->debug(buf);
        // parse request line
        Strings head_parts;
        split_str_by_chars(WIDEN(buf), _T(" \t\r\n"), head_parts);
        if (head_parts.size() != 3)
            throw HttpParserError("process_client_request", "head_parts.size() != 3");
        HttpHeaders request_obj(head_parts[0],
                                head_parts[1],
                                HttpHeaders::parse_version(head_parts[2]));
        // read all of the headers
        String header_name, header_value;
        while (1) {
            string s = cl_sock.readline();
            if (!s.size())
                throw HttpParserError("process", "short read");
            if (s == "\r\n" || s == "\n")
                break;
            String s2 = WIDEN(s);
            if (!is_space(s2[0])) {
                Strings parts;
                split_str_by_chars(s2, _T(":"), parts, 2);
                if (parts.size() != 2)
                    throw HttpParserError("process", "Header format is wrong");
                if (!str_empty(header_name))
                {
                    request_obj.set_header(header_name,
                                           trim_trailing_space(header_value));
                    logger->debug(NARROW(header_name + _T(": ") +
                                         trim_trailing_space(header_value)));
                }
                header_name = trim_trailing_space(parts[0]);
                header_value = parts[1];
            }
            else {
                header_value += s2;
            }
        }
        if (!str_empty(header_name))
        {
            request_obj.set_header(header_name,
                                   trim_trailing_space(header_value));
            logger->debug(NARROW(header_name + _T(": ") +
                                 trim_trailing_space(header_value)));
        }
        // parse content length and type
        int cont_len = -1;
        try {
            from_string(request_obj.get_header(_T("Content-Length")), cont_len);
        }
        catch (const std::exception &ex) {
            logger->warning(
                string("couldn't parse Content-Length: ") + ex.what());
        }
        String cont_type;
        try {
            cont_type = request_obj.get_header(_T("Content-Type"));
        }
        catch (const std::exception &ex) {
            logger->warning(
                string("couldn't parse Content-Type: ") + ex.what());
        }
        // read request body
        if (cont_len > 0)
            request_obj.set_request_body(cl_sock.read(cont_len),
                                 str_to_lower(cont_type) ==
                                 _T("application/x-www-form-urlencoded"));
        if (request_obj.get_method() != _T("GET") &&
            request_obj.get_method() != _T("POST"))
        {
            logger->error("unsupported method \""
                          + NARROW(request_obj.get_method()) + "\"");
            send_response(cl_sock, *logger,
                          make_response(400, _T("Bad request"), bad_resp, cont_type_resp));
        }
        else {
            if (!has_handler_for_path(request_obj.get_path()))
            {
                logger->error("Path " + NARROW(request_obj.get_path()) + " not found!");
                send_response(cl_sock, *logger,
                              make_response(404, _T("Not found"), bad_resp, cont_type_resp));
            }
            else {
                // handle the request
                send_response(cl_sock, *logger,
                              call_handler(request_obj));
            }
        }
    }
    catch (const SocketEx &ex) {
        logger->error(string("socket error: ") + ex.what());
        try {
            send_response(cl_sock, *logger,
                          make_response(400, _T("Short read"), bad_resp, cont_type_resp));
        }
        catch (const std::exception &ex2) {
            logger->error(string("unable to send: ") + ex2.what());
        }
    }
    catch (const HttpParserError &ex) {
        logger->error(string("parser error: ") + ex.what());
        try {
            send_response(cl_sock, *logger,
                          make_response(400, _T("Bad request"), bad_resp, cont_type_resp));
        }
        catch (const std::exception &ex2) {
            logger->error(string("unable to send: ") + ex2.what());
        }
    }
    catch (const std::exception &ex) {
        logger->error(string("exception: ") + ex.what());
        try {
            send_response(cl_sock, *logger,
                          make_response(500, _T("Internal server error"), bad_resp, cont_type_resp));
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

StringDict
HttpHeaders::parse_params(const String &msg)
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

const Yb::String my_url_encode(const string &s, bool path_mode=false)
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

Yb::String
HttpHeaders::serialize_params(const Yb::StringDict &d)
{
    String result;
    Yb::StringDict::const_iterator it = d.begin(), end = d.end();
    for (; it != end; ++it) {
        if (str_length(result))
            result += _T("&");
        result += my_url_encode(NARROW(it->first));
        result += _T("=");
        result += my_url_encode(NARROW(it->second));
    }
    return result;
}

const String
HttpHeaders::normalize_header_name(const String &name)
{
    String s = str_to_lower(trim_trailing_space(name));
    for (int i = 0; i < (int)str_length(s); ++i)
    {
        if (!i || char_code(s[i - 1]) == '-')
            s[i] = to_upper(s[i]);
    }
    return s;
}

// vim:ts=4:sts=4:sw=4:et:
