#include <cstring>
#include <cstdlib>
#include <iostream>
#include "orm/schema.h"
#include "orm/schema_config.h"
#include "orm/schema_reader.h"
#include "orm/engine.h"
#include "orm/code_gen.h"

using namespace std;
using namespace Yb;

#define ORM_LOG(x) cerr << "yborm_gen: " << x << "\n";

struct Params {
    string config, output_path, include_prefix, dialect_name,
           connection_url;
};

enum Mode { NONE, GEN_DOMAIN, GEN_DDL,
            POPULATE_SCHEMA, DROP_SCHEMA, EXTRACT_SCHEMA } mode = NONE;

void usage()
{
    cerr << "Usage:\n"
        << "    yborm_gen --domain config.xml output_path [include_prefix]\n"
        << "    yborm_gen --ddl config.xml dialect_name [output.sql]\n"
        << "    yborm_gen --populate-schema config.xml connection_url\n"
        << "    yborm_gen --drop-schema config.xml connection_url\n"
        << "    yborm_gen --extract-schema config.xml connection_url\n\n";
    exit(1);
}

Mode parse_params(int argc, char *argv[], Params &params)
{
    Mode mode = NONE;
    if (argc < 4 || argc > 5)
        return mode;
    params.config = argv[2];
    if (!strcmp(argv[1], "--domain")) {
        mode = GEN_DOMAIN;
        params.output_path = argv[3];
        if (argc == 5)
            params.include_prefix = argv[4];
        else
            params.include_prefix = "domain/";
    }
    else if (!strcmp(argv[1], "--ddl")) {
        mode = GEN_DDL;
        params.dialect_name = argv[3];
        if (argc == 5)
            params.output_path = argv[4];
    }
    else if (!strcmp(argv[1], "--populate-schema")) {
        mode = POPULATE_SCHEMA;
        params.connection_url = argv[3];
    }
    else if (!strcmp(argv[1], "--drop-schema")) {
        mode = DROP_SCHEMA;
        params.connection_url = argv[3];
    }
    else if (!strcmp(argv[1], "--extract-schema")) {
        mode = EXTRACT_SCHEMA;
        params.connection_url = argv[3];
    }
    return mode;
}

int main(int argc, char *argv[])
{
    Params params;
    Mode mode = parse_params(argc, argv, params);
    if (mode == NONE)
        usage();
    LogAppender appender(cerr);
    ILogger::Ptr root_logger(new Logger(&appender));
    try {
        if (mode == GEN_DDL || mode == GEN_DOMAIN) {
            Schema r;
            load_schema(WIDEN(params.config), r);
            ORM_LOG("table count: " << r.tbl_count());
            ORM_LOG("generation started...");
            if (mode == GEN_DOMAIN)
                generate_domain(r, params.output_path, params.include_prefix);
            else
                generate_ddl(r, params.output_path, params.dialect_name);
            ORM_LOG("generation successfully finished");
        }
        else if (mode == DROP_SCHEMA || mode == POPULATE_SCHEMA) {
            Schema r;
            load_schema(WIDEN(params.config), r);
            auto_ptr<SqlConnection> conn(
                    new SqlConnection(WIDEN(params.connection_url)));
            ORM_LOG("Connected to " + params.connection_url);
            conn->set_convert_params(true);
            Yb::Engine engine(Yb::Engine::READ_WRITE, conn);
            engine.set_logger(root_logger);
            engine.set_echo(true);
            engine.touch();
            if (mode == DROP_SCHEMA)
                engine.drop_schema(r, true);
            else
                engine.create_schema(r, true);
            engine.commit();
        }
        else if (mode == EXTRACT_SCHEMA) {
            SqlConnection conn(WIDEN(params.connection_url));
            ORM_LOG("Connected to " + params.connection_url);
            conn.set_convert_params(true);
            conn.init_logger(root_logger.get());
            conn.set_echo(true);
            Schema::Ptr s = read_schema_from_db(conn);
            s->export_xml(WIDEN(params.config), true);
        }
    }
    catch (exception &e) {
        string error = string("Exception: ") + e.what();
        ORM_LOG(error);
        return 1;
    }
    catch (...) {
        ORM_LOG("Unknown exception");
        return 1;
    }
    return 0;
}
// vim:ts=4:sts=4:sw=4:et:
