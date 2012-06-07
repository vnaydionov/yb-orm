#include <memory>
#include <iostream>
#include <orm/MetaDataSingleton.h>
#include "domain/Client.h"
#include "domain/Order.h"
using namespace std;

int main()
{
    auto_ptr<Yb::SqlConnect> conn(
        new Yb::SqlConnect("ODBC", "MYSQL", "test1_db", "test1", "test1_pwd"));
    Yb::Engine engine(Yb::Engine::MANUAL, conn);
    Yb::Session session(Yb::init_default_meta(), &engine);

    Domain::Order order;
    string amount;
    cout << "Enter order amount: \n";
    cin >> amount;
    order.set_total_sum(Yb::Decimal(amount));
    order.save(session);

    Domain::Client client;
    string name, email;
    cout << "Enter name, email: \n";
    cin >> name >> email;
    client.set_name(name);
    client.set_email(email);
    client.set_dt(Yb::now());
    client.save(session);

    order.set_owner(client);
    session.flush();
    engine.commit();
    cout << order.xmlize(1).get_xml() << endl;
    return 0;
}
