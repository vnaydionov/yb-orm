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

YBORM_DECL const String
guess_class_name(const String &table_name)
{
    String result;
    size_t start = 0, end = str_length(table_name);
    if (starts_with(str_to_upper(table_name), _T("T_")))
    {
        start += 2;
    }
    else if (starts_with(str_to_upper(table_name), _T("TBL_")))
    {
        start += 4;
    }
    if (ends_with(str_to_upper(table_name), _T("_TBL")))
    {
        end -= 4;
    }
    str_append(result, to_upper(table_name[start]));
    for (size_t i = start + 1; i < end; ++i)
    {
        if (table_name[i] == _T('_'))
        {
            ++i;
            if (i < end)
            {
                str_append(result, to_upper(table_name[i]));
            }
        }
        else
        {
            str_append(result, to_lower(table_name[i]));
        }
    }
    return result;
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
            int type = get_sql_type_by_name(j->type, *dialect);
            if (type == (int)Value::INTEGER &&
                    (j->pk || !str_empty(j->fk_table)))
                type = Value::LONGINT;
            Column c(j->name,
                     type,
                     j->size,
                     flags,
                     def_val,
                     j->fk_table,
                     j->fk_table_key);
            t->add_column(c);
        }
        t->set_class_name(guess_class_name(t->name()));
        s->add_table(t);
    }

    s->fill_fkeys();
    s->check_cycles();
    for(Schema::TblMap::const_iterator i = s->tbl_begin(); i != s->tbl_end(); ++i)
    {
        const Table &t = *i->second;
        for(Columns::const_iterator j = t.begin(); j != t.end(); ++j)
        {
            const Column &c = *j;
            if (c.has_fk())
            {
                const String side1 = guess_class_name(c.fk_table_name());
                const String side2 = guess_class_name(t.name());
                Relation::AttrMap a1, a2;
                std::string a2_name = c.fk_table_name();
                std::string a2_result;
                for (int k = 2; k < a2_name.size(); ++k)
                    a2_result.push_back(to_lower(a2_name[k]));
                a2.insert(std::pair <std::string, std::string>("property", a2_result));
                Relation::Ptr r(new Relation(Relation::ONE2MANY, side1, a1, side2, a2));
                s->add_relation(r);
            }
        }
    }
    return s;
}

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
