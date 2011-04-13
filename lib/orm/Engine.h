#ifndef YB__ORM__ENGINE__INCLUDED
#define YB__ORM__ENGINE__INCLUDED

#include <memory>
#include <vector>
#include <string>
#include <map>
#include "Value.h"
#include "EngineBase.h"
#include "OdbcDriver.h"

class TestEngine;

namespace Yb {

typedef std::map<std::string, int> ParamNums;

class Engine : public EngineBase
{
    friend class ::TestEngine;

    SqlDriver *drv_;
public:
    Engine(mode work_mode = READ_ONLY);
    ~Engine();

private:
    RowsPtr on_select(const StrList &what,
            const StrList &from, const Filter &where,
            const StrList &group_by, const Filter &having,
            const StrList &order_by, int max_rows,
            bool for_update);
    const std::vector<LongInt> on_insert(
            const std::string &table_name,
            const Rows &rows, const FieldSet &exclude_fields,
            bool collect_new_ids);
    void on_update(const std::string &table_name,
            const Rows &rows, const FieldSet &key_fields,
            const FieldSet &exclude_fields, const Filter &where);
    void on_delete(const std::string &table_name, const Filter &where);
    void on_exec_proc(const std::string &proc_code);
    void on_commit();
    void on_rollback();

    void do_gen_sql_insert(std::string &sql, Values &params,
            ParamNums &param_nums, const std::string &table_name,
            const Row &row, const FieldSet &exclude_fields) const;
    void do_gen_sql_update(std::string &sql, Values &params,
            ParamNums &param_nums, const std::string &table_name,
            const Row &row, const FieldSet &key_fields,
            const FieldSet &exclude_fields, const Filter &where) const;
    void do_gen_sql_select(std::string &sql, Values &params,
            const StrList &what, const StrList &from, const Filter &where,
            const StrList &group_by, const Filter &having,
            const StrList &order_by, bool for_update) const;
    void do_gen_sql_delete(std::string &sql, Values &params,
            const std::string &table, const Filter &where) const;
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__ENGINE__INCLUDED
