
#include "util/nlogger.h"
#include "orm/domain_object.h"
#include "orm/domain_factory.h"
#include "orm/schema_decl.h"

class Product;

class ProductGroup: public Yb::DomainObject {

YB_DECLARE(ProductGroup, "tbl_product_group", "seq_prod_group", "product-group",
    YB_COL_PK(id, "id")
    YB_COL_FK_NULLABLE(parent_id, "parent_id", "tbl_product_group", "id")
    YB_COL_STR(name, "name", 100)
    YB_REL_ONE(ProductGroup, parent, ProductGroup, children, Yb::Relation::Delete, "parent_id", 1, 1)
    YB_REL_MANY(ProductGroup, parent, ProductGroup, children, Yb::Relation::Delete, "parent_id", 1, 1)
    YB_REL_ONE(ProductGroup, parent, Product, products, Yb::Relation::Delete, "parent_id", 1, 1)
    YB_COL_END)
};

class Product: public Yb::DomainObject {

YB_DECLARE(Product, "tbl_product", "seq_prod_group", "product",
    YB_COL_PK(id, "id")
    YB_COL_FK(parent_id, "parent_id", "tbl_product_group", "id")
    YB_COL_STR(name, "name", 100)
    YB_COL_DATA(price, "price", DECIMAL)
    YB_REL_MANY(ProductGroup, parent, Product, products, Yb::Relation::Delete, "parent_id", 1, 1)
    YB_COL_END)
};

YB_DEFINE(ProductGroup)
YB_DEFINE(Product)

using namespace std;

int main()
{
    Yb::LogAppender appender(cerr);
    try {
        auto_ptr<Yb::SqlConnection> conn(new Yb::SqlConnection(
                "sqlite+sqlite://./tut4.db"));
        Yb::Engine engine(Yb::Engine::READ_WRITE, conn);
        engine.set_echo(true);
        engine.set_logger(Yb::ILogger::Ptr(new Yb::Logger(&appender)));
        Yb::init_schema();
        try {
            engine.create_schema(Yb::theSchema(), false);
            cerr << "Schema created\n";
        }
        catch (const Yb::DBError &e) {
            cerr << "Schema already exists\n";
        }
        
        using namespace std;
        using namespace Yb;
        LongInt root = -1;
        {
            Session session(Yb::theSchema(), &engine);

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
            Session session(Yb::theSchema(), &engine);
            ProductGroup::Holder pg1(session, root);
            cout << pg1->parent->id.is_null() << endl;
            pg1->delete_object();

            session.commit();
        }
    }
    catch (const std::exception &ex) {
        cout << "exception: " << ex.what() << endl;
        return 1;
    }
    return 0;
}
