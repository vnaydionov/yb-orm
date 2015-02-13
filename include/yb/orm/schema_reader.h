// -*- Mode: C++; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
#ifndef YB__ORM__SCHEMA_READER__INCLUDED
#define YB__ORM__SCHEMA_READER__INCLUDED

#include "sql_driver.h"
#include "schema.h"

namespace Yb {

class SchemaReader {
    SqlDriver &driver_;
    SqlDialect &dialect_;
public:
    SchemaReader(SqlDriver &driver, SqlDialect &dialect)
        : driver_(driver)
        , dialect_(dialect)
    {}
    SharedPtr<Schema> read_from_db(SqlConnection &connection);
};

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__SCHEMA_READER__INCLUDED
