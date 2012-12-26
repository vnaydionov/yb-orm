#include <memory>
#include <iostream>
#include <boost/foreach.hpp>
#include <orm/MetaDataSingleton.h>
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
        auto_ptr<SqlConnection> conn(new SqlConnection(
                    "sqlite+sqlite:///home/vaclav/work/test1.db"));
        Engine engine(Engine::READ_WRITE, conn);
        engine.set_logger(ILogger::Ptr(new Logger(&appender)));
        engine.set_echo(true);

        LongInt root = -1;
        {
        Session session(init_default_meta(), &engine);

        ProductGroupHolder pg1(session);
        pg1->name = "Group1";

        ProductGroupHolder pg2(session);
        pg2->name = "Group2";
        pg2->parent = pg1;
        ProductGroupHolder pg3(session);
        pg3->name = "Group3";
        pg3->parent = pg1;

        ProductHolder pr1(session);
        pr1->name = "Product1";
        pr1->price = Decimal("1.00");
        pr1->parent = pg2;

        session.commit();
        root = pg1->id;
        }
        {
        Session session(init_default_meta(), &engine);
        ProductGroupHolder pg1(session, root);
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
