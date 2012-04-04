#if defined(__WIN32__) || defined(_WIN32)
#include <rpc.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif
#include <iostream>
#include <util/str_utils.hpp>
#include <orm/SqlPool.h>
#include <orm/MetaDataSingleton.h>
#include "md5.h"
#include "logger.h"
#include "micro_http.h"
#include "domain/User.h"
#include "domain/LoginSession.h"

using namespace std;
using namespace Domain;

std::auto_ptr<Yb::SqlPool> g_pool;

#define INIT_SESSION \
    Yb::Logger::Ptr ormlog = g_log->new_logger("orm"); \
    Yb::Engine engine(Yb::Engine::MANUAL, *g_pool, "auth_db"); \
    Yb::Session session(Yb::theMetaData::instance(), &engine); \
    engine.get_connect()->set_logger(ormlog.get()); \
    engine.get_connect()->set_echo(true);

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
get_checked_session_by_token(Yb::Session &session, StringMap &params, int admin_flag=0)
{
    LoginSession::ResultSet rs = Yb::query<LoginSession>(
            session, Yb::filter_eq(_T("TOKEN"), params["token"]));
    LoginSession::ResultSet::iterator q = rs.begin(), qend=rs.end();
    if (q == qend)
        return -1;
    if (admin_flag) {
        if (q->get_user().get_status() != 0)
            return -1;
    }
    if (Yb::now() >= q->get_end_session())
        return -1;
    return q->get_id();
}

string 
check(StringMap &params)
{
    INIT_SESSION;
    return get_checked_session_by_token(session, params) == -1? BAD_RESP: OK_RESP;
}

string 
md5_hash(const string &str)
{
    MD5_CTX mdContext;
    MD5Init(&mdContext);
    MD5Update(&mdContext, (unsigned char *)str.c_str(), str.size());
    MD5Final(&mdContext);
    string rez;
    char omg[3];
    for (int i = 0; i < 16; ++i) {
        sprintf(omg, "%02x", mdContext.digest[i]);
        rez.append(omg, 2);
    }
    return rez;
}

string 
registration(StringMap &params)
{
    INIT_SESSION;
    User::ResultSet all = Yb::query<User>(
            session, Yb::Filter(_T("1=1"))); // query all
    User::ResultSet::iterator p = all.begin(), pend = all.end();
    if (p != pend) {
        // when user table is empty it is allowed to create the first user
        // w/o password check, otherwise we should check permissions
        if (-1 == get_checked_session_by_token(session, params, 1))
            return BAD_RESP;
    }
    User::ResultSet rs = Yb::query<User>(
            session, Yb::filter_eq(_T("LOGIN"), params["login"]));
    User::ResultSet::iterator q = rs.begin(), qend = rs.end();
    if (q != qend)
        return BAD_RESP;

    User user(session);
    user.set_login(params["login"]);
    user.set_name(params["name"]);
    user.set_pass(md5_hash(params["pass"]));
    user.set_status(boost::lexical_cast<int>(params["status"]));
    session.flush();
    engine.commit();
    return OK_RESP;
}

Yb::LongInt
get_checked_user_by_creds(Yb::Session &session, StringMap &params)
{
    User::ResultSet rs = Yb::query<User>(
            session, Yb::filter_eq(_T("LOGIN"), params["login"]));
    User::ResultSet::iterator q = rs.begin(), qend = rs.end();
    if (q == qend)
        return -1;
    if (q->get_pass() != md5_hash(params["pass"]))
        return -1;
    return q->get_id();
}

string 
login(StringMap &params)
{
    INIT_SESSION;
    Yb::LongInt uid = get_checked_user_by_creds(session, params);
    if (-1 == uid)
        return BAD_RESP;

    User user(session, uid);
    while (user.get_login_sessions().begin() != user.get_login_sessions().end())
        user.get_login_sessions().begin()->delete_object();
    LoginSession login_session(session);
    login_session.set_user(user);
    Yb::LongInt token = get_random();
    login_session.set_token(boost::lexical_cast<string>(token));
    login_session.set_end_session(Yb::now() + boost::posix_time::hours(11));
    login_session.set_app_name("auth");
    session.flush();
    engine.commit();

    stringstream ss;
    ss << "<token>" << token << "</token>";
    return ss.str();
}

string 
session_info(StringMap &params)
{ 
    INIT_SESSION;
    Yb::LongInt sid = get_checked_session_by_token(session, params);
    if (-1 == sid)
        return BAD_RESP;
    LoginSession ls(session, sid);
    return ls.xmlize(1)->serialize();
}

string 
logout(StringMap &params)
{
    INIT_SESSION;
    Yb::LongInt sid = get_checked_session_by_token(session, params);
    if (-1 == sid)
        return BAD_RESP;
    LoginSession ls(session, sid);
    ls.delete_object();
    session.flush();
    engine.commit();
    return OK_RESP;
}

const string
cfg(const string &key) { return Yb::StrUtils::xgetenv("YBORM_" + key); }

int
main()
{
    try {
        const char *log_fname = "log.txt"; // TODO: read from config
        g_log.reset(new Log(log_fname));
        g_log->info("start log");
    }
    catch (const std::exception &ex) {
        std::cerr << "exception: " << ex.what() << "\n";
        std::cerr << "Can't open log file! Stop!\n";
        return 1;
    }
    try {
        Yb::init_default_meta();
        g_pool.reset(new Yb::SqlPool(YB_POOL_MAX_SIZE,
                YB_POOL_IDLE_TIME, YB_POOL_MONITOR_SLEEP, g_log.get()));
        g_pool->add_source(Yb::SqlSource("auth_db", "ODBC",
                    cfg("DBTYPE"), cfg("DB"), cfg("USER"), cfg("PASSWD")));
        int port = 9090; // TODO: read from config
        HttpHandlerMap handlers;
        handlers["/session_info"] = session_info;
        handlers["/registration"] = registration;
        handlers["/check"] = check;
        handlers["/login"] = login;
        handlers["/logout"] = logout;
        HttpServer server(port, handlers);
        server.serve();
    }
    catch (const std::exception &ex) {
        g_log->error(string("exception: ") + ex.what());
        return 1;
    }
    return 0;
}
// vim:ts=4:sts=4:sw=4:et:
