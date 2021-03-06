#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
#include "orm/schema.h"
#include "orm/schema_reader.h"
#include "util/value_type.h"

using namespace std;
using namespace Yb;

class TestMetaData : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestMetaData);
    CPPUNIT_TEST(test_column);
    CPPUNIT_TEST(test_column_ex);
    CPPUNIT_TEST(test_table_cons);
    CPPUNIT_TEST(test_table_columns);
    CPPUNIT_TEST(test_table_seq);
    CPPUNIT_TEST(test_table_surrogate_pk);
    CPPUNIT_TEST(test_rel_join_cond);
    CPPUNIT_TEST(test_get_fk_for);
    CPPUNIT_TEST_EXCEPTION(test_table_bad_surrogate_pk__no_pk, TableHasNoSurrogatePK);
    CPPUNIT_TEST_EXCEPTION(test_table_bad_surrogate_pk__complex, TableHasNoSurrogatePK);
    CPPUNIT_TEST_EXCEPTION(test_table_bad_surrogate_pk__not_int, TableHasNoSurrogatePK);
    CPPUNIT_TEST_EXCEPTION(test_meta_data_bad_column_name, BadColumnName);
    CPPUNIT_TEST_EXCEPTION(test_meta_data_bad_column_name2, BadColumnName);
    CPPUNIT_TEST_EXCEPTION(test_meta_data_column_not_found, ColumnNotFoundInMetaData);
    CPPUNIT_TEST(test_md_registry);
    CPPUNIT_TEST_EXCEPTION(test_meta_data_empty_table, TableWithoutColumns);
    CPPUNIT_TEST_EXCEPTION(test_meta_data_bad_table_name, BadTableName);
    CPPUNIT_TEST_EXCEPTION(test_meta_data_table_not_found, TableNotFoundInMetaData);
    CPPUNIT_TEST(test_registry_check);
    CPPUNIT_TEST_EXCEPTION(test_registry_check_absent_fk_table, IntegrityCheckFailed);
    CPPUNIT_TEST_EXCEPTION(test_registry_check_absent_fk_field, IntegrityCheckFailed);
    CPPUNIT_TEST_EXCEPTION(test_registry_check_cyclic_references, IntegrityCheckFailed);
    CPPUNIT_TEST(test_class_name);
    CPPUNIT_TEST(test_property);
    CPPUNIT_TEST_SUITE_END();

public:
    void test_column()
    {
        Column c(_T("x"), Value::LONGINT, 0, Column::PK | Column::RO);
        CPPUNIT_ASSERT_EQUAL(string("x"), NARROW(c.name()));
        CPPUNIT_ASSERT_EQUAL((int)Value::LONGINT, c.type());
        CPPUNIT_ASSERT_EQUAL(0, (int)c.size());
        CPPUNIT_ASSERT_EQUAL((int)(Column::PK | Column::RO), c.flags());
        CPPUNIT_ASSERT(c.is_pk() && c.is_ro());
        Column d(_T("y"), Value::LONGINT, 0, Column::PK);
        CPPUNIT_ASSERT(d.is_pk() && !d.is_ro());
        Column e(_T("z"), Value::LONGINT, 0, 0);
        CPPUNIT_ASSERT(!e.is_pk() && !e.is_ro());
    }

    void test_column_ex()
    {
        Column c(_T("X_Y"), Value::LONGINT, 0, 0);
        CPPUNIT_ASSERT_EQUAL(string("x-y"), NARROW(c.xml_name()));
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(c.fk_table_name()));
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(c.fk_name()));
        Column d(_T("X_Y"), Value::LONGINT, 0, 0, Value(),
                _T(""), _T(""), _T("xYz"));
        CPPUNIT_ASSERT_EQUAL(string("xYz"), NARROW(d.xml_name()));
        CPPUNIT_ASSERT(!d.has_fk());
        Column e(_T("X_Y"), Value::LONGINT, 0, 0, Value(),
                _T("b"), _T("z"));
        CPPUNIT_ASSERT_EQUAL(string("b"), NARROW(e.fk_table_name()));
        CPPUNIT_ASSERT_EQUAL(string("z"), NARROW(e.fk_name()));
        CPPUNIT_ASSERT(e.has_fk());
    }

    void test_table_cons()
    {
        Table u(_T("a"));
        CPPUNIT_ASSERT_EQUAL(string("a"), NARROW(u.name()));
        CPPUNIT_ASSERT_EQUAL(string("a"), NARROW(u.xml_name()));
        Table v(_T("A_A"));
        CPPUNIT_ASSERT_EQUAL(string("A_A"), NARROW(v.name()));
        CPPUNIT_ASSERT_EQUAL(string("a-a"), NARROW(v.xml_name()));
        Table w(_T("a"), _T("b"));
        CPPUNIT_ASSERT_EQUAL(string("b"), NARROW(w.xml_name()));
    }

    void test_table_columns()
    {
        Table t(_T("a"));
        CPPUNIT_ASSERT(t.begin() == t.end());
        CPPUNIT_ASSERT_EQUAL(0, (int)t.size());
        t.add_column(Column(_T("x"), Value::LONGINT, 0, 0));
        CPPUNIT_ASSERT_EQUAL(1, (int)t.size());
        CPPUNIT_ASSERT_EQUAL(string("x"), NARROW(t.begin()->name()));
        t.add_column(Column(_T("y"), Value::LONGINT, 0, 0));
        CPPUNIT_ASSERT_EQUAL(2, (int)t.size());
        CPPUNIT_ASSERT_EQUAL(string("y"), NARROW(t.column(_T("Y")).name()));
    }

    void test_table_seq()
    {
        Table t(_T("A"));
        CPPUNIT_ASSERT_EQUAL(string(""), NARROW(t.seq_name()));
        t.set_seq_name(_T("s_a_id"));
        CPPUNIT_ASSERT_EQUAL(string("s_a_id"), NARROW(t.seq_name()));
    }

    void test_table_surrogate_pk()
    {
        Table t(_T("A"));
        t.add_column(Column(_T("X"), Value::LONGINT, 0, 0));
        t.add_column(Column(_T("Y"), Value::LONGINT, 0, Column::PK));
        t.add_column(Column(_T("Z"), Value::LONGINT, 0, 0));
        t.set_seq_name(_T("S_A_ID"));
        CPPUNIT_ASSERT_EQUAL(string("Y"), NARROW(t.get_surrogate_pk()));
    }

    void test_rel_join_cond()
    {
        Schema r;
        Table::Ptr ta(new Table(_T("A"), _T(""), _T("A")));
        ta->add_column(Column(_T("X"), Value::LONGINT, 0, Column::PK | Column::RO));
        r.add_table(ta);
        Table::Ptr tc(new Table(_T("C"), _T(""), _T("C")));
        tc->add_column(Column(_T("X"), Value::LONGINT, 0, Column::PK | Column::RO));
        tc->add_column(Column(_T("AX"), Value::LONGINT, 0, 0,
                    Value(), _T("A"), _T("")));
        r.add_table(tc);
        Relation::AttrMap a1, a2;
        a1[_T("property")] = _T("cs");
        a2[_T("property")] = _T("a");
        Relation::Ptr re1(new Relation(Relation::ONE2MANY,
            _T("A"), a1, _T("C"), a2));
        r.add_relation(re1);
        r.fill_fkeys();
        CPPUNIT_ASSERT_EQUAL(string("A.X = C.AX"),
                NARROW(re1->join_condition().get_sql()));
        Strings tables;
        tables.push_back(_T("C"));
        tables.push_back(_T("A"));
        CPPUNIT_ASSERT_EQUAL(string("C JOIN A ON (A.X = C.AX)"),
                NARROW(r.join_expr(tables).get_sql()));
    }

    void test_get_fk_for()
    {
        Schema r;
        Table::Ptr ta(new Table(_T("A"), _T(""), _T("A")));
        ta->add_column(Column(_T("X"), Value::LONGINT, 0, Column::PK | Column::RO));
        r.add_table(ta);
        Table::Ptr tc(new Table(_T("C"), _T(""), _T("C")));
        tc->add_column(Column(_T("X"), Value::LONGINT, 0, Column::PK | Column::RO));
        tc->add_column(Column(_T("AX"), Value::LONGINT, 0, 0,
                    Value(), _T("A"), _T("")));
        r.add_table(tc);
        Relation::AttrMap a1, a2;
        a1[_T("property")] = _T("cs");
        a2[_T("property")] = _T("a");
        Relation::Ptr re1(new Relation(Relation::ONE2MANY,
            _T("A"), a1, _T("C"), a2));
        r.add_relation(re1);
        Table::Ptr tb(new Table(_T("B"), _T(""), _T("B")));
        *tb << Column(_T("X"), Value::LONGINT, 0, Column::PK | Column::RO)
            << Column(_T("AX"), Value::LONGINT, 0, 0,
                    Value(), _T("A"), _T("X"))
            << Column(_T("A2X"), Value::LONGINT, 0, 0,
                    Value(), _T("A"), _T("X"));
        r.add_table(tb);
        Relation::AttrMap aa1, aa2;
        aa1[_T("property")] = _T("bs");
        aa2[_T("property")] = _T("a");
        aa2[_T("key")] = _T("AX");
        Relation::Ptr re2(new Relation(Relation::ONE2MANY,
            _T("A"), aa1, _T("B"), aa2));
        r.add_relation(re2);
        Relation::AttrMap aaa1, aaa2;
        aaa2[_T("property")] = _T("a2");
        aaa2[_T("key")] = _T("A2X");
        Relation::Ptr re3(new Relation(Relation::ONE2MANY,
            _T("A"), aaa1, _T("B"), aaa2));
        r.add_relation(re3);
        r.fill_fkeys();
        CPPUNIT_ASSERT_EQUAL(string("AX"), NARROW(re1->fk_fields()[0]));
        CPPUNIT_ASSERT_EQUAL(string("AX"), NARROW(re2->fk_fields()[0]));
        CPPUNIT_ASSERT_EQUAL(string("A2X"), NARROW(re3->fk_fields()[0]));
        CPPUNIT_ASSERT_EQUAL((size_t)1, re3->fk_fields().size());
    }

    void test_table_bad_surrogate_pk__no_pk()
    {
        Table t(_T("A"));
        t.add_column(Column(_T("X"), Value::LONGINT, 0, 0));
        t.set_seq_name(_T("S_A_ID"));
        t.get_surrogate_pk();
    }

    void test_table_bad_surrogate_pk__complex()
    {
        Table t(_T("A"));
        t.add_column(Column(_T("X"), Value::LONGINT, 0, Column::PK));
        t.add_column(Column(_T("Y"), Value::LONGINT, 0, Column::PK));
        t.set_seq_name(_T("S_A_ID"));
        t.get_surrogate_pk();
    }

    void test_table_bad_surrogate_pk__not_int()
    {
        Table t(_T("A"));
        t.add_column(Column(_T("X"), Value::STRING, 10, Column::PK));
        t.get_surrogate_pk();
    }

    void test_meta_data_bad_column_name()
    {
        Table t(_T("A"));
        t.add_column(Column());
    }

    void test_meta_data_bad_column_name2()
    {
        Table t(_T("A"));
        t.add_column(Column(_T("1"), 0, 0, 0));
    }

    void test_meta_data_column_not_found()
    {
        Table t(_T("A"));
        t.column(_T("Y"));
    }

    void test_md_registry()
    {
        Table::Ptr t(new Table(_T("A")));
        t->add_column(Column(_T("X"), Value::LONGINT, 0, 0));
        Schema tmd_reg;
        CPPUNIT_ASSERT_EQUAL(0, (int)tmd_reg.tbl_count());
        CPPUNIT_ASSERT(tmd_reg.tbl_begin() == tmd_reg.tbl_end());
        tmd_reg.add_table(t);
        CPPUNIT_ASSERT_EQUAL(1, (int)tmd_reg.tbl_count());
        const Table &d1 = *tmd_reg.tbl_begin()->second;
        const Table &d2 = tmd_reg.table(_T("a"));
        CPPUNIT_ASSERT_EQUAL(NARROW(d1.name()), NARROW(d2.name()));
    }

    void test_meta_data_empty_table()
    {
        Schema r;
        Table::Ptr t(new Table(_T("A")));
        r.add_table(t);
    }

    void test_meta_data_bad_table_name()
    {
        Table::Ptr t(new Table(_T("@#$")));
        t->add_column(Column(_T("Z"), 0, 0, 0));
        Schema r;
        r.add_table(t);
    }

    void test_meta_data_table_not_found()
    {
        Schema r;
        r.table(_T("Y"));
    }

    void test_registry_check()
    {
        Table::Ptr t1(new Table(_T("A")));
        *t1 << Column(_T("X"), Value::LONGINT, 0, Column::PK);
        Table::Ptr t2(new Table(_T("C")));
        *t2 << Column(_T("X"), Value::LONGINT, 0, Column::PK)
            << Column(_T("AX"), Value::LONGINT, 0, 0, Value(), _T("A"), _T("X"));
        Table::Ptr t3(new Table(_T("B")));
        *t3 << Column(_T("X"), Value::LONGINT, 0, Column::PK)
            << Column(_T("AX"), Value::LONGINT, 0, 0, Value(), _T("A"), _T("X"))
            << Column(_T("CX"), Value::LONGINT, 0, 0, Value(), _T("C"), _T("X"));
        Schema r;
        r << t1 << t2 << t3;
        CPPUNIT_ASSERT_EQUAL(3, (int)r.tbl_count());

        set<String> tables;
        r.fill_unique_tables(tables);
        CPPUNIT_ASSERT_EQUAL(3, (int)tables.size());
        {
            set<String>::const_iterator it = tables.begin();
            CPPUNIT_ASSERT_EQUAL(string("A"), NARROW(*it++));
            CPPUNIT_ASSERT_EQUAL(string("B"), NARROW(*it++));
            CPPUNIT_ASSERT_EQUAL(string("C"), NARROW(*it++));
        }

        Schema::StrMap tree_map;
        r.fill_map_tree_by_meta(tables, tree_map);
        CPPUNIT_ASSERT_EQUAL(4, (int)tree_map.size());
        {
            Schema::StrMap::const_iterator it = tree_map.begin();
            CPPUNIT_ASSERT_EQUAL(string(""), NARROW(it->first));
            CPPUNIT_ASSERT_EQUAL(string("A"), NARROW(it->second));
            ++it;
            CPPUNIT_ASSERT_EQUAL(string("A"), NARROW(it->first));
            CPPUNIT_ASSERT(_T("B") == it->second || _T("C") == it->second);
            ++it;
            CPPUNIT_ASSERT_EQUAL(string("A"), NARROW(it->first));
            CPPUNIT_ASSERT(_T("B") == it->second || _T("C") == it->second);
            ++it;
            CPPUNIT_ASSERT_EQUAL(string("C"), NARROW(it->first));
            CPPUNIT_ASSERT_EQUAL(string("B"), NARROW(it->second));
        }

        map<String, int> depths;
        r.zero_depths(tables, depths);
        CPPUNIT_ASSERT_EQUAL(3, (int)depths.size());
        CPPUNIT_ASSERT_EQUAL(0, depths[_T("A")]);
        CPPUNIT_ASSERT_EQUAL(0, depths[_T("B")]);
        CPPUNIT_ASSERT_EQUAL(0, depths[_T("C")]);
        CPPUNIT_ASSERT_EQUAL(3, (int)depths.size());

        r.traverse_children(tree_map, depths);
        CPPUNIT_ASSERT_EQUAL(3, (int)depths.size());
        CPPUNIT_ASSERT_EQUAL(1, depths[_T("A")]);
        CPPUNIT_ASSERT_EQUAL(3, depths[_T("B")]);
        CPPUNIT_ASSERT_EQUAL(2, depths[_T("C")]);

        r.set_absolute_depths(depths);
        CPPUNIT_ASSERT_EQUAL(1, r.table(_T("A")).get_depth());
        CPPUNIT_ASSERT_EQUAL(3, r.table(_T("B")).get_depth());
        CPPUNIT_ASSERT_EQUAL(2, r.table(_T("C")).get_depth());
    }

    void test_registry_check_absent_fk_table()
    {
        Table::Ptr t(new Table(_T("C")));
        *t << Column(_T("X"), Value::LONGINT, 0, Column::PK)
            << Column(_T("AX"), Value::LONGINT, 0, 0, Value(), _T("A"), _T("X"));
        Schema r;
        r << t;
        r.check_cycles();
    }

    void test_registry_check_absent_fk_field()
    {
        Table::Ptr t1(new Table(_T("A")));
        *t1 << Column(_T("X"), Value::LONGINT, 0, Column::PK);
        Table::Ptr t2(new Table(_T("C")));
        *t2 << Column(_T("X"), Value::LONGINT, 0, Column::PK)
            << Column(_T("AX"), Value::LONGINT, 0, 0, Value(), _T("A"), _T("Y"));
        Schema r;
        r << t1 << t2;
        r.check_cycles();
    }

    void test_registry_check_cyclic_references()
    {
        Table::Ptr t1(new Table(_T("A")));
        *t1 << Column(_T("X"), Value::LONGINT, 0, Column::PK)
            << Column(_T("DX"), Value::LONGINT, 0, 0, Value(), _T("D"), _T("X"));
        Table::Ptr t2(new Table(_T("B")));
        *t2 << Column(_T("X"), Value::LONGINT, 0, Column::PK)
            << Column(_T("CX"), Value::LONGINT, 0, 0, Value(), _T("C"), _T("X"));
        Table::Ptr t3(new Table(_T("C")));
        *t3 << Column(_T("X"), Value::LONGINT, 0, Column::PK)
            << Column(_T("AX"), Value::LONGINT, 0, 0, Value(), _T("A"), _T("X"));
        Table::Ptr t4(new Table(_T("D")));
        *t4 << Column(_T("X"), Value::LONGINT, 0, Column::PK);
        Schema r;
        r << t1 << t2 << t3 << t4;
        try {
            r.check_cycles();
        }
        catch (const IntegrityCheckFailed &) {
            CPPUNIT_FAIL("Unexpected exception IntegrityCheckFailed");
        }
        *t1 << Column(_T("BX"), Value::LONGINT, 0, 0, Value(), _T("B"), _T("X"));
        r.check_cycles();
    }

    void test_class_name()
    {
        const String t1 = _T("T_TEST");
        CPPUNIT_ASSERT_EQUAL(string("Test"), NARROW(guess_class_name(t1)));
        const String t2 = _T("T_te_sT");
        CPPUNIT_ASSERT_EQUAL(string("TeSt"), NARROW(guess_class_name(t2)));
        const String t3 = _T("T_tEST_tEst_TEst");
        CPPUNIT_ASSERT_EQUAL(string("TestTestTest"), NARROW(guess_class_name(t3)));
        const String t4 = _T("T_t");
        CPPUNIT_ASSERT_EQUAL(string("T"), NARROW(guess_class_name(t4)));
        const String t5 = _T("T_te_te_TE_sT");
        CPPUNIT_ASSERT_EQUAL(string("TeTeTeSt"), NARROW(guess_class_name(t5)));
        const String t6 = _T("CLIENT_HIST_TBL");
        CPPUNIT_ASSERT_EQUAL(string("ClientHist"), NARROW(guess_class_name(t6)));
        const String t7 = _T("TBL_CLIENT_HIST");
        CPPUNIT_ASSERT_EQUAL(string("ClientHist"), NARROW(guess_class_name(t7)));
        const String t8 = _T("tbl_client_hist_tbl");
        CPPUNIT_ASSERT_EQUAL(string("ClientHist"), NARROW(guess_class_name(t8)));
        const String t9 = _T("CLIENT_hist");
        CPPUNIT_ASSERT_EQUAL(string("ClientHist"), NARROW(guess_class_name(t9)));
    }

    void test_property()
    {
        const String t1 = _T("TEST");
        CPPUNIT_ASSERT_EQUAL(string("test"), NARROW(guess_property(t1)));
        const String t2 = _T("te_sT");
        CPPUNIT_ASSERT_EQUAL(string("te_st"), NARROW(guess_property(t2)));
        const String t3 = _T("tEST_tEst_TEst");
        CPPUNIT_ASSERT_EQUAL(string("test_test_test"), NARROW(guess_property(t3)));
        const String t4 = _T("t");
        CPPUNIT_ASSERT_EQUAL(string("t"), NARROW(guess_property(t4)));
        const String t5 = _T("te_te_TE_sT");
        CPPUNIT_ASSERT_EQUAL(string("te_te_te_st"), NARROW(guess_property(t5)));
        const String t6 = _T("CLIENT_HIST_ID");
        CPPUNIT_ASSERT_EQUAL(string("client_hist"), NARROW(guess_property(t6)));
        const String t7 = _T("ID_CLIENT_HIST");
        CPPUNIT_ASSERT_EQUAL(string("client_hist"), NARROW(guess_property(t7)));
        const String t8 = _T("ID_client_hist_ID");
        CPPUNIT_ASSERT_EQUAL(string("client_hist"), NARROW(guess_property(t8)));
        const String t9 = _T("CLIENT_hist");
        CPPUNIT_ASSERT_EQUAL(string("client_hist"), NARROW(guess_property(t9)));
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestMetaData);

// vim:ts=4:sts=4:sw=4:et:
