#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
#include "orm/Session.h"

using namespace std;
using namespace Yb;

class MockDataSource : public DataSource
{
    const TableMetaDataRegistry &reg_;
    size_t select_cnt_, update_cnt_, insert_cnt_, delete_cnt_;
    LongInt seq_;
public:
    MockDataSource(const TableMetaDataRegistry &reg)
        : reg_(reg)
        , select_cnt_(0)
        , update_cnt_(0)
        , insert_cnt_(0)
        , delete_cnt_(0)
        , seq_(100)
    {}
    RowDataPtr select_row(const RowData &key)
    {
        RowDataVectorPtr vp = select_rows(
                key.get_table().get_name(), FilterByKey(key));
        if (vp->size() != 1)
            throw ObjectNotFoundByKey("Can't fetch exactly one object.");
        RowDataPtr p(new RowData((*vp)[0]));
        return p;
    }
    RowDataVectorPtr select_rows(
            const string &table_name, const Filter &filter, 
            const StrList &order_by = StrList(), int max = -1,
            const string &table_alias = "")
    {
        RowDataVectorPtr vp(new RowDataVector());
        if (table_name == "A") {
            if (delete_cnt_)
                throw ObjectNotFoundByKey("blah");
            RowData d(reg_, table_name);
            d.set("X", Value(1));
            d.set("Y", Value(update_cnt_ == 0? "#": "abc"));
            vp->push_back(d);
        }
        else if (table_name == "B") {
            {
                RowData d(reg_, table_name);
                d.set("X", Value(1));
                d.set("U", Value(1));
                vp->push_back(d);
            }
            {
                RowData d(reg_, table_name);
                d.set("X", Value(2));
                d.set("U", Value(1));
                vp->push_back(d);
            }
        }
        ++select_cnt_;
        return vp;
    }
    void insert_rows(const string &table_name,
            const RowDataVector &rows)
    {
        insert_cnt_ += rows.size();
        const TableMetaData &t = reg_.get_table(table_name);
        const string pk_name = t.find_synth_pk();
        if (!pk_name.empty())
            for (int i = 0; i < rows.size(); ++i) {
                PKIDValue pkid = rows[i].get(pk_name).as_pkid();
                if (pkid.is_temp())
                    pkid.sync(get_next_id("S_A_X"));
            }
    }
    void update_rows(const string & /* table_name */,
            const RowDataVector &rows)
    {
        update_cnt_ += rows.size();
    }
    void delete_row(const RowData &/* row */)
    {
        //
        ++delete_cnt_;
    }
    LongInt get_curr_id(const string &seq_name)
    {
        CPPUNIT_ASSERT_EQUAL(string("S_A_X"), seq_name);
        return seq_;
    }
    LongInt get_next_id(const string &seq_name)
    {
        CPPUNIT_ASSERT_EQUAL(string("S_A_X"), seq_name);
        return ++seq_;
    }
    size_t get_select_cnt() const { return select_cnt_; }
    size_t get_update_cnt() const { return update_cnt_; }
    size_t get_insert_cnt() const { return insert_cnt_; }
    size_t get_delete_cnt() const { return delete_cnt_; }
};

class TestSession : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(TestSession);
    CPPUNIT_TEST(test_id_map);
    CPPUNIT_TEST(test_load);
    CPPUNIT_TEST(test_lazy_load);
    CPPUNIT_TEST(test_dirty);
    CPPUNIT_TEST(test_new);
    CPPUNIT_TEST(test_new_and_existing);
    CPPUNIT_TEST(test_register_as_new);
    CPPUNIT_TEST_EXCEPTION(test_register_as_deleted, ObjectNotFoundByKey);
    CPPUNIT_TEST(test_load_collection);
    CPPUNIT_TEST(test_insert_order);
    CPPUNIT_TEST_SUITE_END();

    TableMetaDataRegistry r_;
    const TableMetaDataRegistry &get_r() const { return r_; }

public:
    void setUp()
    {
        TableMetaData t("A");
        t.set_column(ColumnMetaData("X", Value::LONGINT, 0,
                    ColumnMetaData::PK));
        t.set_column(ColumnMetaData("Y", Value::STRING, 0, 0));
        t.set_seq_name("S_A_X");
        TableMetaDataRegistry r;
        r.set_table(t);
        r_ = r;
    }

    void test_id_map()
    {
        RowSet row_set;
        boost::shared_ptr<RowData> d(new RowData(get_r(), "A"));
        d->set("X", Value(1));
        d->set("Y", Value("#"));
        row_set[*d] = d;
        boost::shared_ptr<RowData> e(new RowData(get_r(), "A"));
        e->set("X", Value(2));
        e->set("Y", Value("@"));
        row_set[*e] = e;
        RowData key(get_r(), "A");
        key.set("X", Value(2));
        RowSet::iterator it = row_set.find(key);
        CPPUNIT_ASSERT(it != row_set.end());
        CPPUNIT_ASSERT_EQUAL(string("@"), it->second->get("Y").as_string());
    }

    void test_load()
    {
        MockDataSource ds(get_r());
        Session session(get_r(), ds);
        RowData key(get_r(), "A");
        key.set("X", Value(1));
        RowData *d = session.find(key);
        CPPUNIT_ASSERT(d != NULL);
        CPPUNIT_ASSERT_EQUAL(1, (int)d->get("X").as_longint());
        CPPUNIT_ASSERT_EQUAL(string("#"), d->get("Y").as_string());
    }

    void test_lazy_load()
    {
        MockDataSource ds(get_r());
        Session session(get_r(), ds);
        RowData key(get_r(), "A");
        key.set("X", Value(1));
        RowData *d = session.find(key);
        CPPUNIT_ASSERT(d != NULL);
        CPPUNIT_ASSERT(d->is_ghost());
        d->get("X");
        CPPUNIT_ASSERT(d->is_ghost());
        CPPUNIT_ASSERT_EQUAL(0, (int)ds.get_select_cnt());
        d->get("Y");
        CPPUNIT_ASSERT(!d->is_ghost());
        CPPUNIT_ASSERT_EQUAL(1, (int)ds.get_select_cnt());
        session.find(key);
        CPPUNIT_ASSERT_EQUAL(1, (int)ds.get_select_cnt());
    }

    void test_dirty()
    {
        MockDataSource ds(get_r());
        Session session(get_r(), ds);
        RowData key(get_r(), "A");
        key.set("X", Value(1));
        RowData *d = session.find(key);
        CPPUNIT_ASSERT(d != NULL);
        CPPUNIT_ASSERT(!d->is_dirty());
        d->set("Y", Value("abc"));
        CPPUNIT_ASSERT(d->is_dirty());
        CPPUNIT_ASSERT_EQUAL(0, (int)ds.get_update_cnt());
        session.flush();
        CPPUNIT_ASSERT(!d->is_dirty());
        CPPUNIT_ASSERT_EQUAL(string("abc"), d->get("Y").as_string());
        CPPUNIT_ASSERT_EQUAL(1, (int)ds.get_update_cnt());
    }

    void test_new()
    {
        MockDataSource ds(get_r());
        Session session(get_r(), ds);
        RowData *d = session.create("A");
        CPPUNIT_ASSERT(d != NULL);
        CPPUNIT_ASSERT(d->is_new());
        Value id = d->get("X");
        CPPUNIT_ASSERT_THROW(id.as_longint(), Yb::ValueBadCast);
        CPPUNIT_ASSERT(Value::PKID == id.get_type());
        CPPUNIT_ASSERT_EQUAL(true, id.as_pkid().is_temp());
        CPPUNIT_ASSERT(d->get("Y").is_null());
        d->set("Y", Value("xyz"));
        CPPUNIT_ASSERT(d->is_new());
        CPPUNIT_ASSERT_EQUAL(0, (int)ds.get_insert_cnt());
        session.flush();
        CPPUNIT_ASSERT(!d->is_new());
        CPPUNIT_ASSERT_EQUAL(1, (int)ds.get_insert_cnt());
        CPPUNIT_ASSERT_EQUAL(false, id.as_pkid().is_temp());
        CPPUNIT_ASSERT(d->get("X").as_longint() > 0);
    }

    void test_new_and_existing()
    {
        MockDataSource ds(get_r());
        Session session(get_r(), ds);
        RowData *d = session.create("A");
        d->set("Y", Value("abc"));
        //cout << "rows_.size()=" << session.rows_.size() << " item="
        //     << (int *)session.rows_.begin()->second.get() << "\n";
        session.flush();
        LongInt id = d->get("X").as_longint();
        //cout << "id=" << id << ", rows_.size()=" << session.rows_.size()
        //     << " item=" << (int *)session.rows_.begin()->second.get() << "\n";
        RowData key(get_r(), "A");
        key.set("X", Value(PKIDValue(get_r().get_table("A"), id)));
        RowData *e = session.find(key);
        //cout << "id=" << id << ", rows_.size()=" << session.rows_.size()
        //     << " item=" << (int *)session.rows_.begin()->second.get() << "\n";
        CPPUNIT_ASSERT_EQUAL(d, e);
        RowData key2(get_r(), "A");
        key2.set("X", id);
        RowData *f = session.find(key2);
        CPPUNIT_ASSERT_EQUAL(d, f);
        CPPUNIT_ASSERT(d->is_ghost());
    }

    void test_register_as_new()
    {
        MockDataSource ds(get_r());
        Session session(get_r(), ds);
        LongInt next_id = ds.get_curr_id("S_A_X") + 1;
        RowData d(get_r(), "A");
        d.set("X", Value(next_id));
        d.set("Y", Value("abc"));
        RowData *e = session.register_as_new(d);
        CPPUNIT_ASSERT(e->is_new());
        session.flush();
        CPPUNIT_ASSERT(!e->is_new());
    }

    void test_register_as_deleted()
    {
        MockDataSource ds(get_r());
        Session session(get_r(), ds);
        RowData key(get_r(), "A");
        key.set("X", Value(1));
        RowData *d = session.find(key);
        d->set_deleted();
        CPPUNIT_ASSERT_EQUAL(0, (int)ds.get_delete_cnt());
        session.flush();
        CPPUNIT_ASSERT_EQUAL(1, (int)ds.get_delete_cnt());
        d = session.find(key);
        d->load();
    }

    void test_load_collection()
    {
        TableMetaData t("B");
        t.set_column(ColumnMetaData("X", Value::LONGINT, 0,
                    ColumnMetaData::PK));
        t.set_column(ColumnMetaData("U", Value::LONGINT, 0, 0));
        r_.set_table(t);
        MockDataSource ds(get_r());
        Session session(get_r(), ds);
        RowData key(get_r(), "B");
        key.set("X", 2);
        RowData *d = session.find(key);
        CPPUNIT_ASSERT(d->is_ghost());
        LoadedRows rows = session.load_collection("B",
                filter_eq("U", Value(1)));
        CPPUNIT_ASSERT_EQUAL(2, (int)rows->size());
        CPPUNIT_ASSERT(!(*rows)[0]->is_ghost());
        CPPUNIT_ASSERT(!(*rows)[0]->get("U").is_null());
        CPPUNIT_ASSERT_EQUAL(1, (int)(*rows)[0]->get("U").as_longint());
        CPPUNIT_ASSERT(!(*rows)[1]->is_ghost());
        CPPUNIT_ASSERT(!(*rows)[1]->get("U").is_null());
        CPPUNIT_ASSERT_EQUAL(1, (int)(*rows)[1]->get("U").as_longint());
    }

    void test_insert_order()
    {
        TableMetaDataRegistry r;
        {
            TableMetaData t("A");
            t.set_column(ColumnMetaData("X", Value::LONGINT, 0,
                        ColumnMetaData::PK));
            t.set_seq_name("S_A_X");
            r.set_table(t);
        }
        {
            TableMetaData t("C");
            t.set_column(ColumnMetaData("X", Value::LONGINT, 0,
                        ColumnMetaData::PK));
            t.set_column(ColumnMetaData("AX", Value::LONGINT, 0, 0, "A", "X"));
            t.set_seq_name("S_A_X");
            r.set_table(t);
        }
        {
            TableMetaData t("B");
            t.set_column(ColumnMetaData("X", Value::LONGINT, 0,
                        ColumnMetaData::PK));
            t.set_column(ColumnMetaData("AX", Value::LONGINT, 0, 0, "A", "X"));
            t.set_column(ColumnMetaData("CX", Value::LONGINT, 0, 0, "C", "X"));
            t.set_seq_name("S_A_X");
            r.set_table(t);
        }
        r.check();

        MockDataSource ds(r);
        Session session(r, ds);
        session.create("C");
        session.create("A");
        session.create("A");
        session.create("B");
        CPPUNIT_ASSERT_EQUAL(4, (int)session.rows_.size());

        set<string> tables;
        session.get_unique_tables_for_insert(tables);
        CPPUNIT_ASSERT_EQUAL(3, (int)tables.size());
        {
            set<string>::const_iterator it = tables.begin();
            CPPUNIT_ASSERT_EQUAL(string("A"), *it++);
            CPPUNIT_ASSERT_EQUAL(string("B"), *it++);
            CPPUNIT_ASSERT_EQUAL(string("C"), *it++);
        }

        list<string> s_tables;
        session.sort_tables(tables, s_tables);
        CPPUNIT_ASSERT_EQUAL(3, (int)s_tables.size());
        {
            list<string>::const_iterator it = s_tables.begin();
            CPPUNIT_ASSERT_EQUAL(string("A"), *it++);
            CPPUNIT_ASSERT_EQUAL(string("C"), *it++);
            CPPUNIT_ASSERT_EQUAL(string("B"), *it++);
        }
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestSession);

// vim:ts=4:sts=4:sw=4:et:
