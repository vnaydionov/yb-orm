#ifndef YB__ORM__SQL_DATA_SOURCE__INCLUDED
#define YB__ORM__SQL_DATA_SOURCE__INCLUDED

#include "RowData.h"
#include "Session.h"

class TestSqlDataSource;

namespace Yb {

class SqlDataSource : public DataSource
{
    friend class ::TestSqlDataSource;

    const TableMetaDataRegistry &reg_;
    Session &session_;
private:
    const RowData sql_row2row_data(const std::string &table_name, const Row &row);
    static RowPtr row_data2sql_row(const RowData &rd);
    RowsPtr row_data_vector2sql_rows(const TableMetaData &table,
            const RowDataVector &rows, int filter = -1);
    void do_insert_rows(const std::string &table_name,
            const RowDataVector &rows, bool process_autoinc);
public:
    SqlDataSource(const TableMetaDataRegistry &reg,
            Session &session);
    RowDataPtr select_row(const RowData &key);
    RowDataVectorPtr select_rows(
            const std::string &table_name, const Filter &filter, const StrList &order_by = StrList(),
            int max = -1, const std::string &table_alias = "");
    void insert_rows(const std::string &table_name,
            const RowDataVector &rows);
    void update_rows(const std::string &table_name,
            const RowDataVector &rows);
    void delete_row(const RowData &row);
    long long get_curr_id(const std::string &seq_name);
    long long get_next_id(const std::string &seq_name);
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__SQL_DATA_SOURCE__INCLUDED
