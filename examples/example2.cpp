#include <memory>
#include <iostream>
#include <util/str_utils.hpp>
#include <orm/Session.h>
#include <orm/SqlDataSource.h>
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
    string conf_dir = Yb::StrUtils::xgetenv("EX1_DIR");
    if (conf_dir.empty())
        conf_dir = ".";
    Yb::load_meta(conf_dir + "/ex1_schema.xml", Yb::theMetaData::instance());
    Domain::ClientRegistrator::register_domain();
    Domain::OrderRegistrator::register_domain();
#ifdef HAVE_DBPOOL3
    auto_ptr<Yb::DBPoolConfig> conf(
            new Yb::DBPoolConfig(conf_dir + "/dbpool.cfg.xml"));
    auto_ptr<Yb::SqlDriver> drv(
            new Yb::DBPoolDriver(conf, "MY_DBPOOL"));
    Yb::register_sql_driver(drv);
    auto_ptr<Yb::SqlConnect> conn(
            new Yb::SqlConnect("MY_DBPOOL", Yb::StrUtils::xgetenv("YBORM_DBTYPE"), "default"));
    Yb::Engine engine(Yb::Engine::MANUAL, conn);
#else
    Yb::Engine engine(Yb::Engine::MANUAL);
#endif
    Yb::Session session(Yb::theMetaData::instance(),
            auto_ptr<Yb::DataSource>(
                new Yb::SqlDataSource(Yb::theMetaData::instance(), engine)));
    engine.get_connect()->set_echo(true);
    Domain::Client client(session);
    string name, email;
    cout << "Enter name, email: \n";
    cin >> name >> email;
    client.set_name(name);
    client.set_email(email);
    client.set_dt(Yb::now());
    {
    Domain::Order order(session);
    string value;
    cout << "Enter order amount: \n";
    cin >> value;
    order.set_total_sum(Yb::Decimal(value));
    cout << "orders size: " << client.get_orders().size() << endl;
    order.set_owner(client);
    cout << "orders size: " << client.get_orders().size() << endl;
    }
    {
    Domain::Order order(session);
    string value;
    cout << "Enter order amount: \n";
    cin >> value;
    order.set_total_sum(Yb::Decimal(value));
    cout << "orders size: " << client.get_orders().size() << endl;
    order.set_owner(client);
    cout << "orders size: " << client.get_orders().size() << endl;
    session.flush();
    cout << "client created: " << client.get_id().as_longint() << endl;
    cout << "order created: " << order.get_id().as_longint() << endl;
    engine.commit();
    cout << order.xmlize(1).get_xml() << endl;
    }
    Domain::Client c2(session, client.get_id().as_longint());
    cout << "c2.orders size: " << c2.get_orders().size() << endl;
    Yb::ManagedList<Domain::Client> lst;
    lst.insert(client);
    Yb::ManagedList<Domain::Client>::iterator it = lst.begin(), end = lst.end();
    for (; it != end; ++it)
        cout << "client in list: " << it->get_id().as_longint() << endl;
    cout << "list size: " << lst.size() << endl;
    lst.erase(lst.begin());
    cout << "list size: " << lst.size() << endl;
    return 0;
}
// vim:ts=4:sts=4:sw=4:et:
