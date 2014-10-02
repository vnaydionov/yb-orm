#ifndef _AUTH__MODEL_H_
#define _AUTH__MODEL_H_

#include "orm/domain_object.h"
#include "orm/domain_factory.h"
#include "orm/schema_decl.h"

class LoginSession;

class User: public Yb::DomainObject {

YB_DECLARE(User, "T_USER", "S_USER", "user",
    YB_COL_PK(id, "ID")
    YB_COL_STR(name, "NAME", 100)
    YB_COL_STR(login, "LOGIN", 100)
    YB_COL_STR(email, "EMAIL", 100)
    YB_COL_STR(phone, "PHONE", 100)
    YB_COL(pass, "PASS", STRING, 100, 0,
           Yb::Value(_T("")), "", "", "!", "")
    YB_COL(status, "STATUS", INTEGER, 0, 0,
           1, "", "", "", "")
    YB_REL_ONE(User, user, LoginSession, login_sessions,
               Yb::Relation::Restrict, "USER_ID", 1, "")
    YB_COL_END)

};

class LoginSession: public Yb::DomainObject {

YB_DECLARE(LoginSession, "T_SESSION", "S_SESSION", "login-session",
    YB_COL_PK(id, "ID")
    YB_COL_FK(user_id, "USER_ID", "T_USER", "ID")
    YB_COL_STR(app_name, "APP_NAME", 100)
    YB_COL(begin_session, "BEGIN_SESSION", DATETIME, 0, 0,
           Yb::Value(_T("sysdate")), "", "", "", "")
    YB_COL_DATA(end_session, "END_SESSION", DATETIME)
    YB_COL_STR(token, "TOKEN", 100)
    YB_REL_MANY(User, user, LoginSession, login_sessions,
                Yb::Relation::Restrict, "USER_ID", 1, "")
    YB_COL_END)

};

#endif // _AUTH__MODEL_H_
// vim:ts=4:sts=4:sw=4:et:
