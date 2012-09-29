#include <memory>
#include <iostream>
#include <boost/foreach.hpp>
#include <orm/MetaDataSingleton.h>
#include "domain/Client.h"
#include "domain/Order.h"
using namespace std;
using namespace boost;
using namespace Yb;
using namespace Domain;

int main()
{
    Yb::LogAppender appender(cerr);
    try {
        auto_ptr<Yb::SqlConnection> conn(new Yb::SqlConnection(
                    "mysql://test1:test1_pwd@test1_db"));
        Yb::Engine engine(Yb::Engine::MANUAL, conn);
        engine.set_logger(Yb::ILogger::Ptr(new Yb::Logger(&appender)));
        engine.set_echo(true);
        Yb::Session session(Yb::init_default_meta(), &engine);

        string name;
        cout << "Enter client name: \n";
        cin >> name;
        Client client = query<Client>(session)
            .filter_by(Client::c.name == Value(name)).one();

        cout << "Order count: " << query<Order>(session)
            .filter_by(Order::c.client_id == client.id).count() << endl;

        typedef tuple<Order, Client> Pair;
        DomainResultSet<Pair> rs = query<Pair>(session)
            .filter_by(Order::c.total_sum > Decimal(100)
                       && Order::c.receipt_sum == Decimal(0))
            .order_by(ExpressionList(Client::c.id, Order::c.id)).all();
        BOOST_FOREACH(Pair pair, rs) {
            cout << pair.get<1>().name.value()
                 << " " << pair.get<0>().total_sum.value() << endl;
        }

        return 0;
    }
    catch (const std::exception &ex) {
        cout << "exception: " << ex.what() << endl;
        return 1;
    }
}
