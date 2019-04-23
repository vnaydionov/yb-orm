#include "micro_http.h"
#include <util/thread.h>
#include <util/utility.h>
#include <util/string_utils.h>

static inline bool logger_ok(Yb::ILogger *x) { return x != NULL; }
static inline bool logger_ok(const Yb::ILogger::Ptr &x) { return x.get() != NULL; }

#define LOG_ERROR(msg) do { if (logger_ok(logger)) logger->error(msg); } while (0)
#define LOG_WARN(msg) do { if (logger_ok(logger)) logger->warning(msg); } while (0)
#define LOG_INFO(msg) do { if (logger_ok(logger)) logger->info(msg); } while (0)
#define LOG_DEBUG(msg) do { if (logger_ok(logger)) logger->debug(msg); } while (0)

using namespace std;
using namespace Yb;
using namespace Yb::StrUtils;

void HttpWorkerThread::process_task(const HttpWorkerTask &task)
{
    task.server->process_client_request(task.cl_s);
}


HttpServerBase::HttpServerBase(const std::string &ip_addr, int port,
        int back_log, ILogger *root_logger,
        const String &content_type, const std::string &bad_resp,
        int thread_pool_size)
    : is_bound_(false)
    , is_serving_(false)
    , ip_addr_(ip_addr)
    , port_(port)
    , back_log_(back_log)
    , content_type_(content_type)
    , bad_resp_(bad_resp)
    , log_(root_logger? root_logger->new_logger("micro_http").release(): NULL)
    , worker_pool_(thread_pool_size)
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
HttpServerBase::send_response(TcpSocket &cl_sock, ILogger *logger,
                              const HttpResponse &response)
{
    try {
        cl_sock.write(response.serialize());
        cl_sock.close(true);
        return true;
    }
    catch (const std::exception &ex) {
        LOG_ERROR(string("exception: ") + ex.what());
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
    ILogger::Ptr logger(log_.get()? log_->new_logger("worker").release(): NULL);
    string &bad_resp = bad_resp_;
    String cont_type_resp = content_type_;
    // read and process request
    try {
        // read request header
        String req_line = WIDEN(cl_sock.readline());
        LOG_DEBUG(NARROW(trim_trailing_space(req_line)));
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
                    LOG_DEBUG(NARROW(header_name + _T(": ") + header_value));
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
            LOG_DEBUG(NARROW(header_name + _T(": ") + header_value));
        }
        // parse content length and type
        int cont_len = 0;
        String cont_type;
        if (request_obj.method() != _T("GET")) {
            try {
                cont_type = request_obj.get_content_type();
            }
            catch (const std::exception &) {
                LOG_WARN("couldn't parse Content-Type header");
            }
            try {
                cont_len = request_obj.get_content_length();
            }
            catch (const std::exception &) {
                LOG_WARN("couldn't parse Content-Length header");
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
            LOG_ERROR("unsupported method \""
                      + NARROW(request_obj.method()) + "\"");
            send_response(cl_sock, logger.get(),
                          make_response(400, _T("Bad request"), bad_resp, cont_type_resp));
        }
        else {
            if (!has_handler_for_path(request_obj.path()))
            {
                LOG_ERROR("Path " + NARROW(request_obj.path()) + " not found!");
                send_response(cl_sock, logger.get(),
                              make_response(404, _T("Not found"), bad_resp, cont_type_resp));
            }
            else {
                // handle the request
                send_response(cl_sock, logger.get(), call_handler(request_obj));
            }
        }
    }
    catch (const SocketEx &ex) {
        LOG_ERROR(string("socket error: ") + ex.what());
        try {
            send_response(cl_sock, logger.get(),
                          make_response(400, _T("Short read"), bad_resp, cont_type_resp));
        }
        catch (const std::exception &ex2) {
            LOG_ERROR(string("unable to send: ") + ex2.what());
        }
    }
    catch (const HttpParserError &ex) {
        LOG_ERROR(string("parser error: ") + ex.what());
        try {
            send_response(cl_sock, logger.get(),
                          make_response(400, _T("Bad request"), bad_resp, cont_type_resp));
        }
        catch (const std::exception &ex2) {
            LOG_ERROR(string("unable to send: ") + ex2.what());
        }
    }
    catch (const std::exception &ex) {
        LOG_ERROR(string("exception: ") + ex.what());
        try {
            send_response(cl_sock, logger.get(),
                          make_response(500, _T("Internal server error"), bad_resp, cont_type_resp));
        }
        catch (const std::exception &ex2) {
            LOG_ERROR(string("unable to send: ") + ex2.what());
        }
    }
    cl_sock.close(true);
}

void
HttpServerBase::bind()
{
    if (is_bound_)
        return;
    TcpSocket::init_socket_lib();
    Yb::ILogger *logger = log_.get();
    LOG_INFO("start server on port " + to_stdstring(port_));
    sock_.bind(ip_addr_, port_);
    sock_.listen(back_log_);
    is_bound_ = true;
}

void
HttpServerBase::serve()
{
    bind();
    Yb::ILogger *logger = log_.get();
    while (1) {
        // accept request
        SOCKET cl_sock = INVALID_SOCKET;
        try {
            LOG_DEBUG("waiting for incoming connect...");
            string ip_addr;
            int ip_port;
            is_serving_ = true;
            cl_sock = sock_.accept(&ip_addr, &ip_port);
            LOG_INFO("accepted from " + ip_addr + ":" + to_stdstring(ip_port));
            worker_pool_.process_task(HttpWorkerTask(this, cl_sock));
        }
        catch (const std::exception &ex) {
            LOG_ERROR(string("exception: ") + ex.what());
            if (cl_sock != INVALID_SOCKET) {
                LOG_INFO("closing failed socket");
                TcpSocket s(cl_sock);
            }
        }
    }
}

// vim:ts=4:sts=4:sw=4:et:
