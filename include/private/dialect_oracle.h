// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__ORM__DIALECT_ORACLE__INCLUDED
#define YB__ORM__DIALECT_ORACLE__INCLUDED

#include "orm/sql_driver.h"

namespace Yb {

class YBORM_DECL OracleDialect: public SqlDialect
{
public:
    OracleDialect();
    virtual const String select_curr_value(const String &seq_name);
    virtual const String select_next_value(const String &seq_name);
    virtual const String sql_value(const Value &x);
    virtual const String type2sql(int t);
    virtual const String create_sequence(const String &seq_name);
    virtual const String drop_sequence(const String &seq_name);
    virtual const String sysdate_func();
    virtual int pager_model();
    // schema introspection
    virtual bool table_exists(SqlConnection &conn, const String &table);
    virtual bool view_exists(SqlConnection &conn, const String &table);
    virtual Strings get_tables(SqlConnection &conn);
    virtual Strings get_views(SqlConnection &conn);
    virtual ColumnsInfo get_columns(SqlConnection &conn, const String &table);
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__DIALECT_ORACLE__INCLUDED
