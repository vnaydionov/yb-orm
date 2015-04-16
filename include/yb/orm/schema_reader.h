// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__ORM__SCHEMA_READER__INCLUDED
#define YB__ORM__SCHEMA_READER__INCLUDED

#include "orm/sql_driver.h"
#include "orm/schema.h"

namespace Yb {

YBORM_DECL int get_sql_type_by_name(const String &sql_type, SqlDialect &sql_dialect);
std::string get_class_name(const std::string &table_name);
YBORM_DECL Schema::Ptr read_schema_from_db(SqlConnection &connection);

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__SCHEMA_READER__INCLUDED
