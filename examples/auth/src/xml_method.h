#ifndef _AUTH__XML_METHOD_H_
#define _AUTH__XML_METHOD_H_

#include <util/string_utils.h>
#include <util/element_tree.h>
#include "micro_http.h"

class TimerGuard
{
    Yb::ILogger &logger_;
    Yb::MilliSec t0_;
    bool except_;
public:
    TimerGuard(Yb::ILogger &logger)
        : logger_(logger), t0_(Yb::get_cur_time_millisec()), except_(true)
    {}
    void set_ok() { except_ = false; }
    Yb::MilliSec time_spent() const
    {
        return Yb::get_cur_time_millisec() - t0_;
    }
    ~TimerGuard() {
        std::ostringstream out;
        out << "finished";
        if (except_)
            out << " with an exception";
        out << ", " << time_spent() << " ms";
        logger_.info(out.str());
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
public:
    ApiResult(Yb::ElementTree::ElementPtr p)
        : runtime_error("api result"), p_(p)
    {}
    virtual ~ApiResult() throw () {}
    Yb::ElementTree::ElementPtr result() const { return p_; }
};

inline const std::string dict2str(const Yb::StringDict &params)
{
    using namespace Yb::StrUtils;
    std::ostringstream out;
    out << "{";
    Yb::StringDict::const_iterator it = params.begin(), end = params.end();
    for (bool first = true; it != end; ++it, first = false) {
        if (!first)
            out << ", ";
        out << NARROW(it->first) << ": " << NARROW(dquote(c_string_escape(it->second)));
    }
    out << "}";
    return out.str();
}

typedef const HttpMessage (*PlainHttpHandler)(
        Yb::ILogger &logger, const HttpMessage &request);

typedef Yb::ElementTree::ElementPtr (*XmlHttpHandler)(
        Yb::Session &session, Yb::ILogger &logger,
        const Yb::StringDict &params);

class XmlHttpWrapper
{
    Yb::String name_, default_status_;
    XmlHttpHandler f_;

    std::string dump_result(Yb::ILogger &logger, Yb::ElementTree::ElementPtr res)
    {
        std::string res_str = res->serialize();
        logger.info("result: " + res_str);
        return res_str;
    }

public:
    XmlHttpWrapper(): f_(NULL) {}

    XmlHttpWrapper(const Yb::String &name, XmlHttpHandler f,
            const Yb::String &default_status = _T("NOT"))
        : name_(name), default_status_(default_status), f_(f)
    {}

    const Yb::String &name() const { return name_; }

    const HttpMessage operator() (const HttpMessage &request)
    {
        Yb::ILogger::Ptr logger(theApp::instance().new_logger(NARROW(name_)));
        TimerGuard t(*logger);
        try {
            const Yb::StringDict &params = request.get_params();
            logger->info("started path: " + NARROW(request.get_path())
                         + ", params: " + dict2str(params));
            std::auto_ptr<Yb::Session> session(
                    theApp::instance().new_session());
            Yb::ElementTree::ElementPtr res = f_(*session, *logger, params);
            session->commit();
            HttpMessage response(10, 200, _T("OK"));
            response.set_response_body(dump_result(*logger, res), _T("text/xml"));
            t.set_ok();
            return response;
        }
        catch (const ApiResult &ex) {
            t.set_ok();
            HttpMessage response(10, 200, _T("OK"));
            response.set_response_body(dump_result(*logger, ex.result()), _T("text/xml"));
            return response;
        }
        catch (const std::exception &ex) {
            logger->error(std::string("exception: ") + ex.what());
            HttpMessage response(10, 500, _T("Internal error"));
            response.set_response_body(dump_result(*logger, mk_resp(default_status_)), _T("text/xml"));
            return response;
        }
        catch (...) {
            logger->error("unknown exception");
            HttpMessage response(10, 500, _T("Internal error"));
            response.set_response_body(dump_result(*logger, mk_resp(default_status_)), _T("text/xml"));
            return response;
        }
    }
};

#define WRAP(func) XmlHttpWrapper(_T(#func), func)

#endif // _AUTH__XML_METHOD_H_
// vim:ts=4:sts=4:sw=4:et:

