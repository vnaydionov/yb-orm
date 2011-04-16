#ifndef YB__ORM__ENGINE_SESSION__INCLUDED
#define YB__ORM__ENGINE_SESSION__INCLUDED

#include "Session.h"
#include "SqlDataSource.h"
#include "Engine.h"

namespace Yb {

class EngineSession
    : public EngineBase
    , public SessionBase
{
    Engine engine_;
    SqlDataSource ds_;
    Session session_;
public:
    EngineSession(bool read_only = true, SqlConnect *conn = NULL);
    // session interface methods
    RowData *find(const RowData &key);
    LoadedRows load_collection(
            const std::string &table_name, const Filter &filter, 
	    const StrList &order_by = StrList(), int max = -1,
        const std::string &table_alias = "");
    RowData *create(const std::string &table_name);
    RowData *register_as_new(const RowData &row);
    void flush();
    const TableMetaDataRegistry &get_meta_data_registry();
    SqlConnect *get_connect() { return engine_.get_connect(); }
private:
    // engine policy methods
    RowsPtr on_select(const StrList &what,
            const StrList &from, const Filter &where,
            const StrList &group_by, const Filter &having,
            const StrList &order_by, int max_rows,
            bool for_update);
    const std::vector<LongInt> on_insert(const std::string &table_name,
            const Rows &rows, const FieldSet &exclude_fields,
            bool collect_new_ids);
    void on_update(const std::string &table_name,
            const Rows &rows, const FieldSet &key_fields,
            const FieldSet &exclude_fields, const Filter &where);
    void on_delete(const std::string &table_name, const Filter &where);
    void on_exec_proc(const std::string &proc_code);
    void on_commit();
    void on_rollback();
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__ENGINE_SESSION__INCLUDED
