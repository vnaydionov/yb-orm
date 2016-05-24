// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef _AUTH__XML_METHOD_H_
#define _AUTH__XML_METHOD_H_

#include <util/string_utils.h>
#include <util/element_tree.h>
#include <util/nlogger.h>
#include "http_message.h"

class TimerGuard
{
    Yb::ILogger &logger_;
    Yb::String method_;
    Yb::MilliSec t0_;
    bool except_;
public:
    TimerGuard(Yb::ILogger &logger, const Yb::String &method = _T("unknown"))
        : logger_(logger)
        , method_(method)
        , t0_(Yb::get_cur_time_millisec())
        , except_(true)
    {}
    void set_ok() { except_ = false; }
    void set_method(const Yb::String &method) { method_ = method; }
    Yb::MilliSec time_spent() const
    {
        return Yb::get_cur_time_millisec() - t0_;
    }
    ~TimerGuard() {
        Yb::String out = _T("Method ") + method_ + _T(" ended");
        if (except_)
            out += _T(" with an exception");
        out += _T(". Execution time=")
            + Yb::to_string(time_spent() / 1000.0);
        logger_.info(out);
    }
};

inline Yb::ElementTree::ElementPtr mk_resp(const Yb::String &status,
        const Yb::String &status_code = _T(""))
{
    Yb::ElementTree::ElementPtr res = Yb::ElementTree::new_element(_T("status"), status);
    return res;
}

#define OK_RESP mk_resp(_T("OK"))
#define NOT_RESP mk_resp(_T("NOT"))

class ApiResult: public std::runtime_error
{
    Yb::ElementTree::ElementPtr p_;
    int http_code_;
    Yb::String http_desc_;
public:
    explicit ApiResult(Yb::ElementTree::ElementPtr p,
                       int http_code = 200, const Yb::String &http_desc = _T("OK"))
        : runtime_error("api result")
        , p_(p)
        , http_code_(http_code)
        , http_desc_(http_desc)
    {}
    virtual ~ApiResult() throw () {}
    Yb::ElementTree::ElementPtr result() const { return p_; }
    int http_code() const { return http_code_; }
    const Yb::String &http_desc() const { return http_desc_; }
};

inline const Yb::String dict2str(const Yb::StringDict &params)
{
    using namespace Yb::StrUtils;
    Yb::String out;
    out += _T("{");
    Yb::StringDict::const_iterator it = params.begin(), end = params.end();
    for (bool first = true; it != end; ++it, first = false) {
        if (!first)
            out += _T(", ");
        out += it->first + _T(": ") + dquote(c_string_escape(it->second));
    }
    out += _T("}");
    return out;
}

typedef const HttpResponse (*PlainHttpHandler)(
        Yb::ILogger &logger, const HttpRequest &request);

typedef Yb::ElementTree::ElementPtr (*XmlHttpHandler)(
        Yb::Session &session, Yb::ILogger &logger,
        const Yb::StringDict &params);


class XmlHttpWrapper
{
    Yb::String name_, prefix_, default_status_;
    XmlHttpHandler f_;
    Yb::SharedPtr<Yb::ILogger>::Type logger_;

    const std::string dump_result(Yb::ElementTree::ElementPtr res)
    {
        std::string res_str = res->serialize();
        logger_->info(_T("result: ") + WIDEN(res_str));
        return res_str;
    }

    const HttpResponse try_call(TimerGuard &t,
                                const HttpRequest &request, int n)
    {
        logger_->info(_T("Method ") + name_ +
                      Yb::String(n? _T(" re"): _T(" ")) + _T("started."));
        const Yb::StringDict &params = request.params();
        logger_->debug(_T("Method ") + name_ +
                       _T(": params: ") + dict2str(params));
        // call xml wrapped
        std::auto_ptr<Yb::Session> session;
        if (theApp::instance().uses_db())
            session.reset(theApp::instance().new_session().release());
        Yb::ElementTree::ElementPtr res = f_(*session, *logger_, params);
        if (theApp::instance().uses_db())
            session->commit();
        // form the reply
        HttpResponse response(HTTP_1_0, 200, _T("OK"));
        response.set_response_body(dump_result(res), _T("application/xml"));
        t.set_ok();
        return response;
    }

public:
    XmlHttpWrapper(): f_(NULL) {}

    XmlHttpWrapper(const Yb::String &name, XmlHttpHandler f,
            const Yb::String &prefix = _T(""),
            const Yb::String &default_status = _T("NOT"))
        : name_(name)
        , prefix_(prefix)
        , default_status_(default_status)
        , f_(f)
    {}

    const Yb::String &name() const { return name_; }
    const Yb::String &prefix() const { return prefix_; }

    const HttpResponse operator() (const HttpRequest &request)
    {
        logger_.reset(theApp::instance().new_logger(
                    _T("invoker_xml")).release());
        const Yb::String cont_type = _T("application/xml");
        TimerGuard t(*logger_, name_);
        try {
            try {
                return try_call(t, request, 0);
            }
            catch (const Yb::DBError &ex) {
                logger_->error(_T("db exception: ") + ex.wtf());
                return try_call(t, request, 1);
            }
        }
        catch (const HttpHeaderNotFound &ex) {
            logger_->error(_T("Missing header: ") + ex.header_name());
            HttpResponse response(HTTP_1_0, 400, _T("Bad Request"));
            response.set_response_body(dump_result(mk_resp(_T("missing_header"))), cont_type);
            return response;
        }
        catch (const ApiResult &ex) {
            if (ex.http_code() == 200)
                t.set_ok();
            HttpResponse response(HTTP_1_0, 200, _T("OK"));
            response.set_response_body(dump_result(ex.result()), cont_type);
            return response;
        }
        catch (const std::exception &ex) {
            logger_->error(Yb::String(_T("exception: ")) + WIDEN(ex.what()));
            HttpResponse response(HTTP_1_0, 500, _T("Internal error"));
            response.set_response_body(dump_result(mk_resp(default_status_)), cont_type);
            return response;
        }
        catch (...) {
            logger_->error(_T("unknown exception"));
            HttpResponse response(HTTP_1_0, 500, _T("Internal error"));
            response.set_response_body(dump_result(mk_resp(default_status_)), cont_type);
            return response;
        }
    }
};

#define WRAP(func) XmlHttpWrapper(_T(#func), func, _T(""))
#define PWRAP(prefix, func) XmlHttpWrapper(_T(#func), func, prefix)

#endif // _AUTH__XML_METHOD_H_
// vim:ts=4:sts=4:sw=4:et:
