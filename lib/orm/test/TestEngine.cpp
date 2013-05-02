#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
#include <util/str_utils.hpp>
#include <orm/Engine.h>

using namespace std;
using namespace Yb;

Row::iterator find_in_row(Row &row, const String &name)
{
    Row::iterator i = row.begin(), iend = row.end();
    String uname = Yb::StrUtils::str_to_upper(name);
    for (; i != iend; ++i)
        if (i->first == uname)
            break;
    return i;
}

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
    CPPUNIT_TEST(test_update_where);
    CPPUNIT_TEST(test_update_combo);
    CPPUNIT_TEST_EXCEPTION(test_update_wo_clause, BadSQLOperation);
    CPPUNIT_TEST(test_delete);
    CPPUNIT_TEST_EXCEPTION(test_delete_wo_pk, BadSQLOperation);
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
        Table t(_T("T"));
        t.add_column(Column(_T("ID"), Value::LONGINT, 0, Column::PK));
        t.add_column(Column(_T("A"), Value::STRING, 0, 0));
        String sql;
        Values params;
        ParamNums param_nums;
        engine.gen_sql_insert(sql, params, param_nums, t, true);
        CPPUNIT_ASSERT_EQUAL(string("INSERT INTO T (ID, A) VALUES (?, ?)"), NARROW(sql));
        CPPUNIT_ASSERT_EQUAL(2, (int)params.size());
        CPPUNIT_ASSERT_EQUAL(2, (int)param_nums.size());
        CPPUNIT_ASSERT_EQUAL(0, (int)param_nums[_T("ID")]);
        CPPUNIT_ASSERT_EQUAL(1, (int)param_nums[_T("A")]);
    }

    void test_insert_exclude()
    {
        Engine engine(Engine::READ_ONLY);
        Table t(_T("T"));
        t.add_column(Column(_T("ID"), Value::LONGINT, 0,
                    Column::PK | Column::RO));
        t.add_column(Column(_T("A"), Value::STRING, 0, Column::RO));
        String sql;
        Values params;
        ParamNums param_nums;
        engine.gen_sql_insert(sql, params, param_nums, t, true);
        CPPUNIT_ASSERT_EQUAL(string("INSERT INTO T (ID) VALUES (?)"), NARROW(sql));
        CPPUNIT_ASSERT_EQUAL(1, (int)params.size());
        CPPUNIT_ASSERT_EQUAL(1, (int)param_nums.size());
        CPPUNIT_ASSERT_EQUAL(0, (int)param_nums[_T("ID")]);
    }

    void test_update_where()
    {
        Engine engine(Engine::READ_ONLY);
        Table t(_T("T"));
        t.add_column(Column(_T("A"), Value::LONGINT, 0, 0));
        t.add_column(Column(_T("B"), Value::LONGINT, 0, Column::PK));
        String sql;
        Values params;
        ParamNums param_nums;
        engine.gen_sql_update(sql, params, param_nums, t);
        CPPUNIT_ASSERT_EQUAL(string("UPDATE T SET A = ? WHERE T.B = ?"), NARROW(sql));
        CPPUNIT_ASSERT_EQUAL((size_t)2, params.size());
        CPPUNIT_ASSERT_EQUAL((size_t)2, param_nums.size());
        CPPUNIT_ASSERT_EQUAL(0, param_nums[_T("A")]);
        CPPUNIT_ASSERT_EQUAL(1, param_nums[_T("B")]);
    }

    void test_update_combo()
    {
        Engine engine(Engine::READ_ONLY);
        Table t(_T("T"));
        t.add_column(Column(_T("Q"), Value::LONGINT, 0, Column::PK));
        t.add_column(Column(_T("E"), Value::LONGINT, 0, 0));
        t.add_column(Column(_T("B"), Value::LONGINT, 0, Column::PK));
        t.add_column(Column(_T("F"), Value::LONGINT, 0, 0));
        t.add_column(Column(_T("D"), Value::LONGINT, 0, Column::PK));
        String sql;
        Values params;
        ParamNums param_nums;
        engine.gen_sql_update(sql, params, param_nums, t);
        CPPUNIT_ASSERT_EQUAL(string("UPDATE T SET E = ?, F = ? "
                    "WHERE ((T.Q = ?) AND (T.B = ?)) AND (T.D = ?)"), NARROW(sql));
        CPPUNIT_ASSERT_EQUAL((size_t)5, params.size());
        CPPUNIT_ASSERT_EQUAL((size_t)5, param_nums.size());
        CPPUNIT_ASSERT_EQUAL(0, param_nums[_T("E")]);
        CPPUNIT_ASSERT_EQUAL(1, param_nums[_T("F")]);
        CPPUNIT_ASSERT_EQUAL(2, param_nums[_T("Q")]);
        CPPUNIT_ASSERT_EQUAL(3, param_nums[_T("B")]);
        CPPUNIT_ASSERT_EQUAL(4, param_nums[_T("D")]);
    }

    void test_update_wo_clause()
    {
        Engine engine(Engine::READ_ONLY);
        Table t(_T("T"));
        t.add_column(Column(_T("Q"), Value::LONGINT, 0, 0));
        t.add_column(Column(_T("E"), Value::LONGINT, 0, 0));
        String sql;
        Values params;
        ParamNums param_nums;
        engine.gen_sql_update(sql, params, param_nums, t);
    }

    void test_delete()
    {
        Engine engine(Engine::READ_ONLY);
        Table t(_T("T"));
        t.add_column(Column(_T("ID"), Value::LONGINT, 0, Column::PK));
        String sql;
        Values params;
        engine.gen_sql_delete(sql, params, t);
        CPPUNIT_ASSERT_EQUAL(string("DELETE FROM T WHERE T.ID = ?"), NARROW(sql));
        CPPUNIT_ASSERT_EQUAL((size_t)1, params.size());
    }

    void test_delete_wo_pk()
    {
        Engine engine(Engine::READ_ONLY);
        Table t(_T("T"));
        String sql;
        Values params;
        engine.gen_sql_delete(sql, params, t);
    }

    void test_insert_ro_mode()
    {
        Engine engine(Engine::READ_ONLY);
        Table t(_T("T"));
        t.add_column(Column(_T("Q"), Value::LONGINT, 0, Column::PK));
        engine.insert(t, RowsData(), false);
    }
    
    void test_update_ro_mode()
    {
        Engine engine(Engine::READ_ONLY);
        Table t(_T("T"));
        t.add_column(Column(_T("Q"), Value::LONGINT, 0, Column::PK));
        engine.update(t, RowsData());
    }
    
    void test_delete_ro_mode()
    {
        Engine engine(Engine::READ_ONLY);
        Table t(_T("T"));
        t.add_column(Column(_T("Q"), Value::LONGINT, 0, Column::PK));
        engine.delete_from(t, Keys());
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
        conn.set_convert_params(true);
        setup_log(conn);
        conn.begin_trans_if_necessary();
        record_id_ = get_next_test_id(&conn);
        //CPPUNIT_ASSERT(record_id_ > 0);
        Values params;
        params.push_back(Value(record_id_));
        params.push_back(Value(_T("item")));
        params.push_back(Value(dt_make(2001, 1, 1)));
        params.push_back(Value(Decimal(_T("1.2"))));
        conn.prepare(_T("INSERT INTO T_ORM_TEST(ID, A, B, C) VALUES(?, ?, ?, ?)"));
        conn.exec(params);
        conn.commit();
    }

    void tearDown()
    {
        SqlConnection conn(Engine::sql_source_from_env());
        setup_log(conn);
        conn.begin_trans_if_necessary();
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
        Table t(_T("T_ORM_TEST"));
        t.add_column(Column(_T("ID"), Value::LONGINT, 0, Column::PK));
        t.add_column(Column(_T("A"), Value::STRING, 100, 0));
        t.add_column(Column(_T("B"), Value::DATETIME, 0, Column::RO));
        t.add_column(Column(_T("C"), Value::DECIMAL, 0, 0));
        setup_log(engine);
        CPPUNIT_ASSERT_EQUAL(false, engine.activity());
        RowsData rows;
        LongInt id = get_next_test_id(engine.get_conn());
        Values row;
        row.push_back(id);
        row.push_back(Value(_T("inserted")));
        row.push_back(now());
        row.push_back(Decimal(_T("1.1")));
        rows.push_back(&row);
        engine.insert(t, rows, false);
        CPPUNIT_ASSERT_EQUAL(true, engine.activity());
        RowsPtr ptr = engine.select(Expression(_T("*")),
                Expression(t.name()),
                t.column(_T("ID")) == id);
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
        Table t(_T("T_ORM_TEST"));
        t.add_column(Column(_T("ID"), Value::LONGINT, 0, Column::PK));
        t.add_column(Column(_T("A"), Value::STRING, 100, 0));
        t.add_column(Column(_T("B"), Value::DATETIME, 0, Column::RO));
        t.add_column(Column(_T("C"), Value::DECIMAL, 0, 0));
        setup_log(engine);
        RowsData rows;
        Values row;
        DateTime timestamp = now();
        row.push_back(record_id_);
        row.push_back(Value(_T("updated")));
        row.push_back(timestamp);
        row.push_back(Decimal(_T("1.3")));
        rows.push_back(&row);
        engine.update(t, rows);
        CPPUNIT_ASSERT_EQUAL(true, engine.activity());
        RowsPtr ptr = engine.select(Expression(_T("*")), Expression(_T("T_ORM_TEST")),
                Expression(_T("ID")) == record_id_);
        CPPUNIT_ASSERT_EQUAL(1, (int)ptr->size());
        CPPUNIT_ASSERT_EQUAL(string("updated"),
                NARROW(find_in_row(*ptr->begin(), _T("A"))->second.as_string()));
        CPPUNIT_ASSERT(Decimal(_T("1.3")) ==
                find_in_row(*ptr->begin(), _T("C"))->second.as_decimal());
        CPPUNIT_ASSERT(timestamp !=
                find_in_row(*ptr->begin(), _T("B"))->second.as_date_time());
        engine.commit();
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestEngineSql);

// vim:ts=4:sts=4:sw=4:et:
