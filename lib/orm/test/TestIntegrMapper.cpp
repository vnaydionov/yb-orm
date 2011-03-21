
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>

#include "util/str_utils.hpp"
#include "orm/Mapper.h"
#include "orm/SqlDataSource.h"
//#include "orm/DbpoolSession.h"
#include "orm/OdbcSession.h"
#include "orm/XMLNode.h"
#include "orm/OdbcDriver.h"

using namespace std;
using namespace Yb::ORMapper;
using namespace Yb;
using Yb::StrUtils::xgetenv;

#define TEST_TBL1 "T_ORM_TEST"
//typedef Yb::SQL::DBPoolSession MySession;
typedef Yb::SQL::OdbcSession MySession;

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
    const TableMetaDataRegistry &get_r() const { return r_; }

public:
    void setUp()
    {
        /*
        CREATE TABLE T_ORM_TEST (
            ID  NUMBER          NOT NULL,
            A   VARCHAR2 (256),
            B   DATE            DEFAULT SYSDATE,
            C   NUMBER,
            PRIMARY KEY(ID));
        CREATE SEQUENCE S_ORM_ID START WITH 100;
        */
        static const char *st[] = {
            "DELETE FROM " TEST_TBL1,
#if 0 // for Oracle
            "INSERT INTO " TEST_TBL1 "(ID, A, B, C) VALUES(1, "
                "'abc', TO_DATE('1981-05-30', 'YYYY-MM-DD'), 3.14)",
            "INSERT INTO " TEST_TBL1 "(ID, A, B, C) VALUES(2, "
                "'xyz', TO_DATE('2006-11-22 09:54:00', 'YYYY-MM-DD HH24:MI:SS'), -0.5)",
            "INSERT INTO " TEST_TBL1 "(ID, A, B, C) VALUES(3, "
                "'@#$', TO_DATE('2006-11-22', 'YYYY-MM-DD'), 0.01)"
#else
            "INSERT INTO " TEST_TBL1 "(ID, A, B, C) VALUES(1, "
                "'abc', '1981-05-30', 3.14)",
            "INSERT INTO " TEST_TBL1 "(ID, A, B, C) VALUES(2, "
                "'xyz', '2006-11-22 09:54:00', -0.5)",
            "INSERT INTO " TEST_TBL1 "(ID, A, B, C) VALUES(3, "
                "'@#$', '2006-11-22', 0.01)"
#endif
        };
        SQL::OdbcDriver drv;
        drv.open(xgetenv("YBORM_DB"), xgetenv("YBORM_USER"), xgetenv("YBORM_PASSWD"));
        for (size_t i = 0; i < sizeof(st)/sizeof(const char *); ++i)
            drv.exec_direct(st[i]);
        drv.commit();

        TableMetaData t(TEST_TBL1, "test");
        t.set_column(ColumnMetaData("ID", Value::LongLong, 0,
                    ColumnMetaData::PK));
        t.set_column(ColumnMetaData("A", Value::String, 256, 0));
        t.set_column(ColumnMetaData("B", Value::DateTime, 0,
                    ColumnMetaData::RO));
        t.set_column(ColumnMetaData("C", Value::Decimal, 0, 0));
#if 0
        t.set_seq_name("S_ORM_ID");
#else
        t.set_autoinc(true);
#endif
        TableMetaDataRegistry r;
        r.set_table(t);
        r_ = r;
    }

    void test_find()
    {
        MySession ses;
        SqlDataSource ds(get_r(), ses);
        TableMapper mapper(get_r(), ds);
        RowData key(get_r(), TEST_TBL1);
        key.set("ID", 2);
        RowData *d = mapper.find(key);
        CPPUNIT_ASSERT(d);
        XMLNode node(*d);
        CPPUNIT_ASSERT_EQUAL(string(
                    "<test><a>xyz</a><b>2006-11-22T09:54:00</b><c>-0.5</c><id>2</id></test>\n"),
                node.get_xml());
    }

    void test_object_not_found()
    {
        MySession ses;
        SqlDataSource ds(get_r(), ses);
        TableMapper mapper(get_r(), ds);
        RowData key(get_r(), TEST_TBL1);
        key.set("ID", Value(-1));
        RowData *d = mapper.find(key);
        CPPUNIT_ASSERT(d);
        d->get("A");
    }

    void test_load_collection()
    {
        MySession ses;
        SqlDataSource ds(get_r(), ses);
        TableMapper mapper(get_r(), ds);
        LoadedRows rows = mapper.load_collection(TEST_TBL1,
                SQL::Filter("ID < 10"));
        CPPUNIT_ASSERT(rows.get() && rows->size() == 3);
        std::vector<RowData * > ::const_iterator it = rows->begin(), end = rows->end();
        for (; it != end; ++it) {
            CPPUNIT_ASSERT(!(*it)->is_ghost());
            if ((*it)->get("ID").as_long_long() == 3) {
                CPPUNIT_ASSERT(Value("@#$") == (*it)->get("A"));
                CPPUNIT_ASSERT(Value(Value("2006-11-22 00:00:00").as_date_time()) == (*it)->get("B"));
                CPPUNIT_ASSERT(Value(decimal(".01")) == (*it)->get("C"));
            }
        }
    }

    void test_create()
    {
        long long id;
        {
            MySession ses(Yb::SQL::Session::MANUAL);
            SqlDataSource ds(get_r(), ses);
            TableMapper mapper(get_r(), ds);
            RowData *d = mapper.create(TEST_TBL1);
            CPPUNIT_ASSERT(d);
            d->set("A", "...");
            d->set("C", decimal("-.001"));
            mapper.flush();
            id = d->get("ID").as_long_long();
            ses.commit();
        }
        {
            MySession ses;
            SqlDataSource ds(get_r(), ses);
            TableMapper mapper(get_r(), ds);
            RowData key(get_r(), TEST_TBL1);
            key.set("ID", id);
            RowData *d = mapper.find(key);
            CPPUNIT_ASSERT(d);
            CPPUNIT_ASSERT(!d->get("B").is_null());
            CPPUNIT_ASSERT(Value("-0.001") == d->get("C"));
            CPPUNIT_ASSERT_EQUAL(string("..."), d->get("A").as_string());
        }
    }

    void test_register_as_new()
    {
        long long id;
        {
            MySession ses(Yb::SQL::Session::MANUAL);
            SqlDataSource ds(get_r(), ses);
            TableMapper mapper(get_r(), ds);
            RowData d(get_r(), TEST_TBL1);
            SQL::RowsPtr res(ses.select(SQL::StrList("MAX(ID) M_ID"),
                    SQL::StrList(TEST_TBL1)));
#if 0
            id = ds.get_next_id("S_ORM_ID");
#else
            id = (*res)[0]["M_ID"].as_long_long() + 1;
#endif
            d.set("ID", id);
            d.set("A", Value(string("...")));
            d.set("C", decimal("-.001"));
            mapper.register_as_new(d);
            mapper.flush();
            ses.commit();
        }
        {
            MySession ses;
            SqlDataSource ds(get_r(), ses);
            TableMapper mapper(get_r(), ds);
            RowData key(get_r(), TEST_TBL1);
            key.set("ID", id);
            RowData *d = mapper.find(key);
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
            MySession ses(Yb::SQL::Session::MANUAL);
            SqlDataSource ds(get_r(), ses);
            TableMapper mapper(get_r(), ds);
            RowData *d = mapper.find(key1);
            CPPUNIT_ASSERT(d);
            d->set("A", Value(string("???")));
            d->set("C", decimal(".001"));
            RowData *e = mapper.find(key2);
            CPPUNIT_ASSERT(e);
            e->set("A", "xxx");
            mapper.flush();
            ses.commit();
        }
        {
            MySession ses;
            SqlDataSource ds(get_r(), ses);
            TableMapper mapper(get_r(), ds);
            RowData *d = mapper.find(key1);
            CPPUNIT_ASSERT(d);
            CPPUNIT_ASSERT(!d->get("B").is_null());
            CPPUNIT_ASSERT(decimal("0.001") == d->get("C").as_decimal());
            CPPUNIT_ASSERT_EQUAL(string("???"), d->get("A").as_string());
            RowData *e = mapper.find(key2);
            CPPUNIT_ASSERT(e);
            CPPUNIT_ASSERT_EQUAL(string("xxx"), e->get("A").as_string());
        }
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestIntegrMapper);

// vim:ts=4:sts=4:sw=4:et


