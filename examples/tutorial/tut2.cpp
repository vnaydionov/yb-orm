#include <iostream>
#include "domain/Client.h"
#include "domain/Order.h"
int main()
{
    try {
        std::auto_ptr<Yb::SqlConnection> conn(new Yb::SqlConnection(
                "sqlite+sqlite://./test1_db"));
        Yb::Engine engine(Yb::Engine::READ_WRITE, conn);
        Yb::Session session(Yb::init_schema(), &engine);

        Domain::Order::Holder order;
        std::string amount;
        std::cout << "Enter order amount: \n";
        std::cin >> amount;
        order->total_sum = Yb::Decimal(amount);

        Domain::Client::Holder client;
        std::string name, email;
        std::cout << "Enter name, email: \n";
        std::cin >> name >> email;
        client->name = name;
        client->email = email;
        client->dt = Yb::now();

        std::cout << "Client's orders count: " << client->orders.size() << "\n";
        //order->owner = client;  // it works either way
        client->orders.insert(*order);
        std::cout << "Client's orders count: " << client->orders.size() << "\n";

        order->save(session);
        client->save(session);
        session.commit();
        std::cout << order->xmlize(1)->serialize() << std::endl;
    }
    catch (const std::exception &ex) {
        std::cerr << "exception: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
