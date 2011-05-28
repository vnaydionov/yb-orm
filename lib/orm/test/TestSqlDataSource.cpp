#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
#include "orm/SqlDataSource.h"
#include "orm/Engine.h"

using namespace std;
using namespace Yb;

class MockSqlEngine : public Engine
{
public:
    size_t select_cnt_, insert_cnt_, update_cnt_, delete_cnt_;
    Rows stored_;
    FieldSet excluded_, keys_;
    LongInt seq_;

    MockSqlEngine()
        : Engine(MANUAL, sql_dialect("ORACLE"))
        , select_cnt_(0)
        , insert_cnt_(0)
        , update_cnt_(0)
        , delete_cnt_(0)
        , seq_(100)
    {}
    RowsPtr on_select(const StrList &what,
            const StrList &from, const Filter &where,
            const StrList &/* group_by */, const Filter &/* having */,
            const StrList &/* order_by */, int /* max_rows */,
            bool /* for_update */)
    {
        RowsPtr rows(new Rows());
        if (from.get_str() == "DUAL") {
            if (what.get_str() == "S_A_X.CURRVAL") {
                Row r;
                r["VAL"] = Value(boost::lexical_cast<string>(seq_));
                rows->push_back(r);
            }
            else if (what.get_str() == "S_A_X.NEXTVAL") {
                Row r;
                r["VAL"] = Value(boost::lexical_cast<string>(++seq_));
                rows->push_back(r);
            }
        }
        else {
            const FilterBackendByKey *by_key =
                dynamic_cast<const FilterBackendByKey *>(where.get_backend());
            if (by_key && by_key->get_key_data().get("X").as_longint() == 1) {
                Row r;
                r["X"] = Value("1");
                r["Y"] = Value("#");
                r["Z"] = Value("2.5");
                rows->push_back(r);
            }
        }
        ++select_cnt_;
        return rows;
    }
    const std::vector<LongInt>
        on_insert(const std::string &/* table_name */,
            const Rows &rows, const FieldSet &exclude_fields,
            bool collect_new_ids)
    {
        stored_ = rows;
        excluded_ = exclude_fields;
        insert_cnt_ += rows.size();
        std::vector<LongInt> ids;
        return ids;
    }
    void on_update(const std::string &/* table_name */,
            const Rows &rows, const FieldSet &key_fields,
            const FieldSet &exclude_fields, const Filter &/* where */)
    {
        stored_ = rows;
        keys_ = key_fields;
        excluded_ = exclude_fields;
        update_cnt_ += rows.size();
    }
    void on_delete(const std::string &/* table_name */, const Filter &/* where */)
    {
        ++delete_cnt_;
    }
    void on_exec_proc(const std::string &/* proc_code */)
    {
    }
    void on_commit()
    {
    }
    void on_rollback()
    {
    }
};

class TestSqlDataSource : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestSqlDataSource);
    CPPUNIT_TEST(test_select_row);
    CPPUNIT_TEST_EXCEPTION(test_object_not_found_by_key, ObjectNotFoundByKey);
    CPPUNIT_TEST_EXCEPTION(test_field_not_found, FieldNotFoundInFetchedRow);
    CPPUNIT_TEST(test_row_data2sql_row);
    //CPPUNIT_TEST(test_row_data_vector2sql_rows);
    CPPUNIT_TEST_EXCEPTION(test_row_data_vector2sql_rows_mixed, TableDoesNotMatchRow);
    CPPUNIT_TEST(test_insert_rows);
    CPPUNIT_TEST(test_update_rows);
    CPPUNIT_TEST_SUITE_END();

    Schema r_;
    const Schema &get_r() const { return r_; }

public:
    void setUp()
    {
        Table t("A");
        t.set_column(Column("X", Value::LONGINT, 0,
                    Column::PK));
        t.set_column(Column("Y", Value::STRING, 0, 0));
        t.set_column(Column("Z", Value::DECIMAL, 0,
                    Column::RO));
        t.set_seq_name("S_A_X");
        Schema r;
        r.set_table(t);
        r_ = r;
    }

    void test_select_row()
    {
        MockSqlEngine engine;
        SqlDataSource ds(get_r(), engine);
        RowData key(get_r(), "A");
        key.set("X", Value(1));
        CPPUNIT_ASSERT_EQUAL(0, (int)engine.select_cnt_);
        RowDataPtr d = ds.select_row(key);
        CPPUNIT_ASSERT_EQUAL(1, (int)engine.select_cnt_);
        CPPUNIT_ASSERT(d.get());
        CPPUNIT_ASSERT_EQUAL(string("A"), d->get_table().get_name());
        CPPUNIT_ASSERT_EQUAL(string("#"), d->get("Y").as_string());
        CPPUNIT_ASSERT(Value::PKID == d->get("X").get_type());
        CPPUNIT_ASSERT(Value::STRING == d->get("Y").get_type());
        CPPUNIT_ASSERT(Value::DECIMAL == d->get("Z").get_type());
    }

    void test_object_not_found_by_key()
    {
        MockSqlEngine engine;
        SqlDataSource ds(get_r(), engine);
        RowData key(get_r(), "A");
        key.set("X", Value(3));
        ds.select_row(key);
    }

    void test_field_not_found()
    {
        Table t("A");
        t.set_column(Column("X", Value::LONGINT, 0,
                    Column::PK));
        t.set_column(Column("Q", Value::STRING, 0, 0));
        Schema r;
        r.set_table(t);
        MockSqlEngine engine;
        SqlDataSource ds(r, engine);
        RowData key(r, "A");
        key.set("X", Value(1));
        ds.select_row(key);
    }

    void test_row_data2sql_row()
    {
        RowData x(get_r(), "A");
        x.set("X", Value(2));
        x.set("Y", Value("@"));
        x.set("Z", Value(Decimal("0.5")));
        RowPtr row = SqlDataSource::row_data2sql_row(x);
        CPPUNIT_ASSERT(row.get());
        CPPUNIT_ASSERT(Value(2) == (*row)["X"]);
        CPPUNIT_ASSERT(Value("@") == (*row)["Y"]);
        CPPUNIT_ASSERT(Value(Decimal("0.5")) == (*row)["Z"]);
    }

#if 0
    void test_row_data_vector2sql_rows()
    {
        RowDataVector v;
        RowData x(get_r(), "A");
        x.set("X", Value(2));
        x.set("Y", Value("@"));
        v.push_back(x);
        RowData y(get_r(), "A");
        y.set("X", Value(4));
        y.set("Y", Value("$"));
        v.push_back(y);
        RowsPtr rows = SqlDataSource::row_data_vector2sql_rows(x.get_table(), v);
        CPPUNIT_ASSERT(rows.get());
        CPPUNIT_ASSERT_EQUAL(2, (int)rows->size());
        CPPUNIT_ASSERT(Value(2) == (*rows)[0]["X"]);
        CPPUNIT_ASSERT(Value("$") == (*rows)[1]["Y"]);
    }
#endif

    void test_row_data_vector2sql_rows_mixed()
    {
        Table t("B");
        t.set_column(Column("Q", Value::LONGINT, 0,
                    Column::PK));
        r_.set_table(t);
        RowDataVector v;
        RowData x(get_r(), "B");
        x.set("Q", Value(1));
        v.push_back(x);
        MockSqlEngine engine;
        SqlDataSource ds(get_r(), engine);
        ds.row_data_vector2sql_rows(get_r().get_table("A"), v);
    }

    void test_insert_rows()
    {
        MockSqlEngine engine;
        SqlDataSource ds(get_r(), engine);
        RowDataVector v;
        RowData x(get_r(), "A");
        x.set("X", Value(2));
        x.set("Y", Value("@"));
        v.push_back(x);
        RowData y(get_r(), "A");
        y.set("X", Value(4));
        y.set("Y", Value("$"));
        v.push_back(y);
        CPPUNIT_ASSERT_EQUAL(0, (int)engine.insert_cnt_);
        ds.insert_rows("A", v);
        CPPUNIT_ASSERT_EQUAL(2, (int)engine.insert_cnt_);
        CPPUNIT_ASSERT_EQUAL(2, (int)engine.stored_.size());
        Row row;
        row["X"] = Value(4);
        row["Y"] = Value("$");
        row["Z"] = Value();
        CPPUNIT_ASSERT(row == engine.stored_[1]);
        CPPUNIT_ASSERT_EQUAL(1, (int)engine.excluded_.size());
        CPPUNIT_ASSERT_EQUAL(string("Z"), *engine.excluded_.begin());
    }

    void test_update_rows()
    {
        MockSqlEngine engine;
        SqlDataSource ds(get_r(), engine);
        RowDataVector v;
        RowData x(get_r(), "A");
        x.set("X", Value(2));
        x.set("Y", Value("@"));
        v.push_back(x);
        RowData y(get_r(), "A");
        y.set("X", Value(4));
        y.set("Y", Value("$"));
        v.push_back(y);
        CPPUNIT_ASSERT_EQUAL(0, (int)engine.update_cnt_);
        ds.update_rows("A", v);
        CPPUNIT_ASSERT_EQUAL(2, (int)engine.update_cnt_);
        CPPUNIT_ASSERT_EQUAL(2, (int)engine.stored_.size());
        Row row;
        row["X"] = Value(4);
        row["Y"] = Value("$");
        row["Z"] = Value();
        CPPUNIT_ASSERT(row == engine.stored_[1]);
        CPPUNIT_ASSERT_EQUAL(1, (int)engine.excluded_.size());
        CPPUNIT_ASSERT_EQUAL(string("Z"), *engine.excluded_.begin());
        CPPUNIT_ASSERT_EQUAL(1, (int)engine.keys_.size());
        CPPUNIT_ASSERT_EQUAL(string("X"), *engine.keys_.begin());
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestSqlDataSource);

// vim:ts=4:sts=4:sw=4:et:
