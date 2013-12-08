#include <memory>
#include <iostream>
#include <boost/foreach.hpp>
#include "domain/Client.h"
#include "domain/Order.h"
using namespace std;
using namespace boost;
using namespace Yb;
using namespace Domain;

int main()
{
    LogAppender appender(cerr);
    try {
        auto_ptr<SqlConnection> conn(new SqlConnection(
                    "sqlite+sqlite://c:/yborm/examples/test1_db"));
        Engine engine(Engine::READ_WRITE, conn);
        engine.set_logger(ILogger::Ptr(new Logger(&appender)));
        engine.set_echo(true);
        Session session(init_schema(), &engine);

        int id;
        cout << "Enter client ID: \n";
        cin >> id;
        Client client_1(session, id);
        try {
            cout << client_1.name.value() << endl;
        } catch (NoDataFound &) {
            cerr << "No such client\n";
        }

        string name;
        cout << "Enter client name: \n";
        cin >> name;
        Client client_2 = query<Client>(session)
            .filter_by(Client::c.name == name).one();
        cout << client_2.name.value() << endl;

        cout << "Walking through client's orders property: \n";
        BOOST_FOREACH(Order order, client_2.orders) {
            cout << "(" << order.id 
                << "," << order.owner->id 
                << "," << order.dt
                << "," << order.total_sum 
                << ")" << endl;
        }

        string min_sum;
        cout << "Enter minimal sum: \n";
        cin >> min_sum;
        DomainResultSet<Order> rs_1 = query<Order>(session)
            .filter_by(Order::c.total_sum > Decimal(min_sum)
                       && Order::c.receipt_dt == Value()).all();
        cout << "Found orders: ";
        BOOST_FOREACH(Order order, rs_1) {
            cout << order.id << ",";
        }
        cout << endl;

        cout << "Order count: " << query<Order>(session)
            .filter_by(Order::c.client_id == client_2.id).count() << endl;

        typedef tuple<Order, Client> Pair;
        DomainResultSet<Pair> rs = query<Pair>(session)
            .filter_by(Order::c.total_sum > Decimal(100)
                       && Client::c.budget == Value())
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
