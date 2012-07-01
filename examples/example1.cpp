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
    Domain::ClientHolder client(session);
    string name, email;
    cout << "Enter name, email: \n";
    cin >> name >> email;
    client->name = WIDEN(name);
    client->email = WIDEN(email);
    client->dt = Yb::now();
    Domain::OrderHolder order(session);
    string value;
    cout << "Enter order amount: \n";
    cin >> value;
    order->total_sum = Yb::Decimal(WIDEN(value));
    order->owner = client;
    order->dt = Yb::now();
    Domain::OrderHolder o2(order);
    session.flush();
    Yb::LongInt cid = client->id; 
    cout << "client created: " << cid << endl;
    cout << "order created: " << order->id.value() << endl;
    cout << "order dt: " << NARROW(order->dt.as<Yb::String>()) << endl;
    engine.commit();
    cout << order->xmlize(1)->serialize() << endl;
    return 0;
}
// vim:ts=4:sts=4:sw=4:et:
