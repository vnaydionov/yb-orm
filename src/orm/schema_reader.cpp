// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#define YBORM_SOURCE

#include "util/string_utils.h"
#include "orm/schema_reader.h"

namespace Yb {

using namespace Yb::StrUtils;

YBORM_DECL int
get_sql_type_by_name(const String &sql_type, SqlDialect &sql_dialect)
{
    int codes[] = { Value::INTEGER, Value::LONGINT, Value::STRING,
                    Value::DECIMAL, Value::DATETIME, Value::FLOAT };
    int codes_sz = sizeof(codes)/sizeof(int);
    for (int i = 0; i < codes_sz; ++i)
    {
        if (sql_dialect.type2sql(codes[i]) == sql_type)
            return codes[i];
    }
    // TODO: more elaborate mapping
    return Value::DECIMAL;
}

YBORM_DECL Schema::Ptr
read_schema_from_db(SqlConnection &connection)
{
    Schema::Ptr s(new Schema());
    Strings tables = connection.get_tables();
    for (Strings::const_iterator i = tables.begin(); i != tables.end(); ++i)
    {
        ColumnsInfo ci = connection.get_columns(*i);
        Table::Ptr t(new Table(*i));
        for (ColumnsInfo::const_iterator j = ci.begin(); j != ci.end(); ++j)
        {
            int flags = 0;
            if (j->pk)
                flags |= Column::PK;
            if (!j->notnull)
                flags |= Column::NULLABLE;
            SqlDialect *dialect = connection.get_dialect();
            String def_val = j->default_value;
            if (str_to_upper(def_val) == str_to_upper(dialect->sysdate_func()))
                def_val = _T("SYSDATE");
            Column c(j->name,
                     get_sql_type_by_name(j->type, *dialect),
                     j->size,
                     flags,
                     def_val,
                     j->fk_table,
                     j->fk_table_key);
            t->add_column(c);
        }
        s->add_table(t);
    }
    s->fill_fkeys();
    s->check_cycles();
    return s;
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
