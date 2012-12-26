#include <memory>
#include <iostream>
#include <orm/MetaDataSingleton.h>
#include "domain/Client.h"
using namespace std;

int main()
{
    try {
        auto_ptr<Yb::SqlConnection> conn(new Yb::SqlConnection(
                    "sqlite+sqlite:///Users/User/work/test1.db"));
        Yb::Engine engine(Yb::Engine::READ_WRITE, conn);
        Yb::Session session(Yb::init_default_meta(), &engine);

        Domain::Client client;
        string name, email, budget;
        cout << "Enter name, email, budget:\n";
        cin >> name >> email >> budget;
        client.name = name;
        client.email = email;
        client.budget = Yb::Decimal(budget);
        client.dt = Yb::now();
        client.save(session);
        session.flush();
        cout << "New client: " << client.id.value() << endl;
        engine.commit();
        return 0;
    }
    catch (const std::exception &ex) {
        cout << "exception: " << ex.what() << endl;
        return 1;
    }
}
 
