#include <memory>
#include <iostream>
#include <util/str_utils.hpp>
#include <orm/DataObject.h>
#include <orm/SchemaSingleton.h>
#include <orm/XMLMetaDataConfig.h>
#include "domain/Client.h"
#include "domain/Order.h"
#include "domain/Payment.h"
#include "domain/CCardPayment.h"
//#define HAVE_DBPOOL3
#ifdef HAVE_DBPOOL3
#include <orm/DBPoolDriver.h>
#endif

using namespace std;

int main()
{
    try {
        Yb::LogAppender appender(cerr);
        Yb::Logger root_logger(&appender);
        Yb::String conf_dir = Yb::StrUtils::xgetenv(_T("EX1_DIR"));
        if (Yb::str_empty(conf_dir))
            conf_dir = _T(".");
#if 0
        Yb::load_schema(conf_dir + _T("/ex1_schema.xml"), Yb::theSchema::instance());
#else
        Yb::init_schema();
#endif
#ifdef HAVE_DBPOOL3
        auto_ptr<Yb::DBPoolConfig> conf(
                new Yb::DBPoolConfig(conf_dir + _T("/dbpool.cfg.xml")));
        auto_ptr<Yb::SqlDriver> drv(
                new Yb::DBPoolDriver(conf, _T("MY_DBPOOL")));
        Yb::register_sql_driver(drv);
        auto_ptr<Yb::SqlConnection> conn(
                new Yb::SqlConnection(_T("MY_DBPOOL"),
                    Yb::env_cfg(_T("DBTYPE")), _T("default")));
        Yb::Engine engine(Yb::Engine::READ_WRITE, conn);
#else
        Yb::Engine engine;
#endif
        engine.set_echo(true);
        engine.set_logger(root_logger.new_logger("yb"));
        Yb::Session session(Yb::theSchema::instance(), &engine);
        Domain::Client::Holder client(session);
        string name, email;
        cout << "Enter name, email: \n";
        cin >> name >> email;
        client->name = WIDEN(name);
        client->email = WIDEN(email);
        client->dt = Yb::now();
        Domain::Client::Holder client_x(session);
        client_x->name = WIDEN(name + "x");
        client_x->email = WIDEN(email);
        client_x->dt = Yb::now();
        {
            Domain::Order order(session);
            string value;
            cout << "Enter order amount: \n";
            cin >> value;
            order.total_sum = Yb::Decimal(WIDEN(value));
            cout << "orders size: " << client->orders.size() << endl;
            order.owner = client;
            cout << "orders size: " << client->orders.size() << endl;
        }
        {
            Domain::Order order(session);
            string value;
            cout << "Enter order amount: \n";
            cin >> value;
            order.total_sum = Yb::Decimal(WIDEN(value));
            cout << "orders size: " << client->orders.size() << endl;
            order.owner = client;
            cout << "orders size: " << client->orders.size() << endl;
            session.flush();
            cout << "client created: " << client->id.value() << endl;
            cout << "order created: " << order.id.value() << endl;
            engine.commit();
            order.owner = client_x;
            cout << order.xmlize(1)->serialize() << endl;
            session.flush();
            cout << order.xmlize(1)->serialize() << endl;
            engine.commit();
        }
        
        Domain::Client::Holder c2(session, client->id.value());
        cout << "c2.orders size: " << c2->orders.size() << endl;
        Yb::ManagedList<Domain::Order> &lst = c2->orders;
        Yb::ManagedList<Domain::Order>::iterator it = lst.begin(), end = lst.end();
        for (; it != end; ++it)
            cout << "order in list: " << it->id.value() << endl;
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
                    c2->id == Client::c.id
            ).order_by(ExpressionList(Client::c.id, Order::c.id)).all();
        DomainResultSet<boost::tuple<Order, Client> >
            ::iterator p = rs0.begin(), pend = rs0.end();
        cout << "Order/Client IDs: " << endl; 
        for (; p != pend; ++p)
            cout << p->get<0>().id.value() << "/" 
                << p->get<1>().id.value() << ",";
        cout << endl;
#endif

        Domain::Order::ListPtr olist = Domain::Order::find(session,
                Order::c.client_id == c2->id);
        cout << "count(*): " << Yb::query<Domain::Order>(
                session, Order::c.client_id == c2->id).count();
        Yb::DomainResultSet<Domain::Order> rs =
            Yb::query<Domain::Order>(session, Order::c.client_id == c2->id).all();
        Yb::DomainResultSet<Domain::Order>::iterator q = rs.begin(), qend = rs.end();
        cout << "Order/Client IDs: " << endl; 
        for (; q != qend; ++q)
            cout << q->id.value() << ",";
        cout << endl;

        Domain::Payment::Holder pm(session);
        pm->amount = Yb::Decimal(100);
        pm->paysys_code = _T("CCARD");
        Domain::CCardPayment::Holder ccpm(session);
        // It works either way:
#if 1
        ccpm->payment = pm;
#else
        pm->ccard_payment = ccpm;
#endif
        session.flush();
        pm->ccard_payment->card_number = _T("3454****5676");
        session.flush();

        c2->delete_object();
        session.flush();
        engine.commit();
        return 0;
    }
    catch (const std::exception &ex) {
        cout << "exception: " << ex.what() << endl;
        return 1;
    }
}
// vim:ts=4:sts=4:sw=4:et:
