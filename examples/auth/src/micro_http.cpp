#include "micro_http.h"
#include <util/thread.h>
#include <util/utility.h>
#include <util/string_utils.h>

using namespace std;
using namespace Yb;
using namespace Yb::StrUtils;

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

typedef SharedPtr<WorkerThread>::Type WorkerThreadPtr;
typedef std::vector<WorkerThreadPtr> Workers;

HttpServerBase::HttpServerBase(const std::string &ip_addr, int port,
        int back_log, ILogger *root_logger,
        const String &content_type, const std::string &bad_resp)
    : ip_addr_(ip_addr)
    , port_(port)
    , back_log_(back_log)
    , content_type_(content_type)
    , bad_resp_(bad_resp)
    , log_(root_logger->new_logger("micro_http").release())
    , prev_clean_ts(time(NULL))
{}

HttpResponse
HttpServerBase::make_response(int code, const Yb::String &desc,
        const std::string &body, const Yb::String &cont_type)
{
    HttpResponse response(HTTP_1_0, code, desc);
    response.set_response_body(body, cont_type);
    return response;
}

bool
HttpServerBase::send_response(TcpSocket &cl_sock, ILogger &logger,
                              const HttpResponse &response)
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
        String req_line = WIDEN(cl_sock.readline());
        logger->debug(NARROW(trim_trailing_space(req_line)));
        HttpRequest request_obj = HttpRequest::parse_request_line(req_line);
        // read all of the headers
        String header_name, header_value;
        while (1) {
            String s = WIDEN(cl_sock.readline());
            if (str_empty(s))
                throw HttpParserError("process", "short read");
            if (s == _T("\r\n") || s == _T("\n"))
                break;
            if (!is_space(s[0])) {
                String new_header_name, new_header_value;
                HttpMessage::parse_header_line(
                            s, new_header_name, new_header_value);
                if (!str_empty(header_name))
                {
                    header_value = trim_trailing_space(header_value);
                    request_obj.set_header(header_name, header_value);
                    logger->debug(NARROW(header_name + _T(": ") + header_value));
                }
                header_name = new_header_name;
                header_value = new_header_value;
            }
            else {
                header_value += s;
            }
        }
        if (!str_empty(header_name))
        {
            header_value = trim_trailing_space(header_value);
            request_obj.set_header(header_name, header_value);
            logger->debug(NARROW(header_name + _T(": ") + header_value));
        }
        // parse content length and type
        int cont_len = 0;
        String cont_type;
        if (request_obj.method() != _T("GET")) {
            try {
                cont_type = request_obj.get_content_type();
            }
            catch (const std::exception &) {
                logger->warning("couldn't parse Content-Type header");
            }
            try {
                cont_len = request_obj.get_content_length();
            }
            catch (const std::exception &) {
                logger->warning("couldn't parse Content-Length header");
            }
        }
        // read request body
        if (cont_len > 0) {
            request_obj.set_body(cl_sock.read(cont_len));
            if (starts_with(cont_type, _T("application/x-www-form-urlencoded")))
                request_obj.urlparse_body();
        }
        if (request_obj.method() != _T("GET") &&
            request_obj.method() != _T("POST"))
        {
            logger->error("unsupported method \""
                          + NARROW(request_obj.method()) + "\"");
            send_response(cl_sock, *logger,
                          make_response(400, _T("Bad request"), bad_resp, cont_type_resp));
        }
        else {
            if (!has_handler_for_path(request_obj.path()))
            {
                logger->error("Path " + NARROW(request_obj.path()) + " not found!");
                send_response(cl_sock, *logger,
                              make_response(404, _T("Not found"), bad_resp, cont_type_resp));
            }
            else {
                // handle the request
                send_response(cl_sock, *logger, call_handler(request_obj));
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

static void
cleanup_workers(bool force,
        time_t &prev_clean_ts, Workers &workers, Yb::ILogger *log_)
{
    if (force || time(NULL) - prev_clean_ts >= 5) {   // sec.
        log_->info("clean up workers");
        Workers workers_dead, workers_alive;
        workers_dead.reserve(workers.size() + 1);
        workers_alive.reserve(workers.size() + 1);
        for (Workers::iterator i = workers.begin();
             i != workers.end(); ++i)
        {
            if ((*i)->finished())
                workers_dead.push_back(*i);
            else
                workers_alive.push_back(*i);
        }
        for (Workers::iterator i = workers_dead.begin();
             i != workers_dead.end(); ++i)
        {
            (*i)->terminate();
            (*i)->wait();
        }
        log_->info("workers: "
                   "processing=" + NARROW(to_string(workers_alive.size())) +
                   ", done=" + NARROW(to_string(workers_dead.size())));
        workers.swap(workers_alive);
        prev_clean_ts = time(NULL);
    }
}


void
HttpServerBase::serve()
{
    TcpSocket::init_socket_lib();
    log_->info("start server on port " + to_stdstring(port_));
    sock_.bind(ip_addr_, port_);
    sock_.listen(back_log_);
    Workers workers;
    while (1) {
        // accept request
        bool force_clean = false;
        SOCKET cl_sock = INVALID_SOCKET;
        try {
            log_->debug("waiting for incoming connect...");
            string ip_addr;
            int ip_port;
            cl_sock = sock_.accept(&ip_addr, &ip_port);
            log_->info("accepted from " + ip_addr + ":" + to_stdstring(ip_port));
            WorkerThreadPtr worker(new WorkerThread(
                        this, cl_sock, HttpServerBase::process));
            workers.push_back(worker);
            worker->start();
        }
        catch (const std::exception &ex) {
            log_->error(string("exception: ") + ex.what());
            if (cl_sock != INVALID_SOCKET) {
                log_->info("closing failed socket");
                TcpSocket s(cl_sock);
            }
            force_clean = true;
        }
        try {
            cleanup_workers(force_clean,
                    prev_clean_ts, workers, log_.get());
        }
        catch (const std::exception &ex) {
            log_->error(string("cleanup exception: ") + ex.what());
        }
    }
}

// vim:ts=4:sts=4:sw=4:et:
