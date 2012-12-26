#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
#include <util/str_utils.hpp>
#include <orm/Engine.h>

using namespace std;
using namespace Yb;

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
    CPPUNIT_TEST_EXCEPTION(test_insert_ro_mode, BadOperationInMode);
    CPPUNIT_TEST_EXCEPTION(test_update_ro_mode, BadOperationInMode);
    CPPUNIT_TEST_EXCEPTION(test_delete_ro_mode, BadOperationInMode);
    CPPUNIT_TEST_EXCEPTION(test_execpoc_ro_mode, BadOperationInMode);
    CPPUNIT_TEST_SUITE_END();

public:
    void test_strlist_one()
    {
        StringSet fields;
        fields.insert(_T("A"));
        ExpressionList list(fields);
        CPPUNIT_ASSERT_EQUAL(string("A"), NARROW(list.get_sql()));
    }

    void test_strlist_two()
    {
        StringSet fields;
        fields.insert(_T("B"));
        fields.insert(_T("A"));
        ExpressionList list(fields);
        CPPUNIT_ASSERT_EQUAL(string("A, B"), NARROW(list.get_sql()));
    }

    void test_select_simple()
    {
        String sql;
        Values params;
        sql = SelectExpr(Expression(_T("A, B")))
            .from_(Expression(_T("T"))).generate_sql(&params);
        CPPUNIT_ASSERT_EQUAL(string("SELECT A, B FROM T"), NARROW(sql));
    }

    void test_select_for_update()
    {
        String sql;
        Values params;
        sql = SelectExpr(Expression(_T("1")))
            .from_(Expression(_T("T"))).for_update(true).generate_sql(&params);
        CPPUNIT_ASSERT_EQUAL(string("SELECT 1 FROM T FOR UPDATE"), NARROW(sql));
    }

    void test_select_where()
    {
        String sql;
        Values params;
        sql = SelectExpr(Expression(_T("*")))
            .from_(Expression(_T("T")))
            .where_(Expression(_T("ID")) == 1).generate_sql(&params);
        CPPUNIT_ASSERT_EQUAL(string("SELECT * FROM T WHERE ID = ?"), NARROW(sql));
        CPPUNIT_ASSERT_EQUAL(1, (int)params.size());
        CPPUNIT_ASSERT_EQUAL(1, (int)params[0].as_longint());
    }

    void test_select_groupby()
    {
        String sql;
        Values params;
        sql = SelectExpr(Expression(_T("A, B")))
            .from_(Expression(_T("T")))
            .group_by_(ExpressionList(Expression(_T("A")), Expression(_T("B"))))
            .generate_sql(&params);
        CPPUNIT_ASSERT_EQUAL(string("SELECT A, B FROM T GROUP BY A, B"), NARROW(sql));
    }

    void test_select_having()
    {
        String sql;
        Values params;
        sql = SelectExpr(Expression(_T("A, COUNT(*)")))
            .from_(Expression(_T("T")))
            .group_by_(Expression(_T("A")))
            .having_(Expression(_T("COUNT(*) > 2"))).generate_sql(&params);
        CPPUNIT_ASSERT_EQUAL(
                string("SELECT A, COUNT(*) FROM T GROUP BY A HAVING COUNT(*) > 2"),
                NARROW(sql));
    }

    void test_select_having_wo_groupby()
    {
        Engine engine(Engine::READ_ONLY);
        String sql;
        Values params;
        sql = SelectExpr(Expression(_T("A, B")))
            .from_(Expression(_T("T")))
            .having_(Expression(_T("ID")) == 1).generate_sql(&params);
    }

    void test_select_orderby()
    {
        Engine engine(Engine::READ_ONLY);
        String sql;
        Values params;
        sql = SelectExpr(Expression(_T("A, B")))
            .from_(Expression(_T("T")))
            .order_by_(Expression(_T("A"))).generate_sql(&params);
        CPPUNIT_ASSERT_EQUAL(string("SELECT A, B FROM T ORDER BY A"), NARROW(sql));
    }

    void test_insert_simple()
    {
        Engine engine(Engine::READ_ONLY);
        String sql;
        Row row;
        row.push_back(make_pair(String(_T("ID")), Value(1)));
        row.push_back(make_pair(String(_T("A")), Value(_T("a"))));
        Values params;
        ParamNums param_nums;
        engine.do_gen_sql_insert(sql, params, param_nums, _T("T"), row, StringSet());
        CPPUNIT_ASSERT_EQUAL(string("INSERT INTO T (ID, A) VALUES (?, ?)"), NARROW(sql));
        CPPUNIT_ASSERT_EQUAL(2, (int)params.size());
        CPPUNIT_ASSERT_EQUAL(1, (int)params[0].as_longint());
        CPPUNIT_ASSERT_EQUAL(string("a"), NARROW(params[1].as_string()));
    }

    void test_insert_exclude()
    {
        Engine engine(Engine::READ_ONLY);
        String sql;
        Row row;
        row.push_back(make_pair(String(_T("ID")), Value(1)));
        row.push_back(make_pair(String(_T("A")), Value(_T("a"))));
        StringSet exclude;
        exclude.insert(_T("ID"));
        Values params;
        ParamNums param_nums;
        engine.do_gen_sql_insert(sql, params, param_nums, _T("T"), row, exclude);
        CPPUNIT_ASSERT_EQUAL(string("INSERT INTO T (A) VALUES (?)"), NARROW(sql));
        CPPUNIT_ASSERT(1 == params.size() && Value(_T("a")) == params[0]);
    }

    void test_insert_empty_row()
    {
        Engine engine(Engine::READ_ONLY);
        String sql;
        Values params;
        ParamNums param_nums;
        engine.do_gen_sql_insert(sql, params, param_nums, _T("A"), Row(), StringSet());
    }

    void test_update_where()
    {
        Engine engine(Engine::READ_ONLY);
        String sql;
        Row row;
        row.push_back(make_pair(String(_T("A")), Value(_T("a"))));
        Values params;
        ParamNums param_nums;
        engine.do_gen_sql_update(sql, params, param_nums,
                _T("T"), row, Strings(), StringSet(), Expression(_T("B")) == String(_T("b")));
        CPPUNIT_ASSERT_EQUAL(string("UPDATE T SET A = ? WHERE (B = ?)"), NARROW(sql));
        CPPUNIT_ASSERT(2 == params.size()
                && Value(_T("a")) == params[0] && Value(_T("b")) == params[1]);
        CPPUNIT_ASSERT(1 == param_nums.size() && 0 == param_nums[_T("A")]);
    }

    void test_update_combo()
    {
        Engine engine(Engine::READ_ONLY);
        String sql;
        Row row;
        row.push_back(make_pair(String(_T("A")), Value(_T("a"))));
        row.push_back(make_pair(String(_T("B")), Value(1)));
        row.push_back(make_pair(String(_T("C")), Value(_T("c"))));
        row.push_back(make_pair(String(_T("D")), Value(_T("d"))));
        row.push_back(make_pair(String(_T("E")), Value(_T("e"))));
        row.push_back(make_pair(String(_T("F")), Value(2)));
        Strings key;
        key.push_back(_T("B"));
        key.push_back(_T("D"));
        StringSet exclude;
        exclude.insert(_T("A"));
        exclude.insert(_T("C"));
        Values params;
        ParamNums param_nums;
        engine.do_gen_sql_update(sql, params, param_nums,
                _T("T"), row, key, exclude, Expression(_T("Q")) == String(_T("q")));
        CPPUNIT_ASSERT_EQUAL(string(
                "UPDATE T SET E = ?, F = ? WHERE (Q = ?) AND (B = ?) AND (D = ?)"), NARROW(sql));
        CPPUNIT_ASSERT(5 == params.size() && Value(_T("e")) == params[0] &&
                Value(2) == params[1] && Value(_T("q")) == params[2] &&
                Value(1) == params[3] && Value(_T("d")) == params[4]);
        CPPUNIT_ASSERT(4 == param_nums.size());
        CPPUNIT_ASSERT(0 == param_nums[_T("E")] && 1 == param_nums[_T("F")] &&
                3 == param_nums[_T("B")] && 4 == param_nums[_T("D")]);
    }

    void test_update_empty_row()
    {
        Engine engine(Engine::READ_ONLY);
        String sql;
        Values params;
        ParamNums param_nums;
        engine.do_gen_sql_update(sql, params, param_nums,
                _T("A"), Row(), Strings(), StringSet(), Filter());
    }

    void test_update_wo_clause()
    {
        Engine engine(Engine::READ_ONLY);
        String sql;
        Row row;
        row.push_back(make_pair(String(_T("A")), Value(_T("a"))));
        Values params;
        ParamNums param_nums;
        engine.do_gen_sql_update(sql, params, param_nums,
                _T("T"), row, Strings(), StringSet(), Filter());
    }

    void test_delete()
    {
        Engine engine(Engine::READ_ONLY);
        String sql;
        Values params;
        engine.do_gen_sql_delete(sql, params, _T("T"), Expression(_T("ID")) == 1);
        CPPUNIT_ASSERT_EQUAL(string("DELETE FROM T WHERE ID = ?"), NARROW(sql));
        CPPUNIT_ASSERT(1 == params.size() && Value(1) == params[0]);
    }

    void test_delete_wo_clause()
    {
        Engine engine(Engine::READ_ONLY);
        String sql;
        Values params;
        engine.do_gen_sql_delete(sql, params, _T("T"), Filter());
    }

    void test_insert_ro_mode()
    {
        Engine engine(Engine::READ_ONLY);
        engine.insert(_T(""), Rows());
    }
    
    void test_update_ro_mode()
    {
        Engine engine(Engine::READ_ONLY);
        engine.update(_T(""), Rows(), Strings(), StringSet());
    }
    
    void test_delete_ro_mode()
    {
        Engine engine(Engine::READ_ONLY);
        engine.delete_from(_T(""), Filter());
    }
    
    void test_execpoc_ro_mode()
    {
        Engine engine(Engine::READ_ONLY);
        engine.exec_proc(_T(""));
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestEngine);

// Real DB tests

static LogAppender appender(cerr);
static ILogger::Ptr root_logger;

static void init_log()
{
    if (!root_logger.get())
        root_logger.reset(new Logger(&appender));
}

static void setup_log(Engine &e)
{
    init_log();
    e.set_logger(root_logger->new_logger("engine"));
    e.set_echo(true);
}

static void setup_log(SqlConnection &c)
{
    init_log();
    c.init_logger(root_logger.get());
    c.set_echo(true);
}

static LongInt get_next_test_id(SqlConnection *conn)
{
    CPPUNIT_ASSERT(conn != NULL);
    std::auto_ptr<SqlCursor> cur = conn->new_cursor();
    cur->prepare(_T("SELECT MAX(ID) MAX_ID FROM T_ORM_TEST"));
    cur->exec(Values());
    RowPtr ptr = cur->fetch_row();
    CPPUNIT_ASSERT(ptr.get() != NULL);
    RowPtr end = cur->fetch_row();
    CPPUNIT_ASSERT(end.get() == NULL);
    Value x = find_in_row(*ptr, _T("MAX_ID"))->second;
    return x.is_null()? 1: x.as_longint() + 1;
}

class TestEngineSql : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestEngineSql);
    CPPUNIT_TEST(test_select_sql);
    CPPUNIT_TEST(test_select_sql_max_rows);
    CPPUNIT_TEST(test_insert_sql);
    CPPUNIT_TEST(test_update_sql);
    CPPUNIT_TEST_SUITE_END();

    LongInt record_id_;
public:
    TestEngineSql() : record_id_(0) {}

    void setUp()
    {
        SqlConnection conn(Engine::sql_source_from_env());
        setup_log(conn);
        conn.begin_trans();
        record_id_ = get_next_test_id(&conn);
        //CPPUNIT_ASSERT(record_id_ > 0);
        Values params;
        params.push_back(Value(record_id_));
        params.push_back(Value(_T("item")));
        params.push_back(Value(now()));
        params.push_back(Value(Decimal(_T("1.2"))));
        conn.prepare(_T("INSERT INTO T_ORM_TEST(ID, A, B, C) VALUES(?, ?, ?, ?)"));
        conn.exec(params);
        conn.commit();
    }

    void tearDown()
    {
        SqlConnection conn(Engine::sql_source_from_env());
        setup_log(conn);
        conn.begin_trans();
        conn.exec_direct(_T("DELETE FROM T_ORM_XML"));
        conn.exec_direct(_T("DELETE FROM T_ORM_TEST"));
        conn.commit();
    }

    void test_select_sql()
    {
        Engine engine(Engine::READ_ONLY);
        setup_log(engine);
        CPPUNIT_ASSERT_EQUAL(false, engine.activity());
        RowsPtr ptr = engine.select(Expression(_T("*")),
                Expression(_T("T_ORM_TEST")),
                Expression(_T("ID")) == record_id_);
        CPPUNIT_ASSERT_EQUAL(true, engine.activity());
        CPPUNIT_ASSERT_EQUAL(1, (int)ptr->size());
        CPPUNIT_ASSERT_EQUAL(string("item"),
                NARROW(find_in_row(*ptr->begin(), _T("A"))->second.as_string()));
        CPPUNIT_ASSERT(Decimal(_T("1.2")) ==
                       find_in_row(*ptr->begin(), _T("C"))->second.as_decimal());
    }

    void test_select_sql_max_rows()
    {
        Engine engine(Engine::READ_ONLY);
        setup_log(engine);
        CPPUNIT_ASSERT_EQUAL(false, engine.activity());
        RowsPtr ptr = engine.select(Expression(_T("*")),
                Expression(_T("T_ORM_TEST")),
                Expression(_T("ID")) == record_id_,
                Expression(), Expression(), Expression(), 0);
        CPPUNIT_ASSERT_EQUAL(true, engine.activity());
        CPPUNIT_ASSERT_EQUAL(0, (int)ptr->size());
    }

    void test_insert_sql()
    {
        Engine engine(Engine::READ_WRITE);
        setup_log(engine);
        CPPUNIT_ASSERT_EQUAL(false, engine.activity());
        Rows rows;
        LongInt id = get_next_test_id(engine.get_connection());
        Row row;
        row.push_back(make_pair(String(_T("ID")), Value(id)));
        row.push_back(make_pair(String(_T("A")), Value(_T("inserted"))));
        row.push_back(make_pair(String(_T("B")), Value(now())));
        row.push_back(make_pair(String(_T("C")), Value(Decimal(_T("1.1")))));
        rows.push_back(row);
        engine.insert(_T("T_ORM_TEST"), rows);
        CPPUNIT_ASSERT_EQUAL(true, engine.activity());
        RowsPtr ptr = engine.select(Expression(_T("*")),
                Expression(_T("T_ORM_TEST")),
                Expression(_T("ID")) == id);
        CPPUNIT_ASSERT_EQUAL(1, (int)ptr->size());
        CPPUNIT_ASSERT_EQUAL(string("inserted"),
                NARROW(find_in_row(*ptr->begin(), _T("A"))->second.as_string()));
        CPPUNIT_ASSERT(Decimal(_T("1.1")) ==
                find_in_row(*ptr->begin(), _T("C"))->second.as_decimal());
        engine.commit();
    }

    void test_update_sql()
    {
        Engine engine(Engine::READ_WRITE);
        setup_log(engine);
        CPPUNIT_ASSERT_EQUAL(false, engine.activity());
        Rows rows;
        LongInt id = get_next_test_id(engine.get_connection());
        Row row;
        row.push_back(make_pair(String(_T("A")), Value(_T("updated"))));
        row.push_back(make_pair(String(_T("C")), Value(Decimal(_T("1.3")))));
        row.push_back(make_pair(String(_T("ID")), Value(record_id_)));
        rows.push_back(row);
        Strings key;
        key.push_back(_T("ID"));
        StringSet exclude;
        exclude.insert(_T("B"));
        engine.update(_T("T_ORM_TEST"), rows, key, exclude);
        CPPUNIT_ASSERT_EQUAL(true, engine.activity());
        RowsPtr ptr = engine.select(Expression(_T("*")), Expression(_T("T_ORM_TEST")),
                Expression(_T("ID")) == record_id_);
        CPPUNIT_ASSERT_EQUAL(1, (int)ptr->size());
        CPPUNIT_ASSERT_EQUAL(string("updated"),
                NARROW(find_in_row(*ptr->begin(), _T("A"))->second.as_string()));
        CPPUNIT_ASSERT(Decimal(_T("1.3")) ==
                find_in_row(*ptr->begin(), _T("C"))->second.as_decimal());
        engine.commit();
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestEngineSql);

// vim:ts=4:sts=4:sw=4:et:
