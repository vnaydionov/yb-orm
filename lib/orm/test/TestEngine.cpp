#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
#include "util/str_utils.hpp"
#include "orm/Engine.h"

using namespace std;
using namespace Yb;
using Yb::StrUtils::xgetenv;

class TestEngine : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestEngine);
    CPPUNIT_TEST(test_strlist_one);
    CPPUNIT_TEST(test_strlist_two);
    CPPUNIT_TEST(test_select_simple);
    CPPUNIT_TEST(test_select_for_update);
    CPPUNIT_TEST(test_select_where);
    CPPUNIT_TEST(test_select_groupby);
    CPPUNIT_TEST(test_select_having);
    CPPUNIT_TEST(test_select_orderby);
    CPPUNIT_TEST_EXCEPTION(test_select_having_wo_groupby, BadSQLOperation);
    CPPUNIT_TEST(test_insert_simple);
    CPPUNIT_TEST(test_insert_exclude);
    CPPUNIT_TEST_EXCEPTION(test_insert_empty_row, BadSQLOperation);
    CPPUNIT_TEST(test_update_where);
    CPPUNIT_TEST(test_update_combo);
    CPPUNIT_TEST_EXCEPTION(test_update_empty_row, BadSQLOperation);
    CPPUNIT_TEST_EXCEPTION(test_update_wo_clause, BadSQLOperation);
    CPPUNIT_TEST(test_delete);
    CPPUNIT_TEST_EXCEPTION(test_delete_wo_clause, BadSQLOperation);
    // Real DB tests
    CPPUNIT_TEST(test_insert_sql);
    CPPUNIT_TEST(test_update_sql);
    CPPUNIT_TEST(test_select_sql);
    CPPUNIT_TEST(test_select_sql_max_rows);
    CPPUNIT_TEST_EXCEPTION(test_insert_ro_mode, BadOperationInMode);
    CPPUNIT_TEST_EXCEPTION(test_update_ro_mode, BadOperationInMode);
    CPPUNIT_TEST_EXCEPTION(test_selforupdate_ro_mode, BadOperationInMode);
    CPPUNIT_TEST_EXCEPTION(test_commit_ro_mode, BadOperationInMode);
    CPPUNIT_TEST_EXCEPTION(test_delete_ro_mode, BadOperationInMode);
    CPPUNIT_TEST_EXCEPTION(test_execpoc_ro_mode, BadOperationInMode);
    CPPUNIT_TEST(test_force_select_for_update);
    CPPUNIT_TEST_SUITE_END();

    string db_type_;
    LongInt record_id_;

    LongInt get_next_test_id(Engine &engine, const string &seq_name)
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
    TestEngine()
        : db_type_(xgetenv("YBORM_DBTYPE"))
        , record_id_(0)
    {}

    void init_sql()
    {
        Engine engine(Engine::READ_ONLY);
        record_id_ = get_next_test_id(engine, "S_ORM_TEST_ID");
        CPPUNIT_ASSERT(record_id_ > 0);
        ostringstream sql;
        sql << "INSERT INTO T_ORM_TEST(ID, A, B, C) VALUES(?, ?, ?, ?)";
        Values params;
        params.push_back(Value(record_id_));
        params.push_back(Value("item"));
        params.push_back(Value(now()));
        params.push_back(Value(Decimal("1.2")));
        SqlConnect conn("ODBC", xgetenv("YBORM_DBTYPE"),
                xgetenv("YBORM_DB"), xgetenv("YBORM_USER"), xgetenv("YBORM_PASSWD"));
        conn.exec_direct("DELETE FROM T_ORM_XML");
        conn.exec_direct("DELETE FROM T_ORM_TEST");
        conn.prepare(sql.str());
        conn.exec(params);
        conn.commit();
    }

    void finish_sql()
    {
        SqlConnect conn("ODBC", xgetenv("YBORM_DBTYPE"),
                xgetenv("YBORM_DB"), xgetenv("YBORM_USER"), xgetenv("YBORM_PASSWD"));
        conn.exec_direct("DELETE FROM T_ORM_TEST");
        conn.commit();
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
        Engine engine(Engine::READ_ONLY);
        string sql;
        Values params;
        engine.do_gen_sql_select(sql, params, "A, B", "T",
                Filter(), StrList(), Filter(), StrList(), false);
        CPPUNIT_ASSERT_EQUAL(string("SELECT A, B FROM T"), sql);
    }

    void test_select_for_update()
    {
        Engine engine(Engine::READ_ONLY);
        string sql;
        Values params;
        engine.do_gen_sql_select(sql, params, "1", "T",
                Filter(), StrList(), Filter(), StrList(), true);
        CPPUNIT_ASSERT_EQUAL(string("SELECT 1 FROM T FOR UPDATE"), sql);
    }

    void test_select_where()
    {
        Engine engine(Engine::READ_ONLY);
        string sql;
        Values params;
        engine.do_gen_sql_select(sql, params, "*", "T",
                filter_eq("ID", Value(1)),
                StrList(), Filter(), StrList(), false);
        CPPUNIT_ASSERT_EQUAL(string("SELECT * FROM T WHERE ID = ?"), sql);
        CPPUNIT_ASSERT_EQUAL(1, (int)params.size());
        CPPUNIT_ASSERT_EQUAL(1, (int)params[0].as_longint());
    }

    void test_select_groupby()
    {
        Engine engine(Engine::READ_ONLY);
        string sql;
        Values params;
        engine.do_gen_sql_select(sql, params, "A, B", "T",
                Filter(), "A, B", Filter(), StrList(), false);
        CPPUNIT_ASSERT_EQUAL(string("SELECT A, B FROM T GROUP BY A, B"), sql);
    }

    void test_select_having()
    {
        Engine engine(Engine::READ_ONLY);
        string sql;
        Values params;
        engine.do_gen_sql_select(sql, params, "A, COUNT(*)", "T",
                Filter(), "A", Filter("COUNT(*) > 2"), StrList(), false);
        CPPUNIT_ASSERT_EQUAL(string("SELECT A, COUNT(*) FROM T GROUP BY A HAVING COUNT(*) > 2"), sql);
    }

    void test_select_having_wo_groupby()
    {
        Engine engine(Engine::READ_ONLY);
        string sql;
        Values params;
        engine.do_gen_sql_select(sql, params, "A, B", "T", Filter(), "",
                filter_eq("ID", Value(1)), StrList(), false);
    }

    void test_select_orderby()
    {
        Engine engine(Engine::READ_ONLY);
        string sql;
        Values params;
        engine.do_gen_sql_select(sql, params, "A, B", "T", Filter(), "",
                Filter(), "A", false);
        CPPUNIT_ASSERT_EQUAL(string("SELECT A, B FROM T ORDER BY A"), sql);
    }

    void test_insert_simple()
    {
        Engine engine(Engine::READ_ONLY);
        string sql;
        Row row;
        row.insert(make_pair(string("ID"), Value(1)));
        row.insert(make_pair(string("A"), Value("a")));
        Values params;
        ParamNums param_nums;
        engine.do_gen_sql_insert(sql, params, param_nums, "T", row, FieldSet());
        CPPUNIT_ASSERT_EQUAL(string("INSERT INTO T (A, ID) VALUES (?, ?)"), sql);
        CPPUNIT_ASSERT_EQUAL(2, (int)params.size());
        CPPUNIT_ASSERT_EQUAL(string("a"), params[0].as_string());
        CPPUNIT_ASSERT_EQUAL(1, (int)params[1].as_longint());
    }

    void test_insert_exclude()
    {
        Engine engine(Engine::READ_ONLY);
        string sql;
        Row row;
        row.insert(make_pair(string("ID"), Value(1)));
        row.insert(make_pair(string("A"), Value("a")));
        FieldSet exclude;
        exclude.insert("ID");
        Values params;
        ParamNums param_nums;
        engine.do_gen_sql_insert(sql, params, param_nums, "T", row, exclude);
        CPPUNIT_ASSERT_EQUAL(string("INSERT INTO T (A) VALUES (?)"), sql);
        CPPUNIT_ASSERT(1 == params.size() && Value("a") == params[0]);
    }

    void test_insert_empty_row()
    {
        Engine engine(Engine::READ_ONLY);
        string sql;
        Values params;
        ParamNums param_nums;
        engine.do_gen_sql_insert(sql, params, param_nums, "A", Row(), FieldSet());
    }

    void test_update_where()
    {
        Engine engine(Engine::READ_ONLY);
        string sql;
        Row row;
        row["A"] = Value("a");
        Values params;
        ParamNums param_nums;
        engine.do_gen_sql_update(sql, params, param_nums,
                "T", row, FieldSet(), FieldSet(), filter_eq("B", Value("b")));
        CPPUNIT_ASSERT_EQUAL(string("UPDATE T SET A = ? WHERE (B = ?)"), sql);
        CPPUNIT_ASSERT(2 == params.size()
                && Value("a") == params[0] && Value("b") == params[1]);
        CPPUNIT_ASSERT(1 == param_nums.size() && 0 == param_nums["A"]);
    }

    void test_update_combo()
    {
        Engine engine(Engine::READ_ONLY);
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
        Values params;
        ParamNums param_nums;
        engine.do_gen_sql_update(sql, params, param_nums,
                "T", row, key, exclude, filter_eq("Q", Value("q")));
        CPPUNIT_ASSERT_EQUAL(string(
                    "UPDATE T SET E = ?, F = ? WHERE (Q = ?) AND (B = ?) AND (D = ?)"), sql);
        CPPUNIT_ASSERT(5 == params.size() && Value("e") == params[0] &&
                Value(2) == params[1] && Value("q") == params[2] &&
                Value(1) == params[3] && Value("d") == params[4]);
        CPPUNIT_ASSERT(4 == param_nums.size());
        CPPUNIT_ASSERT(0 == param_nums["E"] && 1 == param_nums["F"] &&
                3 == param_nums["B"] && 4 == param_nums["D"]);
    }

    void test_update_empty_row()
    {
        Engine engine(Engine::READ_ONLY);
        string sql;
        Values params;
        ParamNums param_nums;
        engine.do_gen_sql_update(sql, params, param_nums,
                "A", Row(), FieldSet(), FieldSet(), Filter());
    }

    void test_update_wo_clause()
    {
        Engine engine(Engine::READ_ONLY);
        string sql;
        Row row;
        row["A"] = Value("a");
        Values params;
        ParamNums param_nums;
        engine.do_gen_sql_update(sql, params, param_nums,
                "T", row, FieldSet(), FieldSet(), Filter());
    }

    void test_delete()
    {
        Engine engine(Engine::READ_ONLY);
        string sql;
        Values params;
        engine.do_gen_sql_delete(sql, params, "T", filter_eq("ID", Value(1)));
        CPPUNIT_ASSERT_EQUAL(string("DELETE FROM T WHERE ID = ?"), sql);
        CPPUNIT_ASSERT(1 == params.size() && Value(1) == params[0]);
    }

    void test_delete_wo_clause()
    {
        Engine engine(Engine::READ_ONLY);
        string sql;
        Values params;
        engine.do_gen_sql_delete(sql, params, "T", Filter());
    }

    void test_select_sql()
    {
        Engine engine(Engine::READ_ONLY);
        CPPUNIT_ASSERT_EQUAL(false, engine.touched_);
        init_sql();
        RowsPtr ptr = engine.select("*", "T_ORM_TEST", filter_eq("ID", Value(record_id_)));
        CPPUNIT_ASSERT_EQUAL(false, engine.touched_);
        CPPUNIT_ASSERT_EQUAL(1, (int)ptr->size());
        CPPUNIT_ASSERT_EQUAL(string("item"), (ptr->begin())->find("A")->second.as_string());
        CPPUNIT_ASSERT_EQUAL(Decimal("1.2"), (ptr->begin())->find("C")->second.as_decimal());
        finish_sql();
    }

    void test_select_sql_max_rows()
    {
        Engine engine(Engine::READ_ONLY);
        CPPUNIT_ASSERT_EQUAL(false, engine.touched_);
        init_sql();
        RowsPtr ptr = engine.select("*", "T_ORM_TEST", filter_eq("ID", Value(record_id_)),
                StrList(), Filter(), StrList(), 0);
        CPPUNIT_ASSERT_EQUAL(false, engine.touched_);
        CPPUNIT_ASSERT_EQUAL(0, (int)ptr->size());
        finish_sql();
    }

    void test_insert_sql()
    {
        init_sql();
        Engine engine(Engine::MANUAL);
        CPPUNIT_ASSERT_EQUAL(false, engine.touched_);
        Rows rows;
        LongInt id = get_next_test_id(engine, "S_ORM_TEST_ID");
        Row row;
        row.insert(make_pair(string("ID"), Value(id)));
        row.insert(make_pair(string("A"), Value("inserted")));
        row.insert(make_pair(string("B"), Value(now())));
        row.insert(make_pair(string("C"), Value(Decimal("1.1"))));
        rows.push_back(row);
        engine.insert("T_ORM_TEST", rows);
        CPPUNIT_ASSERT_EQUAL(true, engine.touched_);
        RowsPtr ptr = engine.select("*", "T_ORM_TEST", filter_eq("ID", Value(id)));
        CPPUNIT_ASSERT_EQUAL(1, (int)ptr->size());
        CPPUNIT_ASSERT_EQUAL(string("inserted"), (ptr->begin())->find("A")->second.as_string());
        CPPUNIT_ASSERT(Decimal("1.1") == (ptr->begin())->find("C")->second.as_decimal());
        engine.commit();
        finish_sql();
    }

    void test_update_sql()
    {
        init_sql();
        Engine engine(Engine::MANUAL);
        CPPUNIT_ASSERT_EQUAL(false, engine.touched_);
        Rows rows;
        LongInt id = get_next_test_id(engine, "S_ORM_TEST_ID");
        Row row;
        row["A"] = Value("updated");
        row["C"] = Value(Decimal("1.3"));
        row["ID"] = Value(record_id_);
        rows.push_back(row);
        FieldSet key, exclude;
        key.insert("ID");
        exclude.insert("B");
        engine.update("T_ORM_TEST", rows, key, exclude);
        CPPUNIT_ASSERT_EQUAL(true, engine.touched_);
        RowsPtr ptr = engine.select("*", "T_ORM_TEST", filter_eq("ID", Value(record_id_)));
        CPPUNIT_ASSERT_EQUAL(1, (int)ptr->size());
        CPPUNIT_ASSERT_EQUAL(string("updated"), (ptr->begin())->find("A")->second.as_string());
        CPPUNIT_ASSERT_EQUAL(Decimal("1.3"), (ptr->begin())->find("C")->second.as_decimal());
        engine.commit();
        finish_sql();
    }
    
    void test_insert_ro_mode()
    {
        Engine engine(Engine::READ_ONLY);
        engine.insert("", Rows());
    }
    
    void test_update_ro_mode()
    {
        Engine engine(Engine::READ_ONLY);
        engine.update("", Rows(), FieldSet(), FieldSet());
    }
    
    void test_selforupdate_ro_mode()
    {
        Engine engine(Engine::READ_ONLY);
        engine.select("", "", Filter(), StrList(), Filter(), StrList(), -1, true);
    }
    
    void test_commit_ro_mode()
    {
        Engine engine(Engine::READ_ONLY);
        engine.commit();
    }
    
    void test_delete_ro_mode()
    {
        Engine engine(Engine::READ_ONLY);
        engine.delete_from("", Filter());
    }
    
    void test_execpoc_ro_mode()
    {
        Engine engine(Engine::READ_ONLY);
        engine.exec_proc("");
    }

    void test_force_select_for_update()
    {
        Engine engine(Engine::FORCE_SELECT_UPDATE);
        CPPUNIT_ASSERT_EQUAL(false, engine.touched_);
        RowsPtr ptr = engine.select("*", "T_ORM_TEST", Filter());
        CPPUNIT_ASSERT_EQUAL(true, engine.touched_);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestEngine);

class TestSqlDriver : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestSqlDriver);
    CPPUNIT_TEST(test_fetch);
    CPPUNIT_TEST_SUITE_END();

public:
    void test_fetch()
    {
        SqlConnect conn("ODBC", xgetenv("YBORM_DBTYPE"),
                xgetenv("YBORM_DB"), xgetenv("YBORM_USER"),
                xgetenv("YBORM_PASSWD"));
        ///
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestSqlDriver);

// vim:ts=4:sts=4:sw=4:et:
