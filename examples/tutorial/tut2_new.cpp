
#include "util/nlogger.h"
#include "orm/domain_object.h"
#include "orm/domain_factory.h"
#include "yb_gen.h"

namespace Domain {

class Order;

class Client: public Yb::DomainObject {

YB_GEN_DECLARE(Client, "client_tbl", "client_seq", "client",
    YB_GEN_COL_PK(id, "id")
    YB_GEN_COL_DATA(dt, "dt", DATETIME)
    YB_GEN_COL_STR(name, "name", 100)
    YB_GEN_COL_STR(email, "email", 100)
    YB_GEN_COL_DATA(budget, "budget", DECIMAL)
    YB_GEN_REL_ONE(Client, owner, Order, orders, Yb::Relation::Restrict, "client_id", 1, 1)
    YB_GEN_COL_END)

public:
    int get_some() const { return 42; }
};

class Order: public Yb::DomainObject {

YB_GEN_DECLARE(Order, "order_tbl", "order_seq", "order",
    YB_GEN_COL_PK(id, "id")
    YB_GEN_COL_FK(client_id, "client_id", "client_tbl", "id")
    YB_GEN_COL(dt, "dt", DATETIME, 0, 0, Yb::now(), "", "", "", "")
    YB_GEN_COL_STR(memo, "memo", 100)
    YB_GEN_COL_DATA(total_sum, "total_sum", DECIMAL)
    YB_GEN_COL_DATA(receipt_sum, "receipt_sum", DECIMAL)
    YB_GEN_COL_DATA(receipt_dt, "receipt_dt", DATETIME)
    YB_GEN_REL_MANY(Client, owner, Order, orders, Yb::Relation::Restrict, "client_id", 1, 1)
    YB_GEN_COL_END)

public:
    int get_some() const { return 43; }
};

YB_GEN_DEFINE(Client)
YB_GEN_DEFINE(Order)

}

using namespace std;

int main()
{
    Yb::LogAppender appender(cerr);
    try {
        auto_ptr<Yb::SqlConnection> conn(new Yb::SqlConnection(
                "sqlite+sqlite://./test1_db"));
        Yb::Engine engine(Yb::Engine::READ_WRITE, conn);
        engine.set_echo(true);
        engine.set_logger(Yb::ILogger::Ptr(new Yb::Logger(&appender)));
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
