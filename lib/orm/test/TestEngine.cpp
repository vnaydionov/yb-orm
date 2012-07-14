#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
#include <util/str_utils.hpp>
#include <orm/Engine.h>

using namespace std;
using namespace Yb;
using Yb::StrUtils::xgetenv;

static LogAppender appender(cerr);
#define SETUP_LOG(e) do{ e.set_echo(true); \
    ILogger::Ptr __log(new Logger(&appender)); \
    e.set_logger(__log); }while(0)

static inline const String cfg(const String &entry, const String &def_value = _T(""))
{
    String value = xgetenv(_T("YBORM_") + entry);
    if (str_empty(value))
        value = def_value;
    return value;
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

    String db_type_;
    LongInt record_id_;

    LongInt get_next_test_id(Engine &engine, const String &seq_name)
    {
        if (!engine.get_dialect()->has_sequences()) {
            RowsPtr ptr = engine.select(_T("MAX(ID) MAX_ID"), _T("T_ORM_TEST"), Filter());
            CPPUNIT_ASSERT(1 == ptr->size() && 1 == ptr->begin()->size());
            Value x = find_in_row(*ptr->begin(), _T("MAX_ID"))->second;
            return x.is_null()? 1: x.as_longint() + 1;
        }
        else {
            return engine.get_next_value(seq_name);
        }
    }

public:
    TestEngine()
        : db_type_(xgetenv(_T("YBORM_DBTYPE")))
        , record_id_(0)
    {}

    void init_sql()
    {
        Engine engine(Engine::READ_ONLY);
        SETUP_LOG(engine);
        record_id_ = get_next_test_id(engine, _T("S_ORM_TEST_ID"));
        //CPPUNIT_ASSERT(record_id_ > 0);
        std::ostringstream sql;
        sql << "INSERT INTO T_ORM_TEST(ID, A, B, C) VALUES(?, ?, ?, ?)";
        Values params;
        params.push_back(Value(record_id_));
        params.push_back(Value(_T("item")));
        params.push_back(Value(now()));
        params.push_back(Value(Decimal(_T("1.2"))));
        SqlConnection conn(cfg(_T("DRIVER"), _T("DEFAULT")), cfg(_T("DBTYPE")),
                cfg(_T("DB")), cfg(_T("USER")), cfg(_T("PASSWD")));
        conn.begin_trans();
        conn.exec_direct(_T("DELETE FROM T_ORM_XML"));
        conn.exec_direct(_T("DELETE FROM T_ORM_TEST"));
        conn.prepare(WIDEN(sql.str()));
        conn.exec(params);
        conn.commit();
    }

    void finish_sql()
    {
        SqlConnection conn(cfg(_T("DRIVER"), _T("DEFAULT")), cfg(_T("DBTYPE")),
                cfg(_T("DB")), cfg(_T("USER")), cfg(_T("PASSWD")));
        conn.begin_trans();
        conn.exec_direct(_T("DELETE FROM T_ORM_TEST"));
        conn.commit();
    }

    void test_strlist_one()
    {
        StringSet fields;
        fields.insert(_T("A"));
        StrList list(fields);
        CPPUNIT_ASSERT_EQUAL(string("A"), NARROW(list.get_str()));
    }

    void test_strlist_two()
    {
        StringSet fields;
        fields.insert(_T("B"));
        fields.insert(_T("A"));
        StrList list(fields);
        CPPUNIT_ASSERT_EQUAL(string("A, B"), NARROW(list.get_str()));
    }

    void test_select_simple()
    {
        Engine engine(Engine::READ_ONLY);
        SETUP_LOG(engine);
        String sql;
        Values params;
        engine.do_gen_sql_select(sql, params, _T("A, B"), _T("T"),
                Filter(), StrList(), Filter(), StrList(), false);
        CPPUNIT_ASSERT_EQUAL(string("SELECT A, B FROM T"), NARROW(sql));
    }

    void test_select_for_update()
    {
        Engine engine(Engine::READ_ONLY);
        SETUP_LOG(engine);
        String sql;
        Values params;
        engine.do_gen_sql_select(sql, params, _T("1"), _T("T"),
                Filter(), StrList(), Filter(), StrList(), true);
        CPPUNIT_ASSERT_EQUAL(string("SELECT 1 FROM T FOR UPDATE"), NARROW(sql));
    }

    void test_select_where()
    {
        Engine engine(Engine::READ_ONLY);
        SETUP_LOG(engine);
        String sql;
        Values params;
        engine.do_gen_sql_select(sql, params, _T("*"), _T("T"),
                filter_eq(_T("ID"), Value(1)),
                StrList(), Filter(), StrList(), false);
        CPPUNIT_ASSERT_EQUAL(string("SELECT * FROM T WHERE ID = ?"), NARROW(sql));
        CPPUNIT_ASSERT_EQUAL(1, (int)params.size());
        CPPUNIT_ASSERT_EQUAL(1, (int)params[0].as_longint());
    }

    void test_select_groupby()
    {
        Engine engine(Engine::READ_ONLY);
        SETUP_LOG(engine);
        String sql;
        Values params;
        engine.do_gen_sql_select(sql, params, _T("A, B"), _T("T"),
                Filter(), _T("A, B"), Filter(), StrList(), false);
        CPPUNIT_ASSERT_EQUAL(string("SELECT A, B FROM T GROUP BY A, B"), NARROW(sql));
    }

    void test_select_having()
    {
        Engine engine(Engine::READ_ONLY);
        SETUP_LOG(engine);
        String sql;
        Values params;
        engine.do_gen_sql_select(sql, params, _T("A, COUNT(*)"), _T("T"),
                Filter(), _T("A"), Filter(_T("COUNT(*) > 2")), StrList(), false);
        CPPUNIT_ASSERT_EQUAL(string("SELECT A, COUNT(*) FROM T GROUP BY A HAVING COUNT(*) > 2"), NARROW(sql));
    }

    void test_select_having_wo_groupby()
    {
        Engine engine(Engine::READ_ONLY);
        SETUP_LOG(engine);
        String sql;
        Values params;
        engine.do_gen_sql_select(sql, params, _T("A, B"), _T("T"), Filter(), _T(""),
                filter_eq(_T("ID"), Value(1)), StrList(), false);
    }

    void test_select_orderby()
    {
        Engine engine(Engine::READ_ONLY);
        SETUP_LOG(engine);
        String sql;
        Values params;
        engine.do_gen_sql_select(sql, params, _T("A, B"), _T("T"), Filter(), _T(""),
                Filter(), _T("A"), false);
        CPPUNIT_ASSERT_EQUAL(string("SELECT A, B FROM T ORDER BY A"), NARROW(sql));
    }

    void test_insert_simple()
    {
        Engine engine(Engine::READ_ONLY);
        SETUP_LOG(engine);
        String sql;
        Row row;
        row.push_back(make_pair(String(_T("ID")), Value(1)));
        row.push_back(make_pair(String(_T("A")), Value(_T("a"))));
        Values params;
        ParamNums param_nums;
        engine.do_gen_sql_insert(sql, params, param_nums, _T("T"), row, StringSet());
        CPPUNIT_ASSERT_EQUAL(string("INSERT INTO T (ID, A) VALUES (?, ?)"), NARROW(sql));
        CPPUNIT_ASSERT_EQUAL(2, (int)params.size());
        CPPUNIT_ASSERT_EQUAL(string("a"), NARROW(params[1].as_string()));
        CPPUNIT_ASSERT_EQUAL(1, (int)params[0].as_longint());
    }

    void test_insert_exclude()
    {
        Engine engine(Engine::READ_ONLY);
        SETUP_LOG(engine);
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
        SETUP_LOG(engine);
        String sql;
        Values params;
        ParamNums param_nums;
        engine.do_gen_sql_insert(sql, params, param_nums, _T("A"), Row(), StringSet());
    }

    void test_update_where()
    {
        Engine engine(Engine::READ_ONLY);
        SETUP_LOG(engine);
        String sql;
        Row row;
        row.push_back(make_pair(String(_T("A")), Value(_T("a"))));
        Values params;
        ParamNums param_nums;
        engine.do_gen_sql_update(sql, params, param_nums,
                _T("T"), row, Strings(), StringSet(), filter_eq(_T("B"), Value(_T("b"))));
        CPPUNIT_ASSERT_EQUAL(string("UPDATE T SET A = ? WHERE (B = ?)"), NARROW(sql));
        CPPUNIT_ASSERT(2 == params.size()
                && Value(_T("a")) == params[0] && Value(_T("b")) == params[1]);
        CPPUNIT_ASSERT(1 == param_nums.size() && 0 == param_nums[_T("A")]);
    }

    void test_update_combo()
    {
        Engine engine(Engine::READ_ONLY);
        SETUP_LOG(engine);
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
                _T("T"), row, key, exclude, filter_eq(_T("Q"), Value(_T("q"))));
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
        SETUP_LOG(engine);
        String sql;
        Values params;
        ParamNums param_nums;
        engine.do_gen_sql_update(sql, params, param_nums,
                _T("A"), Row(), Strings(), StringSet(), Filter());
    }

    void test_update_wo_clause()
    {
        Engine engine(Engine::READ_ONLY);
        SETUP_LOG(engine);
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
        SETUP_LOG(engine);
        String sql;
        Values params;
        engine.do_gen_sql_delete(sql, params, _T("T"), filter_eq(_T("ID"), Value(1)));
        CPPUNIT_ASSERT_EQUAL(string("DELETE FROM T WHERE ID = ?"), NARROW(sql));
        CPPUNIT_ASSERT(1 == params.size() && Value(1) == params[0]);
    }

    void test_delete_wo_clause()
    {
        Engine engine(Engine::READ_ONLY);
        SETUP_LOG(engine);
        String sql;
        Values params;
        engine.do_gen_sql_delete(sql, params, _T("T"), Filter());
    }

    void test_select_sql()
    {
        Engine engine(Engine::READ_ONLY);
        SETUP_LOG(engine);
        CPPUNIT_ASSERT_EQUAL(false, engine.activity());
        init_sql();
        RowsPtr ptr = engine.select(_T("*"), _T("T_ORM_TEST"), filter_eq(_T("ID"), Value(record_id_)));
        CPPUNIT_ASSERT_EQUAL(true, engine.activity());
        CPPUNIT_ASSERT_EQUAL(1, (int)ptr->size());
        CPPUNIT_ASSERT_EQUAL(string("item"),
                             NARROW(find_in_row(*ptr->begin(), _T("A"))->second.as_string()));
        CPPUNIT_ASSERT(Decimal(_T("1.2")) ==
                       find_in_row(*ptr->begin(), _T("C"))->second.as_decimal());
        finish_sql();
    }

    void test_select_sql_max_rows()
    {
        Engine engine(Engine::READ_ONLY);
        SETUP_LOG(engine);
        CPPUNIT_ASSERT_EQUAL(false, engine.activity());
        init_sql();
        RowsPtr ptr = engine.select(_T("*"), _T("T_ORM_TEST"), filter_eq(_T("ID"), Value(record_id_)),
                StrList(), Filter(), StrList(), 0);
        CPPUNIT_ASSERT_EQUAL(true, engine.activity());
        CPPUNIT_ASSERT_EQUAL(0, (int)ptr->size());
        finish_sql();
    }

    void test_insert_sql()
    {
        init_sql();
        Engine engine(Engine::MANUAL);
        SETUP_LOG(engine);
        CPPUNIT_ASSERT_EQUAL(false, engine.activity());
        Rows rows;
        LongInt id = get_next_test_id(engine, _T("S_ORM_TEST_ID"));
        Row row;
        row.push_back(make_pair(String(_T("ID")), Value(id)));
        row.push_back(make_pair(String(_T("A")), Value(_T("inserted"))));
        row.push_back(make_pair(String(_T("B")), Value(now())));
        row.push_back(make_pair(String(_T("C")), Value(Decimal(_T("1.1")))));
        rows.push_back(row);
        engine.insert(_T("T_ORM_TEST"), rows);
        CPPUNIT_ASSERT_EQUAL(true, engine.activity());
        RowsPtr ptr = engine.select(_T("*"), _T("T_ORM_TEST"), filter_eq(_T("ID"), Value(id)));
        CPPUNIT_ASSERT_EQUAL(1, (int)ptr->size());
        CPPUNIT_ASSERT_EQUAL(string("inserted"),
                             NARROW(find_in_row(*ptr->begin(), _T("A"))->second.as_string()));
        CPPUNIT_ASSERT(Decimal(_T("1.1")) ==
                       find_in_row(*ptr->begin(), _T("C"))->second.as_decimal());
        engine.commit();
        finish_sql();
    }

    void test_update_sql()
    {
        init_sql();
        Engine engine(Engine::MANUAL);
        SETUP_LOG(engine);
        CPPUNIT_ASSERT_EQUAL(false, engine.activity());
        Rows rows;
        LongInt id = get_next_test_id(engine, _T("S_ORM_TEST_ID"));
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
        RowsPtr ptr = engine.select(_T("*"), _T("T_ORM_TEST"), filter_eq(_T("ID"), Value(record_id_)));
        CPPUNIT_ASSERT_EQUAL(1, (int)ptr->size());
        CPPUNIT_ASSERT_EQUAL(string("updated"),
                             NARROW(find_in_row(*ptr->begin(), _T("A"))->second.as_string()));
        CPPUNIT_ASSERT(Decimal(_T("1.3")) ==
                       find_in_row(*ptr->begin(), _T("C"))->second.as_decimal());
        engine.commit();
        finish_sql();
    }
    
    void test_insert_ro_mode()
    {
        Engine engine(Engine::READ_ONLY);
        SETUP_LOG(engine);
        engine.insert(_T(""), Rows());
    }
    
    void test_update_ro_mode()
    {
        Engine engine(Engine::READ_ONLY);
        SETUP_LOG(engine);
        engine.update(_T(""), Rows(), Strings(), StringSet());
    }
    
    void test_selforupdate_ro_mode()
    {
        Engine engine(Engine::READ_ONLY);
        SETUP_LOG(engine);
        engine.select(_T(""), _T(""), Filter(), StrList(), Filter(), StrList(), -1, true);
    }
    
    void test_commit_ro_mode()
    {
        Engine engine(Engine::READ_ONLY);
        SETUP_LOG(engine);
        engine.commit();
    }
    
    void test_delete_ro_mode()
    {
        Engine engine(Engine::READ_ONLY);
        SETUP_LOG(engine);
        engine.delete_from(_T(""), Filter());
    }
    
    void test_execpoc_ro_mode()
    {
        Engine engine(Engine::READ_ONLY);
        SETUP_LOG(engine);
        engine.exec_proc(_T(""));
    }

    void test_force_select_for_update()
    {
        Engine engine(Engine::FORCE_SELECT_UPDATE);
        if (engine.get_dialect()->get_name() != _T("SQLITE")) {
            SETUP_LOG(engine);
            CPPUNIT_ASSERT_EQUAL(false, engine.activity());
            RowsPtr ptr = engine.select(_T("*"), _T("T_ORM_TEST"), Filter());
            CPPUNIT_ASSERT_EQUAL(true, engine.activity());
        }
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
        SqlConnection conn(cfg(_T("DRIVER"), _T("DEFAULT")), cfg(_T("DBTYPE")),
                cfg(_T("DB")), cfg(_T("USER")), cfg(_T("PASSWD")));
        ///
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestSqlDriver);

// vim:ts=4:sts=4:sw=4:et:
