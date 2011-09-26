#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
#include "orm/RowData.h"

using namespace std;
using namespace Yb;

class TestRowData : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestRowData);
    CPPUNIT_TEST(test_row_data);
    CPPUNIT_TEST(test_row_data_comp);
    CPPUNIT_TEST(test_row_data_key_comp);
    CPPUNIT_TEST(test_row_data_comp_new);
    CPPUNIT_TEST_EXCEPTION(test_row_data_no_table1, RowNotLinkedToTable);
    CPPUNIT_TEST_EXCEPTION(test_row_data_no_table2, RowNotLinkedToTable);
    CPPUNIT_TEST_EXCEPTION(test_row_data_no_table3, RowNotLinkedToTable);
    CPPUNIT_TEST_EXCEPTION(test_row_data_try_ro, ReadOnlyColumn);
    CPPUNIT_TEST(test_filter_by_key);
    CPPUNIT_TEST_EXCEPTION(test_init_row_wrong_val, BadTypeCast);
    CPPUNIT_TEST_EXCEPTION(test_init_row_long_string, StringTooLong);
    CPPUNIT_TEST(test_set_value_type);
    CPPUNIT_TEST_EXCEPTION(test_bad_type_cast_long_long, BadTypeCast);
    CPPUNIT_TEST_EXCEPTION(test_bad_type_cast_Decimal, BadTypeCast);
    CPPUNIT_TEST_EXCEPTION(test_bad_type_cast_date_time, BadTypeCast);
    CPPUNIT_TEST(test_bad_type_cast_format);
    CPPUNIT_TEST_SUITE_END();

    Schema r_;
    const Schema &get_r() const { return r_; }

public:
    void setUp()
    {
        Table t("A");
        t.add_column(Column("X", Value::LONGINT, 0,
                    Column::PK));
        t.add_column(Column("Y", Value::STRING, 4, 0));
        t.set_seq_name("S_A_X");
        Schema r;
        r.set_table(t);
        r_ = r;
    }

    void test_row_data()
    {
        RowData d(get_r(), "a");
        CPPUNIT_ASSERT_EQUAL(string("A"), d.get_table().get_name());
        CPPUNIT_ASSERT(d.get("X").is_null());
        d.set("x", Value("10"));
        d.set("y", Value("zzz"));
        CPPUNIT_ASSERT_EQUAL((LongInt)10, d.get("X").as_longint());
        CPPUNIT_ASSERT_EQUAL(string("zzz"), d.get("Y").as_string());
    }

    void test_row_data_comp()
    {
        CPPUNIT_ASSERT(RowData() == RowData());
        RowData d1(get_r(), "a"), d2(get_r(), "A");
        CPPUNIT_ASSERT(RowData() != d1);
        CPPUNIT_ASSERT(d1 == d2);
        d1.set("x", Value(10));
        d1.set("y", Value("zzz"));
        d2.set("Y", Value("zzz"));
        d2.set("X", Value(10));
        CPPUNIT_ASSERT(d1 == d2);
        d2.set("Y", Value());
        CPPUNIT_ASSERT(d1 != d2);
        d1.set("Y", Value());
        CPPUNIT_ASSERT(d1 == d2);
    }

    void test_row_data_key_comp()
    {
        Table t("B");
        t.add_column(Column("X", Value::LONGINT, 0, Column::PK));
        t.add_column(Column("Y", Value::STRING, 0, Column::PK));
        r_.set_table(t);
        //
        CPPUNIT_ASSERT(!RowData_key_less()(RowData(), RowData()));
        RowData d0(get_r(), "A");
        CPPUNIT_ASSERT(!RowData_key_less()(d0, RowData()));
        CPPUNIT_ASSERT(!!RowData_key_less()(RowData(), d0));
        RowData d1(get_r(), "A");
        CPPUNIT_ASSERT(!RowData_key_less()(d0, d1));
        CPPUNIT_ASSERT(!RowData_key_less()(d1, d0));
        d0.set("X", Value(1));
        d1.set("X", Value(1));
        CPPUNIT_ASSERT(!RowData_key_less()(d0, d1));
        CPPUNIT_ASSERT(!RowData_key_less()(d1, d0));
        d0.set("Y", Value("ab"));
        d1.set("Y", Value("ac"));
        CPPUNIT_ASSERT(!RowData_key_less()(d0, d1));
        CPPUNIT_ASSERT(!RowData_key_less()(d1, d0));
        RowData d2(get_r(), "A");
        d2.set("X", Value(2));
        CPPUNIT_ASSERT(!!RowData_key_less()(d1, d2));
        RowData d3(get_r(), "B"), d4(get_r(), "B");
        CPPUNIT_ASSERT(!!RowData_key_less()(d2, d3));
        d3.set("X", Value(1));
        d3.set("Y", Value("ab"));
        d4.set("X", Value(1));
        d4.set("Y", Value("aa"));
        CPPUNIT_ASSERT(!!RowData_key_less()(d4, d3));
    }

    void test_row_data_comp_new()
    {
        RowData d1(get_r(), "A"), d2(get_r(), "A");
        d1.set("X", Value(1));
        CPPUNIT_ASSERT(!d1.is_new());
        CPPUNIT_ASSERT(!d2.is_new());
        d2.set_new();
        d2.set_pk(1);
        CPPUNIT_ASSERT(d2.is_new());
        CPPUNIT_ASSERT_EQUAL((LongInt)1, d1.get("X").as_longint());
        CPPUNIT_ASSERT_EQUAL((LongInt)1, d2.get("X").as_longint());
        CPPUNIT_ASSERT(d1 == d2);
        CPPUNIT_ASSERT(!RowData_key_less()(d1, d2));
        CPPUNIT_ASSERT(!RowData_key_less()(d2, d1));
    }

    void test_row_data_no_table1()
    {
        RowData r;
        r.get_table();
    }

    void test_row_data_no_table2()
    {
        RowData r;
        r.get("X");
    }

    void test_row_data_no_table3()
    {
        RowData r;
        r.set("X", Value());
    }

    void test_row_data_try_ro()
    {
        RowData d(get_r(), "A");
        d.set("X", Value(PKIDValue(get_r().get_table("A"), 1)));
        d.set("X", Value(1));
        d.set("X", Value(2));
    }

    void test_filter_by_key()
    {
        RowData d(get_r(), "A");
        d.set("X", Value(1));
        d.set("Y", Value("a"));
        CPPUNIT_ASSERT_EQUAL(string("1=1 AND X = 1"), FilterByKey(d).get_sql());
    }

    void test_init_row_wrong_val()
    {
        RowData d(get_r(), "A");
        d.set("X", Decimal(1.2));
    }

    void test_init_row_long_string()
    {
        RowData d(get_r(), "A");
        d.set("Y", "zrzzz");
    }

    void test_set_value_type()
    {
        Table t("A");
        Column cx("X", Value::LONGINT, 0, 0);
        t.add_column(cx);
        Column cy("Y", Value::STRING, 0, 0);
        t.add_column(cy);
        Column cz("Z", Value::DECIMAL, 0, 0);
        t.add_column(cz);
        Column cw("W", Value::DATETIME, 0, 0);
        t.add_column(cw);
        Schema r;
        r.set_table(t);

        RowData d(r, "A");
        CPPUNIT_ASSERT(Value() == d.get_typed_value(cx, Value()));
        CPPUNIT_ASSERT(Value(12) == d.get_typed_value(cx, Value("12")));
        CPPUNIT_ASSERT(Value(Decimal("1.2")) == d.get_typed_value(cz, Value("1.20")));
        CPPUNIT_ASSERT(Value("ab") == d.get_typed_value(cy, Value("ab")));
        DateTime a(mk_datetime(2006, 11, 16, 15, 5, 10));
        CPPUNIT_ASSERT(Value(a) == d.get_typed_value(cw, Value("2006-11-16T15:05:10")));
    }

    void test_bad_type_cast_long_long()
    {
        RowData d(get_r(), "A");
        d.get_typed_value(get_r().get_table("A").get_column("X"), Value("2.5"));
    }

    void test_bad_type_cast_Decimal()
    {
        Table t("A");
        Column cz("Z", Value::DECIMAL, 0, 0);
        t.add_column(cz);
        Schema r;
        r.set_table(t);

        RowData d(r, "A");
        d.get_typed_value(r.get_table("A").get_column("Z"), Value("abc"));
    }

    void test_bad_type_cast_date_time()
    {
        Table t("A");
        Column cw("W", Value::DATETIME, 0, 0);
        t.add_column(cw);
        Schema r;
        r.set_table(t);

        RowData d(r, "A");
        d.get_typed_value(r.get_table("A").get_column("W"), Value("abc"));
    }

    void test_bad_type_cast_format()
    {
        try {
            Table t("A");
            t.add_column(Column("X", Value::LONGINT, 0, Column::PK | Column::RO));
            t.add_column(Column("Y", Value::DECIMAL, 0, 0));
            Schema r;
            r.set_table(t);

            RowData d(r, "A");
            d.get_typed_value(t.get_column("Y"), Value("#"));
            CPPUNIT_FAIL("Exception BadTypeCast not thrown!");
        }
        catch (const BadTypeCast &e) {
            string err = e.what();
            size_t pos = err.find("\nBacktrace");
            if (pos != string::npos)
                err = string(err, 0, pos);
            CPPUNIT_ASSERT_EQUAL(string("Can't cast field A.Y = \"#\" to type Decimal"),
                                 err);
        }
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestRowData);

// vim:ts=4:sts=4:sw=4:et:
