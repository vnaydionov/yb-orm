#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
#include "orm/MetaData.h"
#include "orm/Value.h"

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
    CPPUNIT_TEST(test_table_synth_pk);
    CPPUNIT_TEST_EXCEPTION(test_table_bad_synth_pk__no_pk, NotSuitableForAutoCreating);
    CPPUNIT_TEST_EXCEPTION(test_table_bad_synth_pk__complex, NotSuitableForAutoCreating);
    CPPUNIT_TEST_EXCEPTION(test_table_bad_synth_pk__no_seq, NotSuitableForAutoCreating);
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
    CPPUNIT_TEST_EXCEPTION(test_registry_check_cyclic_references_whole, IntegrityCheckFailed);
    CPPUNIT_TEST_EXCEPTION(test_registry_check_cyclic_references_inside, IntegrityCheckFailed);
    
    CPPUNIT_TEST_SUITE_END();

public:
    void test_column()
    {
        Column c("x", Value::LONGINT, 0, Column::PK | Column::RO);
        CPPUNIT_ASSERT_EQUAL(string("X"), c.get_name());
        CPPUNIT_ASSERT_EQUAL((int)Value::LONGINT, c.get_type());
        CPPUNIT_ASSERT_EQUAL(0, (int)c.get_size());
        CPPUNIT_ASSERT_EQUAL((int)(Column::PK | Column::RO), c.get_flags());
        CPPUNIT_ASSERT(c.is_pk() && c.is_ro());
        Column d("y", Value::LONGINT, 0, Column::PK);
        CPPUNIT_ASSERT(d.is_pk() && !d.is_ro());
        Column e("z", Value::LONGINT, 0, 0);
        CPPUNIT_ASSERT(!e.is_pk() && !e.is_ro());
    }

    void test_column_ex()
    {
        Column c("X_Y", Value::LONGINT, 0, 0);
        CPPUNIT_ASSERT_EQUAL(string("x-y"), c.get_xml_name());
        CPPUNIT_ASSERT_EQUAL(string(""), c.get_fk_table_name());
        CPPUNIT_ASSERT_EQUAL(string(""), c.get_fk_name());
        Column d("X_Y", Value::LONGINT, 0, 0, "", "", "xYz");
        CPPUNIT_ASSERT_EQUAL(string("xYz"), d.get_xml_name());
        CPPUNIT_ASSERT(!d.has_fk());
        Column e("X_Y", Value::LONGINT, 0, 0, "b", "z");
        CPPUNIT_ASSERT_EQUAL(string("B"), e.get_fk_table_name());
        CPPUNIT_ASSERT_EQUAL(string("Z"), e.get_fk_name());
        CPPUNIT_ASSERT(e.has_fk());
    }

    void test_table_cons()
    {
        Table t;
        CPPUNIT_ASSERT_EQUAL(string(""), t.get_name());
        CPPUNIT_ASSERT_EQUAL(string(""), t.get_xml_name());
        Table u("a");
        CPPUNIT_ASSERT_EQUAL(string("A"), u.get_name());
        CPPUNIT_ASSERT_EQUAL(string("a"), u.get_xml_name());
        Table v("a_a");
        CPPUNIT_ASSERT_EQUAL(string("A_A"), v.get_name());
        CPPUNIT_ASSERT_EQUAL(string("a-a"), v.get_xml_name());
        Table w("a", "b");
        CPPUNIT_ASSERT_EQUAL(string("b"), w.get_xml_name());
    }

    void test_table_columns()
    {
        Table t("a");
        CPPUNIT_ASSERT(t.begin() == t.end());
        CPPUNIT_ASSERT_EQUAL(0, (int)t.size());
        t.add_column(Column("x", Value::LONGINT, 0, 0));
        CPPUNIT_ASSERT_EQUAL(1, (int)t.size());
        CPPUNIT_ASSERT_EQUAL(string("X"), t.begin()->get_name());
        t.add_column(Column("Y", Value::LONGINT, 0, 0));
        CPPUNIT_ASSERT_EQUAL(2, (int)t.size());
        CPPUNIT_ASSERT_EQUAL(string("Y"), t.get_column("y").get_name());
    }

    void test_table_seq()
    {
        Table t("A");
        CPPUNIT_ASSERT_EQUAL(string(""), t.get_seq_name());
        t.set_seq_name("s_a_id");
        CPPUNIT_ASSERT_EQUAL(string("S_A_ID"), t.get_seq_name());
    }

    void test_table_synth_pk()
    {
        Table t("A");
        t.add_column(Column("X", Value::LONGINT, 0, 0));
        t.add_column(Column("Y", Value::LONGINT, 0, Column::PK));
        t.add_column(Column("Z", Value::LONGINT, 0, 0));
        t.set_seq_name("S_A_ID");
        CPPUNIT_ASSERT_EQUAL(string("Y"), t.get_synth_pk());
    }

    void test_table_bad_synth_pk__no_pk()
    {
        Table t("A");
        t.add_column(Column("X", Value::LONGINT, 0, 0));
        t.set_seq_name("S_A_ID");
        t.get_synth_pk();
    }

    void test_table_bad_synth_pk__complex()
    {
        Table t("A");
        t.add_column(Column("X", Value::LONGINT, 0, Column::PK));
        t.add_column(Column("Y", Value::LONGINT, 0, Column::PK));
        t.set_seq_name("S_A_ID");
        t.get_synth_pk();
    }

    void test_table_bad_synth_pk__no_seq()
    {
        Table t("A");
        t.add_column(Column("X", Value::LONGINT, 0, Column::PK));
        t.get_synth_pk();
    }

    void test_meta_data_bad_column_name()
    {
        Table t;
        t.add_column(Column());
    }

    void test_meta_data_bad_column_name2()
    {
        Table t;
        t.add_column(Column("1", 0, 0, 0));
    }

    void test_meta_data_column_not_found()
    {
        Table t;
        t.get_column("Y");
    }

    void test_md_registry()
    {
        Table t("A");
        t.add_column(Column("X", Value::LONGINT, 0, 0));
        Schema tmd_reg;
        CPPUNIT_ASSERT_EQUAL(0, (int)tmd_reg.size());
        CPPUNIT_ASSERT(tmd_reg.begin() == tmd_reg.end());
        tmd_reg.set_table(t);
        CPPUNIT_ASSERT_EQUAL(1, (int)tmd_reg.size());
        const Table &d1 = tmd_reg.begin()->second;
        const Table &d2 = tmd_reg.get_table("a");
        CPPUNIT_ASSERT_EQUAL(d1.get_name(), d2.get_name());
    }

    void test_meta_data_empty_table()
    {
        Schema r;
        r.set_table(Table("A"));
    }

    void test_meta_data_bad_table_name()
    {
        Table t;
        t.add_column(Column("Z", 0, 0, 0));
        Schema r;
        r.set_table(t);
    }

    void test_meta_data_table_not_found()
    {
        Schema r;
        r.get_table("Y");
    }

    void test_registry_check()
    {
        Schema r;
        {
            Table t("A");
            t.add_column(Column("X", Value::LONGINT, 0, Column::PK | Column::RO));
            r.set_table(t);
        }
        {
            Table t("C");
            t.add_column(Column("X", Value::LONGINT, 0, Column::PK | Column::RO));
            t.add_column(Column("AX", Value::LONGINT, 0, 0, "A", "X"));
            r.set_table(t);
        }
        {
            Table t("B");
            t.add_column(Column("X", Value::LONGINT, 0, Column::PK | Column::RO));
            t.add_column(Column("AX", Value::LONGINT, 0, 0, "A", "X"));
            t.add_column(Column("CX", Value::LONGINT, 0, 0, "C", "X"));
            r.set_table(t);
        }

        CPPUNIT_ASSERT_EQUAL(3, (int)r.size());

        set<string> tables;
        r.fill_unique_tables(tables);
        CPPUNIT_ASSERT_EQUAL(3, (int)tables.size());
        {
            set<string>::const_iterator it = tables.begin();
            CPPUNIT_ASSERT_EQUAL(string("A"), *it++);
            CPPUNIT_ASSERT_EQUAL(string("B"), *it++);
            CPPUNIT_ASSERT_EQUAL(string("C"), *it++);
        }

        Schema::StrMap tree_map;
        r.fill_map_tree_by_meta(tables, tree_map);
        CPPUNIT_ASSERT_EQUAL(4, (int)tree_map.size());
        {
            Schema::StrMap::const_iterator it = tree_map.begin();
            CPPUNIT_ASSERT_EQUAL(string(""), it->first);
            CPPUNIT_ASSERT_EQUAL(string("A"), it->second);
            ++it;
            CPPUNIT_ASSERT_EQUAL(string("A"), it->first);
            CPPUNIT_ASSERT("B" == it->second || "C" == it->second);
            ++it;
            CPPUNIT_ASSERT_EQUAL(string("A"), it->first);
            CPPUNIT_ASSERT("B" == it->second || "C" == it->second);
            ++it;
            CPPUNIT_ASSERT_EQUAL(string("C"), it->first);
            CPPUNIT_ASSERT_EQUAL(string("B"), it->second);
        }

        map<string, int> depths;
        r.zero_depths(tables, depths);
        CPPUNIT_ASSERT_EQUAL(3, (int)depths.size());
        CPPUNIT_ASSERT_EQUAL(0, depths["A"]);
        CPPUNIT_ASSERT_EQUAL(0, depths["B"]);
        CPPUNIT_ASSERT_EQUAL(0, depths["C"]);
        CPPUNIT_ASSERT_EQUAL(3, (int)depths.size());

        r.traverse_children(tree_map, depths);
        CPPUNIT_ASSERT_EQUAL(3, (int)depths.size());
        CPPUNIT_ASSERT_EQUAL(1, depths["A"]);
        CPPUNIT_ASSERT_EQUAL(3, depths["B"]);
        CPPUNIT_ASSERT_EQUAL(2, depths["C"]);

        r.set_absolute_depths(depths);    
        CPPUNIT_ASSERT_EQUAL(1, r.get_table("A").get_depth());
        CPPUNIT_ASSERT_EQUAL(3, r.get_table("B").get_depth());
        CPPUNIT_ASSERT_EQUAL(2, r.get_table("C").get_depth());
    }

    void test_registry_check_absent_fk_table()
    {
        Schema r;
        Table t("C");
        t.add_column(Column("X", Value::LONGINT, 0, Column::PK | Column::RO));
        t.add_column(Column("AX", Value::LONGINT, 0, 0, "A", "X"));
        r.set_table(t);
        r.check();
    }
    
    void test_registry_check_absent_fk_field()
    {
       Schema r;
        {
            Table t("A");
            t.add_column(Column("X", Value::LONGINT, 0, Column::PK | Column::RO));
            r.set_table(t);
        }
        {
            Table t("C");
            t.add_column(Column("X", Value::LONGINT, 0, Column::PK | Column::RO));
            t.add_column(Column("AX", Value::LONGINT, 0, 0, "A", "Y"));
            r.set_table(t);
        }
        r.check();
    }
    
    void test_registry_check_cyclic_references_whole()
    {
        Schema r;
        {
            Table t("A");
            t.add_column(Column("X", Value::LONGINT, 0, Column::PK | Column::RO));
            t.add_column(Column("BX", Value::LONGINT, 0, 0, "B", "X"));
            r.set_table(t);
        }
        {
            Table t("C");
            t.add_column(Column("X", Value::LONGINT, 0, Column::PK | Column::RO));
            t.add_column(Column("AX", Value::LONGINT, 0, 0, "A", "X"));
            r.set_table(t);
        }
        {
            Table t("B");
            t.add_column(Column("X", Value::LONGINT, 0, Column::PK | Column::RO));
            t.add_column(Column("CX", Value::LONGINT, 0, 0, "C", "X"));
            r.set_table(t);
        }
        r.check();
    }
    
    void test_registry_check_cyclic_references_inside()
    {
        Schema r;
        {
            Table t("D");
            t.add_column(Column("X", Value::LONGINT, 0, Column::PK | Column::RO));
            r.set_table(t);
        }
        {
            Table t("A");
            t.add_column(Column("X", Value::LONGINT, 0, Column::PK | Column::RO));
            t.add_column(Column("BX", Value::LONGINT, 0, 0, "B", "X"));
            t.add_column(Column("DX", Value::LONGINT, 0, 0, "D", "X"));
            r.set_table(t);
        }
        {
            Table t("C");
            t.add_column(Column("X", Value::LONGINT, 0, Column::PK | Column::RO));
            t.add_column(Column("AX", Value::LONGINT, 0, 0, "A", "X"));
            r.set_table(t);
        }
        {
            Table t("B");
            t.add_column(Column("X", Value::LONGINT, 0, Column::PK | Column::RO));
            t.add_column(Column("CX", Value::LONGINT, 0, 0, "C", "X"));
            r.set_table(t);
        }
        r.check();
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestMetaData);

// vim:ts=4:sts=4:sw=4:et:
