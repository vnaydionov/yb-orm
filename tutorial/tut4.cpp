#include <memory>
#include <iostream>
#include <boost/foreach.hpp>
#include <orm/DomainObj.h>
#include "domain/ProductGroup.h"
#include "domain/Product.h"
using namespace std;
using namespace boost;
using namespace Yb;
using namespace Domain;

int main()
{
    LogAppender appender(cerr);
    try {
        Yb::init_schema();
        auto_ptr<SqlConnection> conn(new SqlConnection(
                    "sqlite+sqlite://c:/yborm/examples/test1_db"));
        Engine engine(Engine::READ_WRITE, conn);
        engine.set_logger(ILogger::Ptr(new Logger(&appender)));
        engine.set_echo(true);

        LongInt root = -1;
        {
            Session session(Yb::theSchema::instance(), &engine);

            ProductGroup::Holder pg1(session);
            pg1->name = "Group1";

            ProductGroup::Holder pg2(session);
            pg2->name = "Group2";
            pg2->parent = pg1;
            ProductGroup::Holder pg3(session);
            pg3->name = "Group3";
            pg3->parent = pg1;

            Product::Holder pr1(session);
            pr1->name = "Product1";
            pr1->price = Decimal("1.00");
            pr1->parent = pg2;

            session.commit();
            root = pg1->id;
        }
        {
            Session session(Yb::theSchema::instance(), &engine);
            ProductGroup::Holder pg1(session, root);
            cout << pg1->parent->id.is_null() << endl;
            pg1->delete_object();

            session.commit();
        }
        return 0;
    }
    catch (const std::exception &ex) {
        cout << "exception: " << ex.what() << endl;
        return 1;
    }
}
