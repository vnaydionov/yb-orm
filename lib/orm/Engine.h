#ifndef YB__ORM__ENGINE__INCLUDED
#define YB__ORM__ENGINE__INCLUDED

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <stdexcept>
#include <boost/utility.hpp>
#include "Value.h"
#include "Filters.h"
#include "SqlDriver.h"

class TestEngine;

namespace Yb {

class Engine : private boost::noncopyable
{
    friend class ::TestEngine;
public:
    enum Mode { READ_ONLY, MANUAL, FORCE_SELECT_UPDATE }; 

    Engine(Mode work_mode = MANUAL);
    Engine(Mode work_mode, std::auto_ptr<SqlConnect> conn);
    Engine(Mode work_mode, SqlDialect *dialect); // for subclassing
    virtual ~Engine();

    SqlConnect *get_connect() { return conn_.get(); }
    SqlDialect *get_dialect() { return dialect_; }
    bool is_touched() const { return touched_; }

    // SQL operator wrappers
    RowsPtr select(const StrList &what,
            const StrList &from, const Filter &where = Filter(),
            const StrList &group_by = StrList(),
            const Filter &having = Filter(),
            const StrList &order_by = StrList(), int max_rows = -1,
            bool for_update = false);
    const std::vector<LongInt> insert(const std::string &table_name,
            const Rows &rows, const FieldSet &exclude_fields = FieldSet(),
            bool collect_new_ids = false);
    void update(const std::string &table_name,
            const Rows &rows, const FieldSet &key_fields,
            const FieldSet &exclude_fields = FieldSet(),
            const Filter &where = Filter());
    void delete_from(const std::string &table_name, const Filter &where);
    void exec_proc(const std::string &proc_code);
    void commit();
    void rollback();

    // Convenience utility methods
    RowPtr select_row(const StrList &what,
            const StrList &from, const Filter &where);
    RowPtr select_row(const StrList &from, const Filter &where);
    const Value select1(const std::string &what,
            const std::string &from, const Filter &where);
    LongInt get_curr_value(const std::string &seq_name);
    LongInt get_next_value(const std::string &seq_name);

    /* Use cases:
    Yb::EngineBase db;
    int count = db.select1("count(1)", "t_manager",
        Yb::FilterEq("hidden", Yb::Value(1))).as_longint();
    Yb::RowPtr row = db.select_row("v_ui_contract",
        Yb::FilterEq("contract_id", Yb::Value(contract_id)));
    */

private:
    virtual RowsPtr on_select(const StrList &what,
            const StrList &from, const Filter &where,
            const StrList &group_by, const Filter &having,
            const StrList &order_by, int max_rows,
            bool for_update);
    virtual const std::vector<LongInt> on_insert(
            const std::string &table_name,
            const Rows &rows, const FieldSet &exclude_fields,
            bool collect_new_ids);
    virtual void on_update(const std::string &table_name,
            const Rows &rows, const FieldSet &key_fields,
            const FieldSet &exclude_fields, const Filter &where);
    virtual void on_delete(const std::string &table_name,
            const Filter &where);
    virtual void on_exec_proc(const std::string &proc_code);
    virtual void on_commit();
    virtual void on_rollback();

    virtual void do_gen_sql_insert(std::string &sql, Values &params,
            ParamNums &param_nums, const std::string &table_name,
            const Row &row, const FieldSet &exclude_fields) const;
    virtual void do_gen_sql_update(std::string &sql, Values &params,
            ParamNums &param_nums, const std::string &table_name,
            const Row &row, const FieldSet &key_fields,
            const FieldSet &exclude_fields, const Filter &where) const;
    virtual void do_gen_sql_select(std::string &sql, Values &params,
            const StrList &what, const StrList &from, const Filter &where,
            const StrList &group_by, const Filter &having,
            const StrList &order_by, bool for_update) const;
    virtual void do_gen_sql_delete(std::string &sql, Values &params,
            const std::string &table, const Filter &where) const;

private:
    bool touched_;
    Mode mode_;
    std::auto_ptr<SqlConnect> conn_;
    SqlDialect *dialect_;
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__ENGINE__INCLUDED
