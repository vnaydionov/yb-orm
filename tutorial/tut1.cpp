#include <memory>
#include <iostream>
#include <orm/MetaDataSingleton.h>
#include "domain/Client.h"
using namespace std;

int main()
{
    try {
        auto_ptr<Yb::SqlConnection> conn(
            new Yb::SqlConnection("ODBC",
                "MYSQL", "test1_db", "test1", "test1_pwd"));
        Yb::Engine engine(Yb::Engine::MANUAL, conn);
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
 
