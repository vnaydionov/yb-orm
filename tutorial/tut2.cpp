#include <memory>
#include <iostream>
#include <orm/MetaDataSingleton.h>
#include "domain/Client.h"
#include "domain/Order.h"
using namespace std;

int main()
{
    try {
        auto_ptr<Yb::SqlConnection> conn(
            new Yb::SqlConnection("ODBC",
                "MYSQL", "test1_db", "test1", "test1_pwd"));
        Yb::Engine engine(Yb::Engine::MANUAL, conn);
        Yb::Session session(Yb::init_default_meta(), &engine);

        Domain::Order order;
        string amount;
        cout << "Enter order amount: \n";
        cin >> amount;
        order.total_sum = Yb::Decimal(amount);
        order.save(session);

        Domain::ClientHolder client;
        string name, email;
        cout << "Enter name, email: \n";
        cin >> name >> email;
        client->name = name;
        client->email = email;
        client->dt = Yb::now();
        client->save(session);

        cout << "Client's orders count: " << client->orders.size() << "\n";
        order.owner = client;
        cout << "Client's orders count: " << client->orders.size() << "\n";
        session.commit();
        cout << order.xmlize(1)->serialize() << endl;
        return 0;
    }
    catch (const std::exception &ex) {
        cout << "exception: " << ex.what() << endl;
        return 1;
    }
}
