#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
#include "util/str_utils.hpp"
#include "orm/Session.h"
#include "orm/SqlDataSource.h"
#include "orm/Engine.h"
#include "orm/XMLNode.h"
#include "orm/OdbcDriver.h"

using namespace std;
using namespace Yb;
using Yb::StrUtils::xgetenv;

#define TEST_TBL1 "T_ORM_TEST"
#define NUM_STMT 4

typedef Engine MyEngine;

class TestIntegrMapper : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestIntegrMapper);
    CPPUNIT_TEST(test_find);
    CPPUNIT_TEST_EXCEPTION(test_object_not_found, ObjectNotFoundByKey);
    CPPUNIT_TEST(test_load_collection);
    CPPUNIT_TEST(test_create);
    CPPUNIT_TEST(test_register_as_new);
    CPPUNIT_TEST(test_update);
    CPPUNIT_TEST_SUITE_END();

    TableMetaDataRegistry r_;
    string db_type_;
    const TableMetaDataRegistry &get_r() const { return r_; }

    LongInt get_next_test_id(EngineBase &engine, const string &seq_name)
    {
        if (db_type_ == "MYSQL") {
            RowsPtr ptr = engine.select("MAX(ID) MAX_ID", "T_ORM_TEST", Filter());
            CPPUNIT_ASSERT(1 == ptr->size() && 1 == (*ptr)[0].size());
            Value x = (*ptr)[0]["MAX_ID"];
            return x.is_null()? 1: x.as_longint() + 1;
        }
        else {
            return engine.get_next_value(seq_name);
        }
    }
public:
    void setUp()
    {
        db_type_ = xgetenv("YBORM_DBTYPE");
        /*
        CREATE TABLE T_ORM_TEST (
            ID  NUMBER          NOT NULL,
            A   VARCHAR2 (256),
            B   DATE            DEFAULT SYSDATE,
            C   NUMBER,
            PRIMARY KEY(ID));
        CREATE SEQUENCE S_ORM_TEST_ID START WITH 100;
        */
        const char **st;
        if (db_type_ == "ORACLE") {
            static const char *st_data[NUM_STMT] = {
                "DELETE FROM " TEST_TBL1,
                "INSERT INTO " TEST_TBL1 "(ID, A, B, C) VALUES(1, "
                    "'abc', TO_DATE('1981-05-30', 'YYYY-MM-DD'), 3.14)",
                "INSERT INTO " TEST_TBL1 "(ID, A, B, C) VALUES(2, "
                    "'xyz', TO_DATE('2006-11-22 09:54:00', 'YYYY-MM-DD HH24:MI:SS'), -0.5)",
                "INSERT INTO " TEST_TBL1 "(ID, A, B, C) VALUES(3, "
                    "'@#$', TO_DATE('2006-11-22', 'YYYY-MM-DD'), 0.01)"
            };
            st = st_data;
        }
        else {
            static const char *st_data[NUM_STMT] = {
                "DELETE FROM " TEST_TBL1,
                "INSERT INTO " TEST_TBL1 "(ID, A, B, C) VALUES(1, "
                    "'abc', '1981-05-30', 3.14)",
                "INSERT INTO " TEST_TBL1 "(ID, A, B, C) VALUES(2, "
                    "'xyz', '2006-11-22 09:54:00', -0.5)",
                "INSERT INTO " TEST_TBL1 "(ID, A, B, C) VALUES(3, "
                    "'@#$', '2006-11-22', 0.01)"
            };
            st = st_data;
        }
        OdbcDriver drv;
        drv.open(xgetenv("YBORM_DB"), xgetenv("YBORM_USER"), xgetenv("YBORM_PASSWD"));
        for (size_t i = 0; i < NUM_STMT; ++i)
            drv.exec_direct(st[i]);
        drv.commit();

        TableMetaData t(TEST_TBL1, "test");
        t.set_column(ColumnMetaData("ID", Value::LONGINT, 0,
                    ColumnMetaData::PK));
        t.set_column(ColumnMetaData("A", Value::STRING, 256, 0));
        t.set_column(ColumnMetaData("B", Value::DATETIME, 0,
                    ColumnMetaData::RO));
        t.set_column(ColumnMetaData("C", Value::DECIMAL, 0, 0));
        if (db_type_ == "MYSQL")
            t.set_autoinc(true);
        else
            t.set_seq_name("S_ORM_TEST_ID");
        TableMetaDataRegistry r;
        r.set_table(t);
        r_ = r;
    }

    void test_find()
    {
        MyEngine engine;
        SqlDataSource ds(get_r(), engine);
        Session session(get_r(), ds);
        RowData key(get_r(), TEST_TBL1);
        key.set("ID", 2);
        RowData *d = session.find(key);
        CPPUNIT_ASSERT(d);
        XMLNode node(*d);
        CPPUNIT_ASSERT_EQUAL(string(
                    "<test><a>xyz</a><b>2006-11-22T09:54:00</b><c>-0.5</c><id>2</id></test>\n"),
                node.get_xml());
    }

    void test_object_not_found()
    {
        MyEngine engine;
        SqlDataSource ds(get_r(), engine);
        Session session(get_r(), ds);
        RowData key(get_r(), TEST_TBL1);
        key.set("ID", Value(-1));
        RowData *d = session.find(key);
        CPPUNIT_ASSERT(d);
        d->get("A");
    }

    void test_load_collection()
    {
        MyEngine engine;
        SqlDataSource ds(get_r(), engine);
        Session session(get_r(), ds);
        LoadedRows rows = session.load_collection(TEST_TBL1,
                Filter("ID < 10"));
        CPPUNIT_ASSERT(rows.get() && rows->size() == 3);
        std::vector<RowData * > ::const_iterator it = rows->begin(), end = rows->end();
        for (; it != end; ++it) {
            CPPUNIT_ASSERT(!(*it)->is_ghost());
            if ((*it)->get("ID").as_longint() == 3) {
                CPPUNIT_ASSERT(Value("@#$") == (*it)->get("A"));
                CPPUNIT_ASSERT(Value(Value("2006-11-22 00:00:00").as_date_time()) == (*it)->get("B"));
                CPPUNIT_ASSERT(Value(Decimal(".01")) == (*it)->get("C"));
            }
        }
    }

    void test_create()
    {
        LongInt id;
        {
            MyEngine engine(EngineBase::MANUAL);
            SqlDataSource ds(get_r(), engine);
            Session session(get_r(), ds);
            RowData *d = session.create(TEST_TBL1);
            CPPUNIT_ASSERT(d);
            d->set("A", "...");
            d->set("C", Decimal("-.001"));
            session.flush();
            id = d->get("ID").as_longint();
            engine.commit();
        }
        {
            MyEngine engine;
            SqlDataSource ds(get_r(), engine);
            Session session(get_r(), ds);
            RowData key(get_r(), TEST_TBL1);
            key.set("ID", id);
            RowData *d = session.find(key);
            CPPUNIT_ASSERT(d);
            CPPUNIT_ASSERT(!d->get("B").is_null());
            CPPUNIT_ASSERT(Value("-0.001") == d->get("C"));
            CPPUNIT_ASSERT_EQUAL(string("..."), d->get("A").as_string());
        }
    }

    void test_register_as_new()
    {
        LongInt id;
        {
            MyEngine engine(EngineBase::MANUAL);
            SqlDataSource ds(get_r(), engine);
            Session session(get_r(), ds);
            RowData d(get_r(), TEST_TBL1);
            id = get_next_test_id(engine, "S_ORM_TEST_ID");
            d.set("ID", id);
            d.set("A", Value(string("...")));
            d.set("C", Decimal("-.001"));
            session.register_as_new(d);
            session.flush();
            engine.commit();
        }
        {
            MyEngine engine;
            SqlDataSource ds(get_r(), engine);
            Session session(get_r(), ds);
            RowData key(get_r(), TEST_TBL1);
            key.set("ID", id);
            RowData *d = session.find(key);
            CPPUNIT_ASSERT(d);
            CPPUNIT_ASSERT_EQUAL(string("..."), d->get("A").as_string());
            CPPUNIT_ASSERT(Value("-0.001") == d->get("C"));
            CPPUNIT_ASSERT(!d->get("B").is_null());
        }
    }

    void test_update()
    {
        RowData key1(get_r(), TEST_TBL1);
        key1.set("ID", 2);
        RowData key2(get_r(), TEST_TBL1);
        key2.set("ID", 1);
        {
            MyEngine engine(EngineBase::MANUAL);
            SqlDataSource ds(get_r(), engine);
            Session session(get_r(), ds);
            RowData *d = session.find(key1);
            CPPUNIT_ASSERT(d);
            d->set("A", Value(string("???")));
            d->set("C", Decimal(".001"));
            RowData *e = session.find(key2);
            CPPUNIT_ASSERT(e);
            e->set("A", "xxx");
            session.flush();
            engine.commit();
        }
        {
            MyEngine engine;
            SqlDataSource ds(get_r(), engine);
            Session session(get_r(), ds);
            RowData *d = session.find(key1);
            CPPUNIT_ASSERT(d);
            CPPUNIT_ASSERT(!d->get("B").is_null());
            CPPUNIT_ASSERT(Decimal("0.001") == d->get("C").as_decimal());
            CPPUNIT_ASSERT_EQUAL(string("???"), d->get("A").as_string());
            RowData *e = session.find(key2);
            CPPUNIT_ASSERT(e);
            CPPUNIT_ASSERT_EQUAL(string("xxx"), e->get("A").as_string());
        }
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestIntegrMapper);

// vim:ts=4:sts=4:sw=4:et:
