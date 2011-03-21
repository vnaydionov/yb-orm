
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>

#include "orm/SqlDataSource.h"

using namespace std;
using namespace Yb::SQL;
using namespace Yb::ORMapper;
using Yb::Value;

class MockSqlSession : public Yb::SQL::Session
{
public:
    size_t select_cnt_, insert_cnt_, update_cnt_, delete_cnt_;
    Rows stored_;
    FieldSet excluded_, keys_;
    long long seq_;

    MockSqlSession()
        : Session(MANUAL)
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
            if (by_key && by_key->get_key_data().get("X").as_long_long() == 1) {
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
    const std::vector<long long>
        on_insert(const std::string &/* table_name */,
            const Rows &rows, const FieldSet &exclude_fields,
            bool collect_new_ids)
    {
        stored_ = rows;
        excluded_ = exclude_fields;
        insert_cnt_ += rows.size();
        std::vector<long long> ids;
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
    CPPUNIT_TEST(test_seq);
    CPPUNIT_TEST_SUITE_END();

    TableMetaDataRegistry r_;
    const TableMetaDataRegistry &get_r() const { return r_; }

public:
    void setUp()
    {
        TableMetaData t("A");
        t.set_column(ColumnMetaData("X", Value::LongLong, 0,
                    ColumnMetaData::PK));
        t.set_column(ColumnMetaData("Y", Value::String, 0, 0));
        t.set_column(ColumnMetaData("Z", Value::Decimal, 0,
                    ColumnMetaData::RO));
        t.set_seq_name("S_A_X");
        TableMetaDataRegistry r;
        r.set_table(t);
        r_ = r;
    }

    void test_select_row()
    {
        MockSqlSession ses;
        SqlDataSource ds(get_r(), ses);
        RowData key(get_r(), "A");
        key.set("X", Value(1));
        CPPUNIT_ASSERT_EQUAL(0U, ses.select_cnt_);
        RowDataPtr d = ds.select_row(key);
        CPPUNIT_ASSERT_EQUAL(1U, ses.select_cnt_);
        CPPUNIT_ASSERT(d.get());
        CPPUNIT_ASSERT_EQUAL(string("A"), d->get_table().get_name());
        CPPUNIT_ASSERT_EQUAL(string("#"), d->get("Y").as_string());
        CPPUNIT_ASSERT(Value::PKID == d->get("X").get_type());
        CPPUNIT_ASSERT(Value::String == d->get("Y").get_type());
        CPPUNIT_ASSERT(Value::Decimal == d->get("Z").get_type());
    }

    void test_object_not_found_by_key()
    {
        MockSqlSession ses;
        SqlDataSource ds(get_r(), ses);
        RowData key(get_r(), "A");
        key.set("X", Value(3));
        ds.select_row(key);
    }

    void test_field_not_found()
    {
        TableMetaData t("A");
        t.set_column(ColumnMetaData("X", Value::LongLong, 0,
                    ColumnMetaData::PK));
        t.set_column(ColumnMetaData("Q", Value::String, 0, 0));
        TableMetaDataRegistry r;
        r.set_table(t);
        MockSqlSession ses;
        SqlDataSource ds(r, ses);
        RowData key(r, "A");
        key.set("X", Value(1));
        ds.select_row(key);
    }

    void test_row_data2sql_row()
    {
        RowData x(get_r(), "A");
        x.set("X", Value(2));
        x.set("Y", Value("@"));
        x.set("Z", Value(decimal("0.5")));
        RowPtr row = SqlDataSource::row_data2sql_row(x);
        CPPUNIT_ASSERT(row.get());
        CPPUNIT_ASSERT(Value(2) == (*row)["X"]);
        CPPUNIT_ASSERT(Value("@") == (*row)["Y"]);
        CPPUNIT_ASSERT(Value(decimal("0.5")) == (*row)["Z"]);
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
        CPPUNIT_ASSERT_EQUAL(2U, rows->size());
        CPPUNIT_ASSERT(Value(2) == (*rows)[0]["X"]);
        CPPUNIT_ASSERT(Value("$") == (*rows)[1]["Y"]);
    }
#endif

    void test_row_data_vector2sql_rows_mixed()
    {
        TableMetaData t("B");
        t.set_column(ColumnMetaData("Q", Value::LongLong, 0,
                    ColumnMetaData::PK));
        r_.set_table(t);
        RowDataVector v;
        RowData x(get_r(), "B");
        x.set("Q", Value(1));
        v.push_back(x);
        MockSqlSession ses;
        SqlDataSource ds(get_r(), ses);
        ds.row_data_vector2sql_rows(get_r().get_table("A"), v);
    }

    void test_insert_rows()
    {
        MockSqlSession ses;
        SqlDataSource ds(get_r(), ses);
        RowDataVector v;
        RowData x(get_r(), "A");
        x.set("X", Value(2));
        x.set("Y", Value("@"));
        v.push_back(x);
        RowData y(get_r(), "A");
        y.set("X", Value(4));
        y.set("Y", Value("$"));
        v.push_back(y);
        CPPUNIT_ASSERT_EQUAL(0U, ses.insert_cnt_);
        ds.insert_rows("A", v);
        CPPUNIT_ASSERT_EQUAL(2U, ses.insert_cnt_);
        CPPUNIT_ASSERT_EQUAL(2U, ses.stored_.size());
        Row row;
        row["X"] = Value(4);
        row["Y"] = Value("$");
        row["Z"] = Value();
        CPPUNIT_ASSERT(row == ses.stored_[1]);
        CPPUNIT_ASSERT_EQUAL(1U, ses.excluded_.size());
        CPPUNIT_ASSERT_EQUAL(string("Z"), *ses.excluded_.begin());
    }

    void test_update_rows()
    {
        MockSqlSession ses;
        SqlDataSource ds(get_r(), ses);
        RowDataVector v;
        RowData x(get_r(), "A");
        x.set("X", Value(2));
        x.set("Y", Value("@"));
        v.push_back(x);
        RowData y(get_r(), "A");
        y.set("X", Value(4));
        y.set("Y", Value("$"));
        v.push_back(y);
        CPPUNIT_ASSERT_EQUAL(0U, ses.update_cnt_);
        ds.update_rows("A", v);
        CPPUNIT_ASSERT_EQUAL(2U, ses.update_cnt_);
        CPPUNIT_ASSERT_EQUAL(2U, ses.stored_.size());
        Row row;
        row["X"] = Value(4);
        row["Y"] = Value("$");
        row["Z"] = Value();
        CPPUNIT_ASSERT(row == ses.stored_[1]);
        CPPUNIT_ASSERT_EQUAL(1U, ses.excluded_.size());
        CPPUNIT_ASSERT_EQUAL(string("Z"), *ses.excluded_.begin());
        CPPUNIT_ASSERT_EQUAL(1U, ses.keys_.size());
        CPPUNIT_ASSERT_EQUAL(string("X"), *ses.keys_.begin());
    }

    void test_seq()
    {
        MockSqlSession ses;
        SqlDataSource ds(get_r(), ses);
        long long x = ds.get_curr_id("S_A_X");
        long long y = ds.get_next_id("S_A_X");
        CPPUNIT_ASSERT_EQUAL(y, x + 1);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestSqlDataSource);

// vim:ts=4:sts=4:sw=4:et

