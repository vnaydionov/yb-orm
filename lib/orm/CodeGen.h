// -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#ifndef YB__ORM__CODE_GEN__INCLUDED
#define YB__ORM__CODE_GEN__INCLUDED

#include <vector>
#include <set>
#include <map>
#include <string>
#include <iostream>
#include <util/Exception.h>
#include <orm/MetaData.h>
#include <orm/SqlDriver.h>

namespace Yb {

class CodeGenError: public RunTimeError {
public:
    CodeGenError(const String &msg);
};

void expand_tabs_to_stream(const std::string &in, std::ostream &out);
bool create_backup(const char *fname);
void split_by_autogen(const std::string &fname,
        std::vector<std::string> &parts, std::vector<int> &stypes);

class SqlTableGenerator
{
    const Table &table_;
    SqlDialect *dialect_;
public:
    SqlTableGenerator(const Table &table, SqlDialect *dialect);
    void gen_typed_column(std::ostream &out, const Column &column);
    void gen_create_table(std::ostream &out);
    void gen_fk_constraints(std::ostream &out);
};

class SqlSchemaGenerator
{
    const Schema &schema_;
    SqlDialect *dialect_;
    std::set<String> sequences_;
    bool need_commit_, new_table_;
    Schema::TblMap::const_iterator tbl_it_, tbl_constr_it_;
    Columns::const_iterator col_it_, col_end_;
    std::set<String>::const_iterator seq_it_;
public:
    SqlSchemaGenerator(const Schema &schema, SqlDialect *dialect);
    void generate(std::ostream &out);
    bool generate_next_statement(String &out);
};

class CppCodeGenerator
{
    const Schema &schema_;
    const Table &table_;
    const std::string path_, inc_prefix_, class_name_, table_name_;

    typedef void (CppCodeGenerator::* AutoGenHandler)(std::ostream &);
    std::map<int, AutoGenHandler> autogen_handlers_;
public:
    CppCodeGenerator(const Schema &schema, const String &table_name,
            const std::string &path, const std::string &inc_prefix);
    void write_autogen_body(std::ostream &out, int code);
    void write_autogen(std::ostream &out, int code);
    void write_include_dependencies(std::ostream &out);
    void write_decl_relation_classes(std::ostream &out);
    void write_domain_columns_decl(std::ostream &out);
    void write_properties(std::ostream &out);
    void write_h_file_header(std::ostream &out);
    void write_h_file_footer(std::ostream &out);
    void update_h_file();
    void write_cpp_meta_globals(std::ostream &out);
    void write_cpp_create_table_meta(std::ostream &out);
    void write_cpp_create_relations_meta(std::ostream &out);
    void write_meta(std::ostream &out);
    void write_props_cons_calls(std::ostream &out);
    void do_write_cpp_ctor_body(std::ostream &out);
    void write_cpp_ctor_body(std::ostream &out, bool save_to_session);
    void write_cpp_file(std::ostream &out);
    void update_cpp_file();
};

void generate_domain(const Schema &schema,
        const std::string &path, const std::string &inc_prefix);
void generate_ddl(const Schema &schema,
        const std::string &path, const std::string &dialect_name);

} // namespace Yb

// vim:ts=4:sts=4:sw=4:et:
#endif // YB__ORM__CODE_GEN__INCLUDED
