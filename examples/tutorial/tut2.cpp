#include <memory>
#include <iostream>
#include "domain/Client.h"
#include "domain/Order.h"
using namespace std;

int main()
{
    try {
        auto_ptr<Yb::SqlConnection> conn(new Yb::SqlConnection(
                    "sqlite+sqlite://c:/yborm/examples/test1_db"));
        Yb::Engine engine(Yb::Engine::READ_WRITE, conn);
        Yb::Session session(Yb::init_schema(), &engine);

        Domain::Order::Holder order;
        string amount;
        cout << "Enter order amount: \n";
        cin >> amount;
        order->total_sum = Yb::Decimal(amount);

        Domain::Client::Holder client;
        string name, email;
        cout << "Enter name, email: \n";
        cin >> name >> email;
        client->name = name;
        client->email = email;
        client->dt = Yb::now();

        cout << "Client's orders count: " << client->orders.size() << "\n";
        order->owner = client;
        cout << "Client's orders count: " << client->orders.size() << "\n";

        order->save(session);
        client->save(session);
        session.commit();
        cout << order->xmlize(1)->serialize() << endl;
        return 0;
    }
    catch (const std::exception &ex) {
        cout << "exception: " << ex.what() << endl;
        return 1;
    }
}
