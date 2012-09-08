#if defined(__WIN32__) || defined(_WIN32)
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
#include <util/str_utils.hpp>
#include <util/xmlnode.h>
#include "md5.h"
#include "App.h"
#include "micro_http.h"
#include "domain/User.h"
#include "domain/LoginSession.h"

using namespace std;
using namespace Domain;

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
        .filter_by(Yb::filter_eq(_T("TOKEN"), params[_T("token")]))
#if defined(YB_USE_TUPLE)
        .filter_by(Yb::Filter(_T("USER_ID = T_USER.ID")))
#endif
        .all();
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

std::string 
check(const Yb::StringDict &params)
{
    return get_checked_session_by_token(*theApp::instance().new_session(),
            params) == -1? BAD_RESP: OK_RESP;
}

Yb::String 
md5_hash(const Yb::String &str0)
{
    std::string str = NARROW(str0);
    MD5_CTX mdContext;
    MD5Init(&mdContext);
    MD5Update(&mdContext, (unsigned char *)str.c_str(), str.size());
    MD5Final(&mdContext);
    std::string rez;
    char omg[3];
    for (int i = 0; i < 16; ++i) {
        sprintf(omg, "%02x", mdContext.digest[i]);
        rez.append(omg, 2);
    }
    return WIDEN(rez);
}

std::string 
registration(const Yb::StringDict &params)
{
    auto_ptr<Yb::Session> session = theApp::instance().new_session();
    User::ResultSet all = Yb::query<User>(*session).all();
    User::ResultSet::iterator p = all.begin(), pend = all.end();
    if (p != pend) {
        // when user table is empty it is allowed to create the first user
        // w/o password check, otherwise we should check permissions
        if (-1 == get_checked_session_by_token(*session, params, 1))
            return BAD_RESP;
    }
    User::ResultSet rs = Yb::query<User>(*session)
        .filter_by(Yb::filter_eq(_T("LOGIN"), params[_T("login")])).all();
    User::ResultSet::iterator q = rs.begin(), qend = rs.end();
    if (q != qend)
        return BAD_RESP;

    User user(*session);
    user.login = params[_T("login")];
    user.name = params[_T("name")];
    user.pass = md5_hash(params[_T("pass")]);
    user.status = params.get_as<int>(_T("status"));
    session->commit();
    return OK_RESP;
}

Yb::LongInt
get_checked_user_by_creds(Yb::Session &session, const Yb::StringDict &params)
{
    User::ResultSet rs = Yb::query<User>(session,
        Yb::filter_eq(_T("LOGIN"), params[_T("login")])).all();
    User::ResultSet::iterator q = rs.begin(), qend = rs.end();
    if (q == qend)
        return -1;
    if (q->pass != md5_hash(params[_T("pass")]))
        return -1;
    return q->id;
}

std::string 
login(const Yb::StringDict &params)
{
    auto_ptr<Yb::Session> session = theApp::instance().new_session();
    Yb::LongInt uid = get_checked_user_by_creds(*session, params);
    if (-1 == uid)
        return BAD_RESP;

    UserHolder user(*session, uid);
    while (user->login_sessions.begin() != user->login_sessions.end())
        user->login_sessions.begin()->delete_object();
    LoginSession login_session(*session);
    login_session.user = user;
    Yb::LongInt token = get_random();
    login_session.token = Yb::to_string(token);
    login_session.end_session = Yb::dt_add_seconds(Yb::now(), 11*60*60);
    login_session.app_name = _T("auth");
    Yb::ElementTree::ElementPtr root = Yb::ElementTree::new_element(
            _T("token"), login_session.token);
    session->commit();
    return root->serialize();
}

std::string
session_info(const Yb::StringDict &params)
{ 
    auto_ptr<Yb::Session> session = theApp::instance().new_session();
    Yb::LongInt sid = get_checked_session_by_token(*session, params);
    if (-1 == sid)
        return BAD_RESP;
    LoginSession ls(*session, sid);
    return ls.xmlize(1)->serialize();
}

std::string
logout(const Yb::StringDict &params)
{
    auto_ptr<Yb::Session> session = theApp::instance().new_session();
    Yb::LongInt sid = get_checked_session_by_token(*session, params);
    if (-1 == sid)
        return BAD_RESP;
    LoginSession ls(*session, sid);
    ls.delete_object();
    session->commit();
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
        int port = 9090; // TODO: read from config
        HttpHandlerMap handlers;
        handlers[_T("/session_info")] = session_info;
        handlers[_T("/registration")] = registration;
        handlers[_T("/check")] = check;
        handlers[_T("/login")] = login;
        handlers[_T("/logout")] = logout;
        HttpServer server(port, handlers, &theApp::instance());
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
