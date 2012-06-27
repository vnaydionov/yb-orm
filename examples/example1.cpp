#include <memory>
#include <iostream>
#include <util/str_utils.hpp>
#include <orm/DataObject.h>
#include <orm/MetaDataSingleton.h>
#include "domain/Client.h"
#include "domain/Order.h"

using namespace std;
Yb::LogAppender appender(cerr);

int main()
{
    Yb::Engine engine(Yb::Engine::MANUAL);
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
