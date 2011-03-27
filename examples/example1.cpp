#include <iostream>
#include <orm/MapperSession.h>
#include <orm/MetaDataSingleton.h>
#include <orm/XMLMetaDataConfig.h>
#include "domain/Client.h"
#include "domain/Order.h"

using namespace std;

int main()
{
    string xml;
    Yb::load_xml_file("./DBScheme.xml", xml);
    Yb::XMLMetaDataConfig xml_config(xml);
    xml_config.parse(Yb::theMetaData::instance());
    Yb::MapperSession session(false);
    Domain::Client client(session);
    string name, email;
    cout << "Enter name, email: \n";
    cin >> name >> email;
    client.set_name(name);
    client.set_email(email);
    client.set_dt(Yb::now());
    Domain::Order order(session);
    string value;
    cout << "Enter order amount: \n";
    cin >> value;
    order.set_total_sum(Yb::Decimal(value));
    order.set_client(client);
    session.flush();
    cout << "client created: " << client.get_id().as_longint() << endl;
    cout << "order created: " << order.get_id().as_longint() << endl;
    session.commit();
    return 0;
}

