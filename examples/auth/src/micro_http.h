#ifndef _AUTH__MICRO_HTTP_H_
#define _AUTH__MICRO_HTTP_H_

#include <util/data_types.h>
#include <util/nlogger.h>
#include "http_message.h"
#include "tcp_socket.h"
#include "thread_pool.h"

class HttpServerBase;

struct HttpWorkerTask {
    HttpServerBase *server;
    SOCKET cl_s;
    HttpWorkerTask(HttpServerBase *srv=NULL, SOCKET x=INVALID_SOCKET)
        : server(srv)
        , cl_s(x)
    {}
    bool is_empty() const { return cl_s == INVALID_SOCKET; }
    static const HttpWorkerTask empty_task() { return HttpWorkerTask(); }
};

class HttpWorkerThread: public WorkerThread<HttpWorkerTask> {
public:
    HttpWorkerThread(WorkerTaskQueue<HttpWorkerTask> &task_queue)
        : WorkerThread<HttpWorkerTask>(task_queue)
    {}

    virtual void process_task(const HttpWorkerTask &task);
};

class HttpServerBase
{
public:
    HttpServerBase(const std::string &ip_addr, int port, int back_log,
            Yb::ILogger *root_logger,
            const Yb::String &content_type, const std::string &bad_resp,
            int thread_pool_size);
    void bind();
    void serve();
    bool is_bound() const { return is_bound_; }
    bool is_serving() const { return is_serving_; }

    static void process(HttpServerBase *server, SOCKET cl_s);
    void process_client_request(SOCKET cl_s);

protected:
    virtual bool has_handler_for_path(const Yb::String &path) = 0;
    virtual const HttpResponse call_handler(const HttpRequest &request) = 0;

private:
    bool is_bound_;
    bool is_serving_;
    std::string ip_addr_;
    int port_;
    int back_log_;
    Yb::String content_type_;
    std::string bad_resp_;
    Yb::ILogger::Ptr log_;
    TcpSocket sock_;
    WorkerThreadPool<HttpWorkerThread> worker_pool_;

    static HttpResponse make_response(int code, const Yb::String &desc,
                                      const std::string &body,
                                      const Yb::String &cont_type);
    static bool send_response(TcpSocket &cl_sock, Yb::ILogger *logger,
                              const HttpResponse &response);
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
            const std::string &bad_resp = "<status>NOT</status>",
            int thread_pool_size = 10):
        HttpServerBase(ip_addr, port, back_log, root_logger,
                       content_type, bad_resp, thread_pool_size),
        handlers_(handlers)
    {}
protected:
    virtual bool has_handler_for_path(const Yb::String &path)
    {
        return handlers_.has(path);
    }
    virtual const HttpResponse call_handler(const HttpRequest &request)
    {
        Handler func_ptr = handlers_.get(request.path());
        return func_ptr(request);
    }
private:
    const HandlerMap handlers_;
};

#endif // _AUTH__MICRO_HTTP_H_
// vim:ts=4:sts=4:sw=4:et:
