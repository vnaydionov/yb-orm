
#ifndef YB__CORE__MAPPER_SESSION__INCLUDED
#define YB__CORE__MAPPER_SESSION__INCLUDED

#include "Mapper.h"
#include "SqlDataSource.h"
#include "OdbcSession.h"

namespace Yb {

class MapperSession
    : public ::Yb::SQL::Session
    , public ::Yb::ORMapper::Mapper
{
    SQL::OdbcSession session_;
    ORMapper::SqlDataSource ds_;
    ORMapper::TableMapper mapper_;
public:
    MapperSession(bool read_only = true);
    // mapper interface methods
    ORMapper::RowData *find(const ORMapper::RowData &key);
    ORMapper::LoadedRows load_collection(
            const std::string &table_name, const SQL::Filter &filter, 
	    const SQL::StrList &order_by = SQL::StrList(), int max = -1,
        const std::string &table_alias = "");
    ORMapper::RowData *create(const std::string &table_name);
    ORMapper::RowData *register_as_new(const ORMapper::RowData &row);
    void flush();
    const ORMapper::TableMetaDataRegistry &get_meta_data_registry();
private:
    // session policy methods
    SQL::RowsPtr on_select(const SQL::StrList &what,
            const SQL::StrList &from, const SQL::Filter &where,
            const SQL::StrList &group_by, const SQL::Filter &having,
            const SQL::StrList &order_by, int max_rows,
            bool for_update);
    const std::vector<long long> on_insert(const std::string &table_name,
            const SQL::Rows &rows, const SQL::FieldSet &exclude_fields,
            bool collect_new_ids);
    void on_update(const std::string &table_name,
            const SQL::Rows &rows, const SQL::FieldSet &key_fields,
            const SQL::FieldSet &exclude_fields, const SQL::Filter &where);
    void on_delete(const std::string &table_name, const SQL::Filter &where);
    void on_exec_proc(const std::string &proc_code);
    void on_commit();
    void on_rollback();
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et

#endif // YB__CORE__MAPPER_SESSION__INCLUDED

