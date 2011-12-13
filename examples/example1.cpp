#include <memory>
#include <iostream>
#include <util/str_utils.hpp>
#include <orm/DataObject.h>
#include <orm/MetaDataSingleton.h>
#include <orm/XMLMetaDataConfig.h>
#include "domain/Client.h"
#include "domain/Order.h"
//#define HAVE_DBPOOL3
#ifdef HAVE_DBPOOL3
#include <orm/DBPoolDriver.h>
#endif

using namespace std;

int main()
{
    Yb::String conf_dir = Yb::StrUtils::xgetenv(_T("EX1_DIR"));
    if (conf_dir.empty())
        conf_dir = _T(".");
    Yb::load_meta(conf_dir + _T("/ex1_schema.xml"), Yb::theMetaData::instance());
#ifdef HAVE_DBPOOL3
    auto_ptr<Yb::DBPoolConfig> conf(
            new Yb::DBPoolConfig(conf_dir + _T("/dbpool.cfg.xml")));
    auto_ptr<Yb::SqlDriver> drv(
            new Yb::DBPoolDriver(conf, _T("MY_DBPOOL")));
    Yb::register_sql_driver(drv);
    auto_ptr<Yb::SqlConnect> conn(
            new Yb::SqlConnect(_T("MY_DBPOOL"), Yb::StrUtils::xgetenv(_T("YBORM_DBTYPE")), _T("default")));
    Yb::Engine engine(Yb::Engine::MANUAL, conn);
#else
    Yb::Engine engine(Yb::Engine::MANUAL);
#endif
    Yb::Session session(Yb::theMetaData::instance(), &engine);
    engine.get_connect()->set_echo(true);
    Domain::Client client(session);
    string name, email;
    cout << "Enter name, email: \n";
    cin >> name >> email;
    client.set_name(WIDEN(name));
    client.set_email(WIDEN(email));
    //client.set_dt(Yb::now());
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
    cout << order.xmlize(0).get_xml() << endl;
    return 0;
}
// vim:ts=4:sts=4:sw=4:et:
