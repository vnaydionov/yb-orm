
#include "util/nlogger.h"
#include "orm/domain_object.h"
#include "orm/domain_factory.h"
#include "orm/schema_decl.h"
//#include <soci/soci.h>

class Order;

class Client: public Yb::DomainObject {

YB_DECLARE(Client, "client_tbl", "client_seq", "client",
    YB_COL_PK(id, "id")
    YB_COL_DATA(dt, "dt", DATETIME)
    YB_COL_STR(name, "name", 100)
    YB_COL_STR(email, "email", 100)
    YB_COL_DATA(budget, "budget", DECIMAL)
    YB_REL_ONE(Client, owner, Order, orders, Yb::Relation::Restrict, "client_id", 1, 1)
    YB_COL_END)

public:
    int get_info() const { return 42; }
};

class Order: public Yb::DomainObject {

YB_DECLARE(Order, "order_tbl", "order_seq", "order",
    YB_COL_PK(id, "id")
    YB_COL_FK(client_id, "client_id", "client_tbl", "id")
    YB_COL(dt, "dt", DATETIME, 0, 0, Yb::now(), "", "", "", "")
    YB_COL_STR(memo, "memo", 100)
    YB_COL_DATA(total_sum, "total_sum", DECIMAL)
    YB_COL_DATA(receipt_sum, "receipt_sum", DECIMAL)
    YB_COL_DATA(receipt_dt, "receipt_dt", DATETIME)
    YB_REL_MANY(Client, owner, Order, orders, Yb::Relation::Restrict, "client_id", 1, 1)
    YB_COL_END)

};

YB_DEFINE(Client)
YB_DEFINE(Order)

using namespace std;

int main()
{
    Yb::LogAppender appender(cerr);
    try {
        //soci::session soci_conn("sqlite3", "./tut2.db");
        //Yb::Session session(Yb::init_schema(), "SOCI", "SQLITE", &soci_conn);
        Yb::Session session(Yb::init_schema(), "sqlite+sqlite://./tut2.db");
        session.set_logger(Yb::ILogger::Ptr(new Yb::Logger(&appender)));
        try {
            session.create_schema();
            cerr << "Schema created\n";
        }
        catch (const Yb::DBError &e) {
            cerr << "Schema already exists\n";
        }

        Order::Holder order;
        string amount;
        cout << "Enter order amount: \n";
        cin >> amount;
        order->total_sum = Yb::Decimal(amount);

        Client::Holder client;
        string name, email;
        cout << "Enter name, email: \n";
        cin >> name >> email;
        client->name = name;
        client->email = email;
        client->dt = Yb::now();

        cout << "Client's info: " << client->get_info() << endl;
        cout << "Client's orders count: " << client->orders.size() << endl;
        order->owner = client;
        cout << "Client's orders count: " << client->orders.size() << endl;

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
