#include <util/util_config.h>
#if defined(YBUTIL_WINDOWS)
#include <rpc.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif
#if defined(YB_USE_WX)
#include <wx/app.h>
#elif defined(YB_USE_QT)
#include <QCoreApplication>
#endif
#include <iostream>
#include "md5.h"
#include "app_class.h"
#include "micro_http.h"
#include "xml_method.h"
#if 0
#include "domain/User.h"
#include "domain/LoginSession.h"
using namespace Domain;
#else
#include "model.h"
YB_DEFINE(User)
YB_DEFINE(LoginSession)
#endif

using namespace std;

Yb::LongInt
get_random()
{
    Yb::LongInt buf;
#if defined(__WIN32__) || defined(_WIN32)
    UUID new_uuid;
    UuidCreate(&new_uuid);
    buf = new_uuid.Data1;
    buf <<= 32;
    Yb::LongInt buf2 = (new_uuid.Data2 << 16) | new_uuid.Data3;
    buf += buf2;
#else
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1)
        throw std::runtime_error("can't open /dev/urandom");
    if (read(fd, &buf, sizeof(buf)) != sizeof(buf)) {
        close(fd);
        throw std::runtime_error("can't read from /dev/urandom");
    }
    close(fd);
#endif
    return buf;
}

Yb::LongInt
get_checked_session_by_token(Yb::Session &session,
        const Yb::StringDict &params, int admin_flag=0)
{
#if !defined(YB_USE_TUPLE)
    typedef LoginSession Row;
#else
    typedef boost::tuple<LoginSession, User> Row;
#endif
    Yb::DomainResultSet<Row> rs = Yb::query<Row>(session)
        .filter_by(LoginSession::c.token == params[_T("token")]).all();
    Yb::DomainResultSet<Row>::iterator q = rs.begin(), qend = rs.end();
    if (q == qend)
        return -1;
#if !defined(YB_USE_TUPLE)
    LoginSession &ls = *q;
#else
    LoginSession &ls = q->get<0>();
#endif
    if (admin_flag) {
        if (ls.user->status != 0)
            return -1;
    }
    if (Yb::now() >= ls.end_session)
        return -1;
    return ls.id;
}

Yb::ElementTree::ElementPtr
check(Yb::Session &session, Yb::ILogger &logger,
        const Yb::StringDict &params)
{
    return get_checked_session_by_token(session, params) == -1?
        NOT_RESP: OK_RESP;
}

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif // _MSC_VER

Yb::String
md5_hash(const Yb::String &str0)
{
    std::string str = NARROW(str0);
    MD5_CTX mdContext;
    MD5Init(&mdContext);
    MD5Update(&mdContext, (unsigned char *)str.c_str(), str.size());
    MD5Final(&mdContext);
    std::string rez;
    char omg[20];
    for (int i = 0; i < 16; ++i) {
        sprintf(omg, "%02x", mdContext.digest[i]);
        rez.append(omg, 2);
    }
    return WIDEN(rez);
}

Yb::ElementTree::ElementPtr
registration(Yb::Session &session, Yb::ILogger &logger,
        const Yb::StringDict &params)
{
    Yb::LongInt total_count = Yb::query<User>(session).count();
    if (total_count) {
        // when user table is empty it is allowed to create the first user
        // w/o password check, otherwise we should check permissions
        if (-1 == get_checked_session_by_token(session, params, 1))
            return NOT_RESP;
    }
    if (Yb::query<User>(session)
            .filter_by(User::c.login == params[_T("login")]).count())
        return NOT_RESP;

    User user(session);
    user.login = params[_T("login")];
    user.name = params[_T("name")];
    user.pass = md5_hash(params[_T("pass")]);
    user.status = params.get_as<int>(Yb::String(_T("status")));
    return OK_RESP;
}

Yb::LongInt
get_checked_user_by_creds(Yb::Session &session, const Yb::StringDict &params)
{
    User::ResultSet rs = Yb::query<User>(session,
        User::c.login == params[_T("login")]).all();
    User::ResultSet::iterator q = rs.begin(), qend = rs.end();
    if (q == qend)
        return -1;
    if (q->pass != md5_hash(params[_T("pass")]))
        return -1;
    return q->id;
}

Yb::ElementTree::ElementPtr
login(Yb::Session &session, Yb::ILogger &logger,
        const Yb::StringDict &params)
{
    Yb::LongInt uid = get_checked_user_by_creds(session, params);
    if (-1 == uid)
        return NOT_RESP;
    User::Holder user(session, uid);
    while (user->login_sessions.begin() != user->login_sessions.end())
        user->login_sessions.begin()->delete_object();
    LoginSession login_session(session);
    login_session.user = user;
    Yb::LongInt token = get_random();
    login_session.token = Yb::to_string(token);
    login_session.end_session = Yb::dt_add_seconds(Yb::now(), 11*60*60);
    login_session.app_name = _T("auth");
    Yb::ElementTree::ElementPtr root = Yb::ElementTree::new_element(
            _T("token"), login_session.token);
    return root;
}

Yb::ElementTree::ElementPtr
session_info(Yb::Session &session, Yb::ILogger &logger,
        const Yb::StringDict &params)
{
    Yb::LongInt sid = get_checked_session_by_token(session, params);
    if (-1 == sid)
        return NOT_RESP;
    LoginSession ls(session, sid);
    return ls.xmlize(1);
}

Yb::ElementTree::ElementPtr
logout(Yb::Session &session, Yb::ILogger &logger,
        const Yb::StringDict &params)
{
    Yb::LongInt sid = get_checked_session_by_token(session, params);
    if (-1 == sid)
        return NOT_RESP;
    LoginSession ls(session, sid);
    ls.delete_object();
    return OK_RESP;
}

#if defined(YB_USE_WX)
class MyApp : public wxAppConsole {
public:
    virtual bool OnInit() { return true; }
    virtual int OnRun()
#else
    int main(int argc, char *argv[])
#endif
{
#if defined(YB_USE_QT)
    QCoreApplication app(argc, argv);
#endif
    Yb::ILogger::Ptr log;
    try {
        theApp::instance().init("auth.log", "auth_db");
        log.reset(theApp::instance().new_logger("main").release());
    }
    catch (const std::exception &ex) {
        std::cerr << "exception: " << ex.what() << "\n";
        return 1;
    }
    try {
        XmlHttpWrapper handlers[] = {
            WRAP(session_info),
            WRAP(registration),
            WRAP(check),
            WRAP(login),
            WRAP(logout),
        };
        int n_handlers = sizeof(handlers)/sizeof(handlers[0]);
        int port = 9090; // TODO: read from config
        typedef HttpServer<XmlHttpWrapper> AuthHttpServer;
        typename AuthHttpServer::HandlerMap handler_map;
        for (int i = 0; i < n_handlers; ++i)
            handler_map[_T("/") + handlers[i].name()] = handlers[i];
        AuthHttpServer server("0.0.0.0", port, 3,
                handler_map, &theApp::instance(),
                _T("text/xml"), "<status>NOT</status>");
        server.serve();
    }
    catch (const std::exception &ex) {
        log->error(string("exception: ") + ex.what());
        return 1;
    }
    return 0;
}
#if defined(YB_USE_WX)
};
IMPLEMENT_APP(MyApp)
#endif

// vim:ts=4:sts=4:sw=4:et:
