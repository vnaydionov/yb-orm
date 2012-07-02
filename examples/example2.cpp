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
Yb::LogAppender appender(cerr);

int main()
{
    Yb::String conf_dir = Yb::StrUtils::xgetenv(_T("EX1_DIR"));
    if (Yb::str_empty(conf_dir))
        conf_dir = _T(".");
#if 0
    Yb::load_meta(conf_dir + _T("/ex1_schema.xml"), Yb::theMetaData::instance());
#else
    Yb::init_default_meta();
#endif
#ifdef HAVE_DBPOOL3
    auto_ptr<Yb::DBPoolConfig> conf(
            new Yb::DBPoolConfig(conf_dir + _T("/dbpool.cfg.xml")));
    auto_ptr<Yb::SqlDriver> drv(
            new Yb::DBPoolDriver(conf, _T("MY_DBPOOL")));
    Yb::register_sql_driver(drv);
    auto_ptr<Yb::SqlConnection> conn(
            new Yb::SqlConnection(_T("MY_DBPOOL"),
                Yb::StrUtils::xgetenv(_T("YBORM_DBTYPE")), _T("default")));
    Yb::Engine engine(Yb::Engine::MANUAL, conn);
#else
    Yb::Engine engine(Yb::Engine::MANUAL);
#endif
    Yb::Session session(Yb::theMetaData::instance(), &engine);
    engine.set_echo(true);
    engine.set_logger(Yb::ILogger::Ptr(new Yb::Logger(&appender)));
    Domain::ClientHolder client(session);
    string name, email;
    cout << "Enter name, email: \n";
    cin >> name >> email;
    client->name = WIDEN(name);
    client->email = WIDEN(email);
    client->dt = Yb::now();
    Domain::ClientHolder client_x(session);
    client_x->name = WIDEN(name + "x");
    client_x->email = WIDEN(email);
    client_x->dt = Yb::now();
    {
    Domain::Order order(session);
    string value;
    cout << "Enter order amount: \n";
    cin >> value;
    order.set_total_sum(Yb::Decimal(WIDEN(value)));
    cout << "orders size: " << client->orders.size() << endl;
    order.owner = client;
    cout << "orders size: " << client->orders.size() << endl;
    }
    {
    Domain::Order order(session);
    string value;
    cout << "Enter order amount: \n";
    cin >> value;
    order.set_total_sum(Yb::Decimal(WIDEN(value)));
    cout << "orders size: " << client->orders.size() << endl;
    order.owner = client;
    cout << "orders size: " << client->orders.size() << endl;
    session.flush();
    cout << "client created: " << client->id.value() << endl;
    cout << "order created: " << order.get_id() << endl;
    engine.commit();
    order.owner = client_x;
    cout << order.xmlize(1)->serialize() << endl;
    session.flush();
    cout << order.xmlize(1)->serialize() << endl;
    engine.commit();
    }
    
    Domain::ClientHolder c2(session, client->id.value());
    cout << "c2.orders size: " << c2->orders.size() << endl;
    Yb::ManagedList<Domain::Order> &lst = c2->orders;
    Yb::ManagedList<Domain::Order>::iterator it = lst.begin(), end = lst.end();
    for (; it != end; ++it)
        cout << "order in list: " << it->get_id() << endl;
    cout << "list size: " << lst.size() << endl;
    //lst.erase(lst.begin());
    cout << "list size: " << lst.size() << endl;
    session.flush();
    engine.commit();
    using namespace Domain;
    using namespace Yb;

#if defined(YB_USE_TUPLE)
    DomainResultSet<boost::tuple<Order, Client> > rs0 =
        query<boost::tuple<Order, Client> >(session).filter_by(
            filter_eq(_T("T_CLIENT.ID"), c2->id.value()) &&
            Filter(_T("T_ORDER.CLIENT_ID = T_CLIENT.ID"))
        ).order_by(_T("T_CLIENT.ID, T_ORDER.ID")).all();
    DomainResultSet<boost::tuple<Order, Client> >
        ::iterator p = rs0.begin(), pend = rs0.end();
    cout << "Order/Client IDs: " << endl; 
    for (; p != pend; ++p)
        cout << p->get<0>().id.value() << "/" 
            << p->get<1>().id.value() << ",";
    cout << endl;
#endif

    Domain::Order::ListPtr olist = Domain::Order::find(
        session, Yb::filter_eq(_T("T_ORDER.CLIENT_ID"), c2->id.value()));
    Yb::DomainResultSet<Domain::Order> rs =
        Yb::query<Domain::Order>(session,
            Yb::filter_eq(_T("T_ORDER.CLIENT_ID"), c2->id)).all();
    Yb::DomainResultSet<Domain::Order>::iterator q = rs.begin(), qend = rs.end();
    cout << "Order/Client IDs: " << endl; 
    for (; q != qend; ++q)
        cout << q->get_id() << ",";
    cout << endl;
    c2->delete_object();
    session.flush();
    engine.commit();
    return 0;
}
// vim:ts=4:sts=4:sw=4:et:
