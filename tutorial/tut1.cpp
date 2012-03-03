#include <memory>
#include <iostream>
#include <orm/MetaDataSingleton.h>
#include "domain/Client.h"
using namespace std;

int main()
{
    try {
        auto_ptr<Yb::SqlConnect> conn(
            new Yb::SqlConnect("ODBC", "INTERBASE", "test1_db_fb", "test1", "test1_pwd"));
        Yb::Engine engine(Yb::Engine::MANUAL, conn);
        Yb::Session session(Yb::init_default_meta(), &engine);

        Domain::Client client;
        string name, email, budget;
        cout << "Enter name, email, budget:\n";
        cin >> name >> email >> budget;
        client.set_name(name);
        client.set_email(email);
        client.set_budget(Yb::Decimal(budget));
        client.set_dt(Yb::now());
        client.save(session);
        session.flush();
        cout << "New client: " << client.get_id() << endl;
        engine.commit();
        return 0;
    }
    catch (const std::exception &ex) {
        cout << "exception: " << ex.what() << endl;
        return 1;
    }
}
 
