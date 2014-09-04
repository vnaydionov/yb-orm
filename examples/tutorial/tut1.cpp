#include <iostream>
#include "domain/Client.h"
int main()
{
    try {
        std::auto_ptr<Yb::SqlConnection> conn(new Yb::SqlConnection(
                "sqlite+sqlite://./test1_db"));
        Yb::Engine engine(Yb::Engine::READ_WRITE, conn);
        Yb::init_schema();
        //engine.drop_schema(Yb::theSchema(), true);
        //engine.create_schema(Yb::theSchema(), false);
        Yb::Session session(Yb::theSchema(), &engine);

        Domain::Client client;
        std::string name, email, budget;
        std::cout << "Enter name, email, budget:\n";
        std::cin >> name >> email >> budget;
        client.name = name;
        client.email = email;
        client.budget = Yb::Decimal(budget);
        client.dt = Yb::now();
        client.save(session);
        session.flush();
        std::cout << "New client: " << client.id.value() << std::endl;
        engine.commit();
    }
    catch (const std::exception &ex) {
        std::cerr << "exception: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
