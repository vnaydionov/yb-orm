
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>

#include "util/str_utils.hpp"
#include "orm/DbpoolSession.h"

using namespace std;
using namespace Yb::SQL;
using Yb::Value;
using Yb::StrUtils::xgetenv;

class TestSession : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestSession);
    CPPUNIT_TEST(test_strlist_one);
    CPPUNIT_TEST(test_strlist_two);
    CPPUNIT_TEST(test_select_simple);
    CPPUNIT_TEST(test_select_for_update);
    CPPUNIT_TEST(test_select_where);
    CPPUNIT_TEST(test_select_groupby);
    CPPUNIT_TEST(test_select_having);
    CPPUNIT_TEST(test_select_orderby);
    CPPUNIT_TEST_EXCEPTION(test_select_having_wo_groupby, Yb::SQL::BadSQLOperation);
    CPPUNIT_TEST(test_insert_simple);
    CPPUNIT_TEST(test_insert_exclude);
    CPPUNIT_TEST_EXCEPTION(test_insert_empty_row, Yb::SQL::BadSQLOperation);
    CPPUNIT_TEST(test_update_where);
    CPPUNIT_TEST(test_update_combo);
    CPPUNIT_TEST_EXCEPTION(test_update_empty_row, Yb::SQL::BadSQLOperation);
    CPPUNIT_TEST_EXCEPTION(test_update_wo_clause, Yb::SQL::BadSQLOperation);
    CPPUNIT_TEST(test_delete);
    CPPUNIT_TEST_EXCEPTION(test_delete_wo_clause, Yb::SQL::BadSQLOperation);
    // Real DB tests
    CPPUNIT_TEST(test_insert_sql);
    CPPUNIT_TEST(test_update_sql);
    CPPUNIT_TEST(test_select_sql);
    CPPUNIT_TEST(test_select_sql_max_rows);
    CPPUNIT_TEST_EXCEPTION(test_insert_ro_mode, Yb::SQL::BadOperationInMode);
    CPPUNIT_TEST_EXCEPTION(test_update_ro_mode, Yb::SQL::BadOperationInMode);
    CPPUNIT_TEST_EXCEPTION(test_selforupdate_ro_mode, Yb::SQL::BadOperationInMode);
    CPPUNIT_TEST_EXCEPTION(test_commit_ro_mode, Yb::SQL::BadOperationInMode);
    CPPUNIT_TEST_EXCEPTION(test_delete_ro_mode, Yb::SQL::BadOperationInMode);
    CPPUNIT_TEST_EXCEPTION(test_execpoc_ro_mode, Yb::SQL::BadOperationInMode);
    CPPUNIT_TEST(test_force_select_for_update);
    CPPUNIT_TEST_SUITE_END();

    long long record_id_;

public:
    void init_sql()
    {
        {
            DBPoolSession ses;
            record_id_ = ses.get_next_value("S_ORM_TEST_ID");
        }
        CPPUNIT_ASSERT(record_id_ > 0);
        ostringstream sql;
        sql << "INSERT INTO T_ORM_TEST(ID, A, B, C) VALUES("
            << Value(record_id_).sql_str() << ", "
            << "'item', "
            << Value((boost::posix_time::second_clock::local_time())).sql_str() << ", "
            << Value(decimal("1.2")).sql_str() << ")";
        OdbcDriver drv;
        drv.open(xgetenv("YBORM_DB"), xgetenv("YBORM_USER"), xgetenv("YBORM_PASSWD"));
        drv.exec_direct("DELETE FROM T_ORM_TEST");
        drv.exec_direct(sql.str());
        drv.commit();
    }

    void finish_sql()
    {
        OdbcDriver drv;
        drv.open(xgetenv("YBORM_DB"), xgetenv("YBORM_USER"), xgetenv("YBORM_PASSWD"));
        drv.exec_direct("DELETE FROM T_ORM_TEST");
        drv.commit();
    }

    void test_strlist_one()
    {
        FieldSet fields;
        fields.insert("A");
        StrList list(fields);
        CPPUNIT_ASSERT_EQUAL(string("A"), list.get_str());
    }

    void test_strlist_two()
    {
        FieldSet fields;
        fields.insert("B");
        fields.insert("A");
        StrList list(fields);
        CPPUNIT_ASSERT_EQUAL(string("A, B"), list.get_str());
    }

    void test_select_simple()
    {
        DBPoolSession ses;
        string sql;
        ses.do_gen_sql_select(sql, "A, B", "T",
                Filter(), StrList(), Filter(), StrList(), false);
        CPPUNIT_ASSERT_EQUAL(string("SELECT A, B FROM T"), sql);
    }

    void test_select_for_update()
    {
        DBPoolSession ses;
        string sql;
        ses.do_gen_sql_select(sql, "1", "T",
                Filter(), StrList(), Filter(), StrList(), true);
        CPPUNIT_ASSERT_EQUAL(string("SELECT 1 FROM T FOR UPDATE"), sql);
    }

    void test_select_where()
    {
        DBPoolSession ses;
        string sql;
        ses.do_gen_sql_select(sql, "*", "T",
                filter_eq("ID", Value(1)),
                StrList(), Filter(), StrList(), false);
        CPPUNIT_ASSERT_EQUAL(string("SELECT * FROM T WHERE ID = 1"), sql);
    }

    void test_select_groupby()
    {
        DBPoolSession ses;
        string sql;
        ses.do_gen_sql_select(sql, "A, B", "T",
                Filter(), "A, B", Filter(), StrList(), false);
        CPPUNIT_ASSERT_EQUAL(string("SELECT A, B FROM T GROUP BY A, B"), sql);
    }

    void test_select_having()
    {
        DBPoolSession ses;
        string sql;
        ses.do_gen_sql_select(sql, "A, COUNT(*)", "T",
                Filter(), "A", Filter("COUNT(*) > 2"), StrList(), false);
        CPPUNIT_ASSERT_EQUAL(string("SELECT A, COUNT(*) FROM T GROUP BY A HAVING COUNT(*) > 2"), sql);
    }

    void test_select_having_wo_groupby()
    {
        DBPoolSession ses;
        string sql;
        ses.do_gen_sql_select(sql, "A, B", "T", Filter(), "",
                filter_eq("ID", Value(1)), StrList(), false);
    }

    void test_select_orderby()
    {
        DBPoolSession ses;
        string sql;
        ses.do_gen_sql_select(sql, "A, B", "T", Filter(), "",
                Filter(), "A", false);
        CPPUNIT_ASSERT_EQUAL(string("SELECT A, B FROM T ORDER BY A"), sql);
    }

    void test_insert_simple()
    {
        DBPoolSession ses;
        string sql;
        Row row;
        row.insert(make_pair(string("ID"), Value(1)));
        row.insert(make_pair(string("A"), Value("a")));
        ses.do_gen_sql_insert(sql, "T", row, FieldSet());
        CPPUNIT_ASSERT_EQUAL(string("INSERT INTO T (A, ID) VALUES ('a', 1)"), sql);
    }

    void test_insert_exclude()
    {
        DBPoolSession ses;
        string sql;
        Row row;
        row.insert(make_pair(string("ID"), Value(1)));
        row.insert(make_pair(string("A"), Value("a")));
        FieldSet exclude;
        exclude.insert("ID");
        ses.do_gen_sql_insert(sql, "T", row, exclude);
        CPPUNIT_ASSERT_EQUAL(string("INSERT INTO T (A) VALUES ('a')"), sql);
    }

    void test_insert_empty_row()
    {
        DBPoolSession ses;
        string sql;
        ses.do_gen_sql_insert(sql, "A", Row(), FieldSet());
    }

    void test_update_where()
    {
        DBPoolSession ses;
        string sql;
        Row row;
        row["A"] = Value("a");
        ses.do_gen_sql_update(sql, "T", row, FieldSet(), FieldSet(), filter_eq("A", Value("b")));
        CPPUNIT_ASSERT_EQUAL(string("UPDATE T SET A = 'a' WHERE (A = 'b')"), sql);
    }

    void test_update_combo()
    {
        DBPoolSession ses;
        string sql;
        Row row;
        row["A"] = Value("a");
        row["B"] = Value(1);
        row["C"] = Value("c");
        row["D"] = Value("d");
        row["E"] = Value("e");
        row["F"] = Value(2);
        FieldSet key, exclude;
        key.insert("B");
        key.insert("D");
        exclude.insert("A");
        exclude.insert("C");
        ses.do_gen_sql_update(sql, "T", row, key, exclude, Filter());
        CPPUNIT_ASSERT_EQUAL(string(
                    "UPDATE T SET E = 'e', F = 2 WHERE (B = 1) AND (D = 'd')"), sql);
    }

    void test_update_empty_row()
    {
        DBPoolSession ses;
        string sql;
        ses.do_gen_sql_update(sql, "A", Row(), FieldSet(), FieldSet(), Filter());
    }

    void test_update_wo_clause()
    {
        DBPoolSession ses;
        string sql;
        Row row;
        row["A"] = Value("a");
        ses.do_gen_sql_update(sql, "T", row, FieldSet(), FieldSet(), Filter());
    }

    void test_delete()
    {
        DBPoolSession ses;
        string sql;
        ses.do_gen_sql_delete(sql, "T", filter_eq("ID", Value(1)));
        CPPUNIT_ASSERT_EQUAL(string("DELETE FROM T WHERE ID = 1"), sql);
    }

    void test_delete_wo_clause()
    {
        DBPoolSession ses;
        string sql;
        ses.do_gen_sql_delete(sql, "T", Filter());
    }

    void test_select_sql()
    {
        DBPoolSession ses;
        CPPUNIT_ASSERT_EQUAL(false, ses.touched_);
        init_sql();
        RowsPtr ptr = ses.select("*", "T_ORM_TEST", filter_eq("ID", Value(record_id_)));
        CPPUNIT_ASSERT_EQUAL(false, ses.touched_);
        CPPUNIT_ASSERT_EQUAL(1U, ptr->size());
        CPPUNIT_ASSERT_EQUAL(string("item"), (ptr->begin())->find("A")->second.as_string());
        CPPUNIT_ASSERT_EQUAL(string("1.2"), (ptr->begin())->find("C")->second.as_string());
        finish_sql();
    }

    void test_select_sql_max_rows()
    {
        DBPoolSession ses;
        CPPUNIT_ASSERT_EQUAL(false, ses.touched_);
        init_sql();
        RowsPtr ptr = ses.select("*", "T_ORM_TEST", filter_eq("ID", Value(record_id_)),
                StrList(), Filter(), StrList(), 0);
        CPPUNIT_ASSERT_EQUAL(false, ses.touched_);
        CPPUNIT_ASSERT_EQUAL(0U, ptr->size());
        finish_sql();
    }

    void test_insert_sql()
    {
        DBPoolSession ses(Session::MANUAL);
        CPPUNIT_ASSERT_EQUAL(false, ses.touched_);
        Rows rows;
        long long id = ses.get_next_value("S_ORM_TEST_ID");
        Row row;
        row.insert(make_pair(string("ID"), Value(id)));
        row.insert(make_pair(string("A"), Value("inserted")));
        row.insert(make_pair(string("B"), Value(boost::posix_time::second_clock::local_time())));
        row.insert(make_pair(string("C"), Value(decimal("1.1"))));
        rows.push_back(row);
        ses.insert("T_ORM_TEST", rows);
        CPPUNIT_ASSERT_EQUAL(true, ses.touched_);
        RowsPtr ptr = ses.select("*", "T_ORM_TEST", filter_eq("ID", Value(id)));
        CPPUNIT_ASSERT_EQUAL(1U, ptr->size());
        CPPUNIT_ASSERT_EQUAL(string("inserted"), (ptr->begin())->find("A")->second.as_string());
        CPPUNIT_ASSERT_EQUAL(string("1.1"), (ptr->begin())->find("C")->second.as_string());
        ses.commit();
        finish_sql();
    }

    void test_update_sql()
    {
        init_sql();
        DBPoolSession ses(Session::MANUAL);
        CPPUNIT_ASSERT_EQUAL(false, ses.touched_);
        Rows rows;
        long long id = ses.get_next_value("S_ORM_TEST_ID");
        Row row;
        row["A"] = Value("updated");
        row["C"] = Value(decimal("1.3"));
        row["ID"] = Value(record_id_);
        rows.push_back(row);
        FieldSet key, exclude;
        key.insert("ID");
        exclude.insert("B");
        ses.update("T_ORM_TEST", rows, key, exclude);
        CPPUNIT_ASSERT_EQUAL(true, ses.touched_);
        RowsPtr ptr = ses.select("*", "T_ORM_TEST", filter_eq("ID", Value(record_id_)));
        CPPUNIT_ASSERT_EQUAL(1U, ptr->size());
        CPPUNIT_ASSERT_EQUAL(string("updated"), (ptr->begin())->find("A")->second.as_string());
        CPPUNIT_ASSERT_EQUAL(string("1.3"), (ptr->begin())->find("C")->second.as_string());
        ses.commit();
        finish_sql();
    }
    
    void test_insert_ro_mode()
    {
        DBPoolSession ses;
        ses.insert("", Rows());
    }
    
    void test_update_ro_mode()
    {
        DBPoolSession ses;
        ses.update("", Rows(), FieldSet(), FieldSet());
    }
    
    void test_selforupdate_ro_mode()
    {
        DBPoolSession ses;
        ses.select("", "", Filter(), StrList(), Filter(), StrList(), -1, true);
    }
    
    void test_commit_ro_mode()
    {
        DBPoolSession ses;
        ses.commit();
    }
    
    void test_delete_ro_mode()
    {
        DBPoolSession ses;
        ses.delete_from("", Filter());
    }
    
    void test_execpoc_ro_mode()
    {
        DBPoolSession ses;
        ses.exec_proc("");
    }

    void test_force_select_for_update()
    {
        DBPoolSession ses(Session::FORCE_SELECT_UPDATE);
        CPPUNIT_ASSERT_EQUAL(false, ses.touched_);
        RowsPtr ptr = ses.select("*", "T_ORM_TEST", Filter());
        CPPUNIT_ASSERT_EQUAL(true, ses.touched_);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestSession);

// vim:ts=4:sts=4:sw=4:et

