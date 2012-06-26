#include <memory>
#include <iostream>
#include <util/str_utils.hpp>
#include <orm/DataObject.h>
#include <orm/MetaDataSingleton.h>
#include "domain/Client.h"
#include "domain/Order.h"

using namespace std;
Yb::LogAppender appender(cerr);

const Yb::String
cfg(const Yb::String &key) { return Yb::StrUtils::xgetenv(_T("YBORM_") + key); }

int main()
{
    Yb::SqlSource src(_T("ex1_db"), _T("DEFAULT"), cfg(_T("DBTYPE")),
            cfg(_T("DB")), cfg(_T("USER")), cfg(_T("PASSWD")));
#if defined(YB_USE_QT)
    src.set_lowlevel_driver(_T("QODBC"));
#endif
    auto_ptr<Yb::SqlConnection> conn(new Yb::SqlConnection(src));
    Yb::Engine engine(Yb::Engine::MANUAL, conn);
    Yb::Session session(Yb::init_default_meta(), &engine);
    engine.set_echo(true);
    engine.set_logger(Yb::ILogger::Ptr(new Yb::Logger(&appender)));
    Domain::Client client(session);
    string name, email;
    cout << "Enter name, email: \n";
    cin >> name >> email;
    client.set_name(WIDEN(name));
    client.set_email(WIDEN(email));
    client.set_dt(Yb::now());
    Domain::Order order(session);
    string value;
    cout << "Enter order amount: \n";
    cin >> value;
    order.set_total_sum(Yb::Decimal(WIDEN(value)));
    order.set_owner(client);
    session.flush();
    cout << "client created: " << client.get_id() << endl;
    cout << "order created: " << order.get_id() << endl;
    engine.commit();
    cout << order.xmlize(1)->serialize() << endl;
    return 0;
}
// vim:ts=4:sts=4:sw=4:et:
