#include <algorithm>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
#include "util/string_utils.h"
#include "orm/engine.h"

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
    CPPUNIT_TEST(test_select_pager);
    CPPUNIT_TEST(test_select_pager_params);
    CPPUNIT_TEST(test_select_pager_ib);
    CPPUNIT_TEST(test_select_pager_my);
    CPPUNIT_TEST(test_select_pager_ora);
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
        SqlGeneratorOptions options;
        SqlGeneratorContext ctx;
        sql = SelectExpr(Expression(_T("A, B")))
            .from_(Expression(_T("T"))).generate_sql(options, &ctx);
        CPPUNIT_ASSERT_EQUAL(string("SELECT A, B FROM T"), NARROW(sql));
    }

    void test_select_for_update()
    {
        String sql;
        SqlGeneratorOptions options; SqlGeneratorContext ctx;
        sql = SelectExpr(Expression(_T("1")))
            .from_(Expression(_T("T"))).for_update(true).generate_sql(options, &ctx);
        CPPUNIT_ASSERT_EQUAL(string("SELECT 1 FROM T FOR UPDATE"), NARROW(sql));
    }

    void test_select_where()
    {
        String sql;
        SqlGeneratorOptions options(NO_QUOTES, true, true);
        SqlGeneratorContext ctx;
        sql = SelectExpr(Expression(_T("*")))
            .from_(Expression(_T("T")))
            .where_(Expression(_T("ID")) == 1).generate_sql(options, &ctx);
        CPPUNIT_ASSERT_EQUAL(string("SELECT * FROM T WHERE ID = ?"), NARROW(sql));
        CPPUNIT_ASSERT_EQUAL(1, (int)ctx.params_.size());
        CPPUNIT_ASSERT_EQUAL(1, (int)ctx.params_[0].as_longint());
    }

    void test_select_groupby()
    {
        String sql;
        SqlGeneratorOptions options; SqlGeneratorContext ctx;
        sql = SelectExpr(Expression(_T("A, B")))
            .from_(Expression(_T("T")))
            .group_by_(ExpressionList(Expression(_T("A")), Expression(_T("B"))))
            .generate_sql(options, &ctx);
        CPPUNIT_ASSERT_EQUAL(string("SELECT A, B FROM T GROUP BY A, B"), NARROW(sql));
    }

    void test_select_having()
    {
        String sql;
        SqlGeneratorOptions options; SqlGeneratorContext ctx;
        sql = SelectExpr(Expression(_T("A, COUNT(*)")))
            .from_(Expression(_T("T")))
            .group_by_(Expression(_T("A")))
            .having_(Expression(_T("COUNT(*) > 2"))).generate_sql(options, &ctx);
        CPPUNIT_ASSERT_EQUAL(
                string("SELECT A, COUNT(*) FROM T GROUP BY A HAVING COUNT(*) > 2"),
                NARROW(sql));
    }

    void test_select_pager()
    {
        String sql;
        SqlGeneratorOptions options; SqlGeneratorContext ctx;
        sql = SelectExpr(Expression(_T("A, B")))
            .from_(Expression(_T("T")))
            .order_by_(ExpressionList(Expression(_T("A")), Expression(_T("B"))))
            .pager(5, 10)
            .generate_sql(options, &ctx);false,
        CPPUNIT_ASSERT_EQUAL(string("SELECT A, B FROM T ORDER BY A, B LIMIT 5 OFFSET 10"), NARROW(sql));
    }

    void test_select_pager_params()
    {
        String sql;
        SqlGeneratorOptions options(NO_QUOTES, true, true, true);
        SqlGeneratorContext ctx;
        sql = SelectExpr(Expression(_T("A, B")))
            .from_(Expression(_T("T")))
            .order_by_(ExpressionList(Expression(_T("A")), Expression(_T("B"))))
            .pager(5, 10)
            .generate_sql(options, &ctx);false,
        CPPUNIT_ASSERT_EQUAL(string("SELECT A, B FROM T ORDER BY A, B LIMIT :1 OFFSET :2"), NARROW(sql));
    }

    void test_select_pager_ib()
    {
        String sql;
        SqlGeneratorOptions options(NO_QUOTES, true, false, false, PAGER_INTERBASE);
        SqlGeneratorContext ctx;
        sql = SelectExpr(Expression(_T("A, B")))
            .from_(Expression(_T("T")))
            .order_by_(ExpressionList(Expression(_T("A")), Expression(_T("B"))))
            .pager(5, 10)
            .generate_sql(options, &ctx);
        CPPUNIT_ASSERT_EQUAL(string("SELECT FIRST 5 SKIP 10 A, B FROM T ORDER BY A, B"), NARROW(sql));
    }

    void test_select_pager_my()
    {
        String sql;
        SqlGeneratorOptions options(NO_QUOTES, true, false, false, PAGER_MYSQL);
        SqlGeneratorContext ctx;
        sql = SelectExpr(Expression(_T("A, B")))
            .from_(Expression(_T("T")))
            .order_by_(ExpressionList(Expression(_T("A")), Expression(_T("B"))))
            .pager(5, 10)
            .generate_sql(options, &ctx);
        CPPUNIT_ASSERT_EQUAL(string("SELECT A, B FROM T ORDER BY A, B LIMIT 10, 5"), NARROW(sql));
    }

    void test_select_pager_ora()
    {
        String sql;
        SqlGeneratorOptions options(NO_QUOTES, true, false, false, PAGER_ORACLE);
        SqlGeneratorContext ctx;
        sql = SelectExpr(Expression(_T("A, B")))
            .from_(Expression(_T("T")))
            .order_by_(ExpressionList(Expression(_T("A")), Expression(_T("B"))))
            .pager(5, 10)
            .generate_sql(options, &ctx);
        CPPUNIT_ASSERT_EQUAL(string("SELECT OUTER_2.* FROM ("
                    "SELECT OUTER_1.*, ROWNUM AS RN_ORA FROM ("
                    "SELECT A, B FROM T ORDER BY A, B"
                    ") OUTER_1 WHERE ROWNUM <= 15"
                    ") OUTER_2 WHERE RN_ORA > 10"), NARROW(sql));
    }

    void test_select_having_wo_groupby()
    {
        String sql;
        SqlGeneratorOptions options; SqlGeneratorContext ctx;
        sql = SelectExpr(Expression(_T("A, B")))
            .from_(Expression(_T("T")))
            .having_(Expression(_T("ID")) == 1).generate_sql(options, &ctx);
    }

    void test_select_orderby()
    {
        String sql;
        SqlGeneratorOptions options; SqlGeneratorContext ctx;
        sql = SelectExpr(Expression(_T("A, B")))
            .from_(Expression(_T("T")))
            .order_by_(Expression(_T("A"))).generate_sql(options, &ctx);
        CPPUNIT_ASSERT_EQUAL(string("SELECT A, B FROM T ORDER BY A"), NARROW(sql));
    }

    void test_insert_simple()
    {
        Engine engine(Engine::READ_ONLY);
        Table t(_T("T"));
        t.add_column(Column(_T("ID"), Value::LONGINT, 0, Column::PK));
        t.add_column(Column(_T("A"), Value::STRING, 0, 0));
        String sql;
        TypeCodes types;
        ParamNums param_nums;
        engine.gen_sql_insert(sql, types, param_nums, t, true);
        CPPUNIT_ASSERT_EQUAL(string("INSERT INTO T (ID, A) VALUES (?, ?)"), NARROW(sql));
        CPPUNIT_ASSERT_EQUAL(2, (int)types.size());
        CPPUNIT_ASSERT_EQUAL(2, (int)param_nums.size());
        CPPUNIT_ASSERT_EQUAL(0, (int)param_nums[_T("ID")]);
        CPPUNIT_ASSERT_EQUAL(1, (int)param_nums[_T("A")]);
        CPPUNIT_ASSERT_EQUAL((int)Value::LONGINT, types[0]);
        CPPUNIT_ASSERT_EQUAL((int)Value::STRING, types[1]);
    }

    void test_insert_exclude()
    {
        Engine engine(Engine::READ_ONLY);
        Table t(_T("T"));
        t.add_column(Column(_T("ID"), Value::LONGINT, 0,
                    Column::PK | Column::RO));
        t.add_column(Column(_T("A"), Value::STRING, 0, Column::RO));
        String sql;
        TypeCodes types;
        ParamNums param_nums;
        engine.gen_sql_insert(sql, types, param_nums, t, true);
        CPPUNIT_ASSERT_EQUAL(string("INSERT INTO T (ID) VALUES (?)"), NARROW(sql));
        CPPUNIT_ASSERT_EQUAL(1, (int)types.size());
        CPPUNIT_ASSERT_EQUAL(1, (int)param_nums.size());
        CPPUNIT_ASSERT_EQUAL(0, (int)param_nums[_T("ID")]);
        CPPUNIT_ASSERT_EQUAL((int)Value::LONGINT, types[0]);
    }

    void test_update_where()
    {
        Engine engine(Engine::READ_ONLY);
        Table t(_T("T"));
        t.add_column(Column(_T("A"), Value::INTEGER, 0, 0));
        t.add_column(Column(_T("B"), Value::LONGINT, 0, Column::PK));
        String sql;
        TypeCodes types;
        ParamNums param_nums;
        SqlGeneratorOptions options(NO_QUOTES, true, true);
        engine.gen_sql_update(sql, types, param_nums, t, options);
        CPPUNIT_ASSERT_EQUAL(string("UPDATE T SET A = ? WHERE T.B = ?"), NARROW(sql));
        CPPUNIT_ASSERT_EQUAL((size_t)2, types.size());
        CPPUNIT_ASSERT_EQUAL((size_t)2, param_nums.size());
        CPPUNIT_ASSERT_EQUAL(0, param_nums[_T("A")]);
        CPPUNIT_ASSERT_EQUAL(1, param_nums[_T("B")]);
        CPPUNIT_ASSERT_EQUAL((int)Value::INTEGER, types[0]);
        CPPUNIT_ASSERT_EQUAL((int)Value::LONGINT, types[1]);
    }

    void test_update_combo()
    {
        Engine engine(Engine::READ_ONLY);
        Table t(_T("T"));
        t.add_column(Column(_T("Q"), Value::LONGINT, 0, Column::PK));
        t.add_column(Column(_T("E"), Value::STRING, 0, 0));
        t.add_column(Column(_T("B"), Value::DATETIME, 0, Column::PK));
        t.add_column(Column(_T("F"), Value::FLOAT, 0, 0));
        t.add_column(Column(_T("D"), Value::DECIMAL, 0, Column::PK));
        String sql;
        TypeCodes types;
        ParamNums param_nums;
        SqlGeneratorOptions options(NO_QUOTES, true, true);
        engine.gen_sql_update(sql, types, param_nums, t, options);
        CPPUNIT_ASSERT_EQUAL(string("UPDATE T SET E = ?, F = ? "
                    "WHERE ((T.Q = ?) AND (T.B = ?)) AND (T.D = ?)"), NARROW(sql));
        CPPUNIT_ASSERT_EQUAL((size_t)5, types.size());
        CPPUNIT_ASSERT_EQUAL((size_t)5, param_nums.size());
        CPPUNIT_ASSERT_EQUAL(0, param_nums[_T("E")]);
        CPPUNIT_ASSERT_EQUAL(1, param_nums[_T("F")]);
        CPPUNIT_ASSERT_EQUAL(2, param_nums[_T("Q")]);
        CPPUNIT_ASSERT_EQUAL(3, param_nums[_T("B")]);
        CPPUNIT_ASSERT_EQUAL(4, param_nums[_T("D")]);
        CPPUNIT_ASSERT_EQUAL((int)Value::STRING, types[0]);
        CPPUNIT_ASSERT_EQUAL((int)Value::FLOAT, types[1]);
        CPPUNIT_ASSERT_EQUAL((int)Value::LONGINT, types[2]);
        CPPUNIT_ASSERT_EQUAL((int)Value::DATETIME, types[3]);
        CPPUNIT_ASSERT_EQUAL((int)Value::DECIMAL, types[4]);
    }

    void test_update_wo_clause()
    {
        Engine engine(Engine::READ_ONLY);
        Table t(_T("T"));
        t.add_column(Column(_T("Q"), Value::LONGINT, 0, 0));
        t.add_column(Column(_T("E"), Value::LONGINT, 0, 0));
        String sql;
        TypeCodes types;
        ParamNums param_nums;
        SqlGeneratorOptions options;
        engine.gen_sql_update(sql, types, param_nums, t, options);
    }

    void test_delete()
    {
        Engine engine(Engine::READ_ONLY);
        Table t(_T("T"));
        t.add_column(Column(_T("ID"), Value::LONGINT, 0, Column::PK));
        String sql;
        TypeCodes types;
        SqlGeneratorOptions options(NO_QUOTES, true, true);
        engine.gen_sql_delete(sql, types, t, options);
        CPPUNIT_ASSERT_EQUAL(string("DELETE FROM T WHERE T.ID = ?"), NARROW(sql));
        CPPUNIT_ASSERT_EQUAL((size_t)1, types.size());
        CPPUNIT_ASSERT_EQUAL((int)Value::LONGINT, types[0]);
    }

    void test_delete_wo_pk()
    {
        Engine engine(Engine::READ_ONLY);
        Table t(_T("T"));
        String sql;
        TypeCodes types;
        SqlGeneratorOptions options;
        engine.gen_sql_delete(sql, types, t, options);
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
        conn.grant_insert_id(_T("T_ORM_TEST"), true, true);
        Values params;
        params.push_back(Value(record_id_));
        params.push_back(Value(_T("item")));
        params.push_back(Value(dt_make(2001, 1, 1)));
        params.push_back(Value(Decimal(_T("1.2"))));
        conn.prepare(_T("INSERT INTO T_ORM_TEST(ID, A, B, C) VALUES(?, ?, ?, ?)"));
        conn.exec(params);
        conn.grant_insert_id(_T("T_ORM_TEST"), false, true);
        conn.commit();
    }

    void tearDown()
    {
        SqlConnection conn(Engine::sql_source_from_env());
        setup_log(conn);
        conn.begin_trans_if_necessary();
        conn.exec_direct(_T("DELETE FROM T_ORM_XML"));
        conn.exec_direct(_T("DELETE FROM T_ORM_TEST"));
        conn.grant_insert_id(_T("T_ORM_TEST"), false, true);
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
        SqlConnection conn(Engine::sql_source_from_env());
        Table t(_T("T_ORM_TEST"));
        setup_log(conn);
        conn.grant_insert_id(_T("T_ORM_TEST"), true, true);
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
        conn.grant_insert_id(_T("T_ORM_TEST"), false, true);
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

class TestSqlIntrospection: public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestSqlIntrospection);
    CPPUNIT_TEST(test_show_all);
    CPPUNIT_TEST(test_find_by_name);
    CPPUNIT_TEST(test_table_columns);
    CPPUNIT_TEST_SUITE_END();

public:
    void test_show_all()
    {
        SqlConnection conn(Engine::sql_source_from_env());
        Strings tables = conn.get_tables();
        //sort(tables.begin(), tables.end());
        Strings::iterator i, j;
        i = find(tables.begin(), tables.end(), String(_T("T_ORM_TEST")));
        CPPUNIT_ASSERT(tables.end() != i);
        j = find(tables.begin(), tables.end(), String(_T("T_ORM_XML")));
        CPPUNIT_ASSERT(tables.end() != j);
        if (conn.get_dialect()->get_name() == _T("SQLITE")) {
            i = find(tables.begin(), tables.end(), _T("SQLITE_SEQUENCE"));
            CPPUNIT_ASSERT(tables.end() == i);
        }
    }

    void test_find_by_name()
    {
        SqlConnection conn(Engine::sql_source_from_env());
        CPPUNIT_ASSERT(conn.table_exists(_T("T_ORM_TEST")));
        CPPUNIT_ASSERT(conn.table_exists(_T("T_ORM_XML")));
        if (conn.get_dialect()->get_name() == _T("SQLITE")) {
            CPPUNIT_ASSERT(conn.table_exists(_T("SQLITE_SEQUENCE")));
        }
    }

    void test_table_columns()
    {
        SqlConnection conn(Engine::sql_source_from_env());
        ColumnsInfo t1 = conn.get_columns(_T("T_ORM_TEST"));
        CPPUNIT_ASSERT_EQUAL(5, (int)t1.size());
        // T_ORM_TEST.ID
        CPPUNIT_ASSERT_EQUAL(string("ID"), NARROW(t1[0].name));
        CPPUNIT_ASSERT_EQUAL(NARROW(conn.get_dialect()->type2sql(
                Value::LONGINT)), NARROW(t1[0].type));
        CPPUNIT_ASSERT_EQUAL(0, t1[0].size);
        CPPUNIT_ASSERT_EQUAL(true, t1[0].notnull);
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t1[0].default_value));
        CPPUNIT_ASSERT_EQUAL(true, t1[0].pk);
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t1[0].fk_table));
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t1[0].fk_table_key));
        // T_ORM_TEST.A
        CPPUNIT_ASSERT_EQUAL(string("A"), NARROW(t1[1].name));
        CPPUNIT_ASSERT_EQUAL(NARROW(conn.get_dialect()->type2sql(
                Value::STRING)), NARROW(t1[1].type));
        CPPUNIT_ASSERT_EQUAL(200, t1[1].size);
        CPPUNIT_ASSERT_EQUAL(false, t1[1].notnull);
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t1[1].default_value));
        CPPUNIT_ASSERT_EQUAL(false, t1[1].pk);
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t1[1].fk_table));
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t1[1].fk_table_key));
        // T_ORM_TEST.B
        CPPUNIT_ASSERT_EQUAL(string("B"), NARROW(t1[2].name));
        CPPUNIT_ASSERT_EQUAL(NARROW(conn.get_dialect()->type2sql(
                Value::DATETIME)), NARROW(t1[2].type));
        CPPUNIT_ASSERT_EQUAL(0, t1[2].size);
        CPPUNIT_ASSERT_EQUAL(false, t1[2].notnull);
        CPPUNIT_ASSERT_EQUAL(NARROW(conn.get_dialect()->sysdate_func()),
                NARROW(t1[2].default_value));
        CPPUNIT_ASSERT_EQUAL(false, t1[2].pk);
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t1[2].fk_table));
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t1[2].fk_table_key));
        // T_ORM_TEST.C
        CPPUNIT_ASSERT_EQUAL(string("C"), NARROW(t1[3].name));
        CPPUNIT_ASSERT_EQUAL(NARROW(conn.get_dialect()->type2sql(
                Value::DECIMAL)), NARROW(t1[3].type));
        CPPUNIT_ASSERT_EQUAL(0, t1[3].size);
        CPPUNIT_ASSERT_EQUAL(false, t1[3].notnull);
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t1[3].default_value));
        CPPUNIT_ASSERT_EQUAL(false, t1[3].pk);
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t1[3].fk_table));
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t1[3].fk_table_key));
        // T_ORM_TEST.D
        CPPUNIT_ASSERT_EQUAL(string("D"), NARROW(t1[4].name));
        CPPUNIT_ASSERT_EQUAL(NARROW(conn.get_dialect()->type2sql(
                Value::FLOAT)), NARROW(t1[4].type));
        CPPUNIT_ASSERT_EQUAL(0, t1[4].size);
        CPPUNIT_ASSERT_EQUAL(false, t1[4].notnull);
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t1[4].default_value));
        CPPUNIT_ASSERT_EQUAL(false, t1[4].pk);
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t1[4].fk_table));
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t1[4].fk_table_key));

        ColumnsInfo t2 = conn.get_columns(_T("T_ORM_XML"));
        CPPUNIT_ASSERT_EQUAL(3, (int)t2.size());
        // T_ORM_XML.ID
        CPPUNIT_ASSERT_EQUAL(string("ID"), NARROW(t2[0].name));
        CPPUNIT_ASSERT_EQUAL(NARROW(conn.get_dialect()->type2sql(
                Value::LONGINT)), NARROW(t2[0].type));
        CPPUNIT_ASSERT_EQUAL(0, t2[0].size);
        CPPUNIT_ASSERT_EQUAL(true, t2[0].notnull);
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t2[0].default_value));
        CPPUNIT_ASSERT_EQUAL(true, t2[0].pk);
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t2[0].fk_table));
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t2[0].fk_table_key));
        // T_ORM_XML.ORM_TEST_ID
        CPPUNIT_ASSERT_EQUAL(string("ORM_TEST_ID"), NARROW(t2[1].name));
        CPPUNIT_ASSERT_EQUAL(NARROW(conn.get_dialect()->type2sql(
                Value::LONGINT)), NARROW(t2[1].type));
        CPPUNIT_ASSERT_EQUAL(0, t2[1].size);
        CPPUNIT_ASSERT_EQUAL(false, t2[1].notnull);
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t2[1].default_value));
        CPPUNIT_ASSERT_EQUAL(false, t2[1].pk);
        CPPUNIT_ASSERT_EQUAL(string("T_ORM_TEST"), NARROW(t2[1].fk_table));
        CPPUNIT_ASSERT_EQUAL(string("ID"), NARROW(t2[1].fk_table_key));
        // T_ORM_XML.B
        CPPUNIT_ASSERT_EQUAL(string("B"), NARROW(t2[2].name));
        CPPUNIT_ASSERT_EQUAL(NARROW(conn.get_dialect()->type2sql(
                Value::DECIMAL)), NARROW(t2[2].type));
        CPPUNIT_ASSERT_EQUAL(0, t2[2].size);
        CPPUNIT_ASSERT_EQUAL(false, t2[2].notnull);
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t2[2].default_value));
        CPPUNIT_ASSERT_EQUAL(false, t2[2].pk);
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t2[2].fk_table));
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t2[2].fk_table_key));
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestSqlIntrospection);

// vim:ts=4:sts=4:sw=4:et:
