#include <sstream>
#include <fstream>
#include <util/str_utils.hpp>
#include <orm/CodeGen.h>

using namespace std;
using namespace Yb::StrUtils;

#define ORM_LOG(x) cerr << "yborm_gen: " << x << "\n";

namespace Yb {

CodeGenError::CodeGenError(const String &msg):
    RunTimeError(msg)
{}

namespace { // anonymous namespace

const char
    *AUTOGEN_BEGIN = "// AUTOGEN_BEGIN",
    *AUTOGEN_END = "// } AUTOGEN_END\n";

string MK_AUTOGEN_BEGIN(int n) {
    char buf[100];
    sprintf(buf, "// AUTOGEN_BEGIN(%03d) {\n", n);
    return string(buf);
}

const char *kwords[] = { "and", "and_eq", "asm", "auto", "bitand", "bitor",
    "bool", "break", "case", "catch", "char", "class", "compl", "const",
    "const_cast", "continue", "default", "delete", "do", "double",
    "dynamic_cast", "else", "enum", "explicit", "export", "extern", "false",
    "float", "for", "friend", "goto", "if", "inline", "int", "long",
    "mutable", "namespace", "new", "not", "not_eq", "operator", "or", "or_eq",
    "private", "protected", "public", "register", "reinterpret_cast",
    "return", "short", "signed", "sizeof", "static", "static_cast", "struct",
    "switch", "template", "this", "throw", "true", "try", "typedef", "typeid",
    "typename", "union", "unsigned", "using", "virtual", "void", "volatile",
    "wchar_t", "while", "xor", "xor_eq", NULL };

string fix_name(const string &name)
{
    for (int i = 0; kwords[i]; ++i)
        if (!name.compare(kwords[i]))
            return name + "_";
    return name;
}

string type_nc_by_handle(int type)
{
    switch (type) {
        case Value::INTEGER:  return "int";
        case Value::LONGINT:  return "Yb::LongInt";
        case Value::DATETIME: return "Yb::DateTime";
        case Value::STRING:   return "Yb::String";
        case Value::DECIMAL:  return "Yb::Decimal";
        default:
            throw CodeGenError(_T("Unknown type while parsing metadata"));
    }
}

string type_code_by_handle(int type)
{
    string code;
    switch (type) {
        case Value::INTEGER:  code = "INTEGER";  break;
        case Value::LONGINT:  code = "LONGINT";  break;
        case Value::DATETIME: code = "DATETIME"; break;
        case Value::STRING:   code = "STRING";   break;
        case Value::DECIMAL:  code = "DECIMAL";  break;
        default:
            throw CodeGenError(_T("Unknown type while parsing metadata"));
    }
    return "Yb::Value::" + code;
}

string flags_code_by_handle(int flags)
{
    string code;
    if (flags & Column::PK)
        code += "|Yb::Column::PK";
    if (flags & Column::RO)
        code += "|Yb::Column::RO";
    if (flags & Column::NULLABLE)
        code += "|Yb::Column::NULLABLE";
    if (code.empty())
        return "0";
    return code.substr(1);
}

string relation_type_to_str(int type) {
    string code = "UNKNOWN";
    switch (type) {
        case Relation::ONE2MANY: code = "ONE2MANY"; break;
        case Relation::MANY2MANY: code = "MANY2MANY"; break;
        case Relation::PARENT2CHILD: code = "PARENT2CHILD"; break;
    }
    return "Yb::Relation::" + code;
}

string cascade_code_to_str(int cascade) {
    string code = "Restrict";
    switch (cascade) {
        case Relation::Nullify: code = "Nullify"; break;
        case Relation::Delete: code = "Delete"; break;
    }
    return "Yb::Relation::" + code;
}

void dump_rel_attr(const Relation::AttrMap &attr, int n, ostream &out)
{
    Relation::AttrMap::const_iterator i = attr.begin(), iend = attr.end();
    for (; i != iend; ++i)
        out << "\t\tattr" << n << "[_T(\"" << NARROW(i->first)
            << "\")] = _T(\"" << NARROW(i->second) << "\");\n";
}

const string fk_rule(const Column &c)
{
    return "FOREIGN KEY (" + NARROW(c.get_name())
        + ") REFERENCES " + NARROW(c.get_fk_table_name())
        + "(" + NARROW(c.get_fk_name()) + ")";
}

} // end of anonymous namespace

void expand_tabs_to_stream(const string &in, ostream &out)
{
    string::const_iterator it = in.begin(), end = in.end();
    for (; it != end; ++it) {
        if (*it == '\t')
            out << "    ";
        else
            out << *it;
    }
}

bool create_backup(const char *fname)
{
    FILE *t = fopen(fname, "r");
    if (t) {
        fclose(t);
        string new_fname = fname;
        new_fname += ".bak";
        remove(new_fname.c_str());
        if (rename(fname, new_fname.c_str()) == -1) {
            throw CodeGenError(_T("Can't create backup file: ") +
                    WIDEN(new_fname));
        }
        ORM_LOG("Updating existing file: " << fname);
        return true;
    }
    return false;
}

void split_by_autogen(const string &fname,
        vector<string> &parts, vector<int> &stypes)
{
    parts.clear();
    stypes.clear();
    ifstream input((fname + ".bak").c_str());
    string s;
    char buf[4096 + 1];
    while (input.good()) {
        input.read(buf, sizeof(buf) - 1);
        buf[input.gcount()] = 0;
        s += buf;
    }
    size_t spos = 0;
    bool search_begin = true;
    while (1) {
        if (search_begin) {
            size_t pos = s.find(AUTOGEN_BEGIN, spos);
            if (pos == string::npos) {
                parts.push_back(s.substr(spos));
                break;
            }
            size_t eol = s.find("\n", pos);
            if (eol == string::npos) {
                throw CodeGenError(
                    _T("Can't find newline after AUTOGEN_BEGIN, file ")
                    + WIDEN(fname));
            }
            parts.push_back(s.substr(spos, eol - spos + 1));
            if (s[pos + strlen(AUTOGEN_BEGIN)] != '(' ||
                    s[pos + strlen(AUTOGEN_BEGIN) + 4] != ')')
                throw CodeGenError(
                    _T("Wrong format of AUTOGEN section, file ")
                    + WIDEN(fname));
            int stype = strtol(&s[pos + strlen(AUTOGEN_BEGIN) + 1], NULL, 10);
            stypes.push_back(stype);
            spos = eol + 1;
            search_begin = false;
        }
        else {
            size_t pos = s.find(AUTOGEN_END, spos);
            if (pos == string::npos) {
                throw CodeGenError(
                    _T("Can't find AUTOGEN_END in file: ")
                    + WIDEN(fname));
            }
            spos = pos;
            search_begin = true;
        }
    }
}

SqlTableGenerator::SqlTableGenerator(
        const Table &table, SqlDialect *dialect)
    : table_(table)
    , dialect_(dialect)
{}

void SqlTableGenerator::gen_typed_column(ostream &out,
        const Column &column)
{
    out << NARROW(column.get_name()) << " "
        << NARROW(dialect_->type2sql(column.get_type()));
    if (column.get_type() == Value::STRING)
        out << "(" << column.get_size() << ")";
    String default_clause;
    if (!column.get_default_value().is_null()) {
        default_clause = _T("DEFAULT ");
        if (column.get_type() == Value::DATETIME &&
                column.get_default_value().as_string() == _T("sysdate"))
            default_clause += dialect_->sysdate_func();
        else
            default_clause += dialect_->sql_value(column.get_default_value());
    }
    String not_null_default_clause = dialect_->not_null_default(
            column.is_nullable() && !column.is_pk()? _T(""): _T("NOT NULL"),
            default_clause);
    if (!str_empty(not_null_default_clause))
        out << " " << NARROW(not_null_default_clause);
    String autoinc_flag = dialect_->autoinc_flag();
    if (column.is_pk()
            && (table_.get_autoinc() || !str_empty(table_.get_seq_name()))
            && !str_empty(autoinc_flag))
    {
        String pk_flag = dialect_->primary_key_flag();
        if (!str_empty(pk_flag))
            out << " " << NARROW(pk_flag);
        out << " " << NARROW(autoinc_flag);
    }
}

void SqlTableGenerator::gen_create_table(ostream &out)
{
    out << "CREATE TABLE " << NARROW(table_.get_name()) << " (\n";
    Columns::const_iterator it = table_.begin(), end = table_.end();
    for (; it != end; ++it) {
        out << "\t";
        gen_typed_column(out, *it);
        if (it + 1 != end)
            out << ",";
        out << "\n";
    }
    String pk_flag = dialect_->primary_key_flag();
    if (str_empty(pk_flag)
            || !(table_.get_autoinc() || !str_empty(table_.get_seq_name())))
    {
        out << "\t, PRIMARY KEY (";
        bool first = true;
        for (it = table_.begin(); it != end; ++it)
            if (it->is_pk()) {
                if (!first)
                    out << ", ";
                else
                    first = false;
                out << NARROW(it->get_name());
            }
        out << ")\n";
    }
    if (dialect_->fk_internal()) {
        for (it = table_.begin(); it != end; ++it)
            if (it->has_fk())
                out << "\t, " << fk_rule(*it) << "\n";
    }
    out << ")" << NARROW(dialect_->suffix_create_table());
}

void SqlTableGenerator::gen_fk_constraints(ostream &out)
{
    if (!dialect_->fk_internal()) {
        Columns::const_iterator it = table_.begin(), end = table_.end();
        for (; it != end; ++it)
            if (it->has_fk())
                out << "ALTER TABLE " << NARROW(table_.get_name())
                    << " ADD " << fk_rule(*it) << ";\n";
    }
}

SqlSchemaGenerator::SqlSchemaGenerator(
        const Schema &schema, SqlDialect *dialect)
    : schema_(schema)
    , dialect_(dialect)
{}

void SqlSchemaGenerator::gen_commit(ostream &out)
{
    if (dialect_->commit_ddl())
        out << "COMMIT;\n\n";
}

void SqlSchemaGenerator::gen_create_tables(ostream &out)
{
    Schema::TblMap::const_iterator it = schema_.tbl_begin(),
        end = schema_.tbl_end();
    for (; it != end; ++it) {
        SqlTableGenerator tgen(*(it->second), dialect_);
        tgen.gen_create_table(out);
        out << ";\n\n";
        gen_commit(out);
    }
}

void SqlSchemaGenerator::gen_create_sequences(ostream &out)
{
    if (dialect_->has_sequences()) {
        set<String> sequences;
        Schema::TblMap::const_iterator it = schema_.tbl_begin(),
            end = schema_.tbl_end();
        for (; it != end; ++it)
            if (!str_empty(it->second->get_seq_name()))
                sequences.insert(it->second->get_seq_name());
        set<String>::iterator j = sequences.begin(),
            jend = sequences.end();
        for (; j != jend; ++j)
            out << NARROW(dialect_->gen_sequence(*j)) << ";\n";
        gen_commit(out);
    }
}

void SqlSchemaGenerator::gen_create_fk_constraints(ostream &out)
{
    Schema::TblMap::const_iterator it = schema_.tbl_begin(),
        end = schema_.tbl_end();
    for (; it != end; ++it) {
        SqlTableGenerator tgen(*(it->second), dialect_);
        tgen.gen_fk_constraints(out);
    }
    gen_commit(out);
}

void SqlSchemaGenerator::generate(ostream &out)
{
    out << "-- DBTYPE=" << NARROW(dialect_->get_name()) << "\n\n";
    gen_create_tables(out);
    gen_create_fk_constraints(out);
    gen_create_sequences(out);
}

CppCodeGenerator::CppCodeGenerator(const Schema &schema,
        const String &table_name,
        const string &path, const string &inc_prefix)
    : schema_(schema)
    , table_(schema_.table(table_name))
    , path_(path)
    , inc_prefix_(inc_prefix)
    , class_name_(NARROW(table_.get_class_name()))
    , table_name_(NARROW(table_.get_name()))
{
    autogen_handlers_[1] = &CppCodeGenerator::write_include_dependencies;
    autogen_handlers_[2] = &CppCodeGenerator::write_decl_relation_classes;
    autogen_handlers_[3] = &CppCodeGenerator::write_properties;
    autogen_handlers_[4] = &CppCodeGenerator::write_meta;
    autogen_handlers_[5] = &CppCodeGenerator::write_props_cons_calls;
    autogen_handlers_[6] = &CppCodeGenerator::do_write_cpp_ctor_body;
}

void CppCodeGenerator::write_autogen_body(ostream &out, int code)
{
    (this->*(autogen_handlers_[code]))(out);
}

void CppCodeGenerator::write_autogen(ostream &out, int code)
{
    out << MK_AUTOGEN_BEGIN(code);
    write_autogen_body(out, code);
    out << AUTOGEN_END;
}

void CppCodeGenerator::write_include_dependencies(ostream &out)
{
    Columns::const_iterator it = table_.begin(), end = table_.end();
    for (; it != end; ++it)
        if (it->has_fk())
            out << "#include \"" << inc_prefix_ <<
                NARROW(schema_.table(it->get_fk_table_name()).get_class_name())
                << ".h\"\n";
}

void CppCodeGenerator::write_decl_relation_classes(ostream &out)
{
    Schema::RelMap::const_iterator
        i = schema_.rels_lower_bound(table_.get_class_name()),
        iend = schema_.rels_upper_bound(table_.get_class_name());
    set<String> classes;
    for (; i != iend; ++i) {
        if (i->second->type() == Relation::ONE2MANY &&
                table_.get_class_name() == i->second->side(0) &&
                i->second->has_attr(0, _T("property")))
        {
            classes.insert(i->second->side(1));
        }
    }
    set<String>::const_iterator j = classes.begin(), jend = classes.end();
    for (; j != jend; ++j)
        out << "class " << NARROW(*j) << ";\n";
}

void CppCodeGenerator::write_properties(ostream &out)
{
    out << "\t// properties\n";
    Columns::const_iterator it = table_.begin(), end = table_.end();
    for (int count = 0; it != end; ++it, ++count)
        if (!it->has_fk()) {
            out << "\tYb::Property<" << type_nc_by_handle(it->get_type())
                << ", " << count << "> "
                << fix_name(NARROW(it->get_prop_name())) << ";\n";
        }
    out << "\t// relation properties\n";
    typedef map<String, String> MapString;
    MapString map_fk;
    for (it = table_.begin(); it != end; ++it)
        if (it->has_fk())
            map_fk.insert(MapString::value_type(
                        it->get_fk_table_name(), it->get_name()));
    Schema::RelMap::const_iterator
        i = schema_.rels_lower_bound(table_.get_class_name()),
        iend = schema_.rels_upper_bound(table_.get_class_name());
    for (; i != iend; ++i) {
        if (i->second->type() == Relation::ONE2MANY &&
                table_.get_class_name() == i->second->side(1) &&
                i->second->has_attr(1, _T("property")))
        {
            String prop = i->second->attr(1, _T("property")),
                   class_one = i->second->side(0);
            out << "\t" << NARROW(class_one) << "Holder "
                << NARROW(prop) << ";\n";
        }
    }
    i = schema_.rels_lower_bound(table_.get_class_name());
    iend = schema_.rels_upper_bound(table_.get_class_name());
    for (; i != iend; ++i) {
        if (i->second->type() == Relation::ONE2MANY &&
                table_.get_class_name() == i->second->side(0) &&
                i->second->has_attr(0, _T("property")))
        {
            String prop = i->second->attr(0, _T("property")),
                   class_many = i->second->side(1);
            out << "\tYb::ManagedList<" << NARROW(class_many)
                << "> " << NARROW(prop) << ";\n";
        }
    }
}

void CppCodeGenerator::write_h_file_header(ostream &out)
{
    string def_name = "ORM_DOMAIN__" +
        NARROW(str_to_upper(table_.get_class_name())) + "__INCLUDED";
    out << "#ifndef " << def_name << "\n"
        << "#define " << def_name << "\n\n"
        << "#include <orm/DomainObj.h>\n";
    write_autogen(out, 1); // include_dependencies
    out << "\n"
        << "namespace Domain {\n\n";
    write_autogen(out, 2); // decl_relation_classes
    out << "\n"
        << "class " << class_name_ << ";\n"
        << "typedef Yb::DomainObjHolder<" << class_name_ << "> " << class_name_ << "Holder;\n\n"
        << "class " << class_name_ << ": public Yb::DomainObject\n"
        << "{\n"
        << "public:\n"
        << "\tstatic const Yb::String get_table_name() { return _T(\""
        << table_name_ << "\"); }\n"
        << "\ttypedef Yb::DomainResultSet<"
        << class_name_ << "> ResultSet;\n"
        << "\t// static method 'find'\n"
        << "\ttypedef std::vector<" << class_name_ << "> List;\n"
        << "\ttypedef std::auto_ptr<List> ListPtr;\n"
        << "\tstatic ListPtr find(Yb::Session &session,\n"
        << "\t\t\tconst Yb::Filter &filter, const Yb::StrList order_by = _T(\"\"), int max_n = -1);\n"
        << "\t// constructors\n"
        << "\t" << class_name_ << "(Yb::DomainObject *owner, const Yb::String &prop_name);\n"
        << "\t" << class_name_ << "();\n"
        << "\t" << class_name_ << "(const " << class_name_ << " &other);\n"
        << "\texplicit " << class_name_ << "(Yb::Session &session);\n"
        << "\texplicit " << class_name_ << "(Yb::DataObject::Ptr d);\n"
        << "\t" << class_name_ << "(Yb::Session &session, const Yb::Key &key);\n";
    try {
        String mega_key = table_.get_unique_pk();
        out << "\t" << class_name_ << "(Yb::Session &session, Yb::LongInt id);\n";
    }
    catch (const AmbiguousPK &)
    {}
    out << "\t" << class_name_ << " &operator=(const " << class_name_ << " &other);\n"
        << "\tstatic void create_tables_meta(Yb::Tables &tbls);\n"
        << "\tstatic void create_relations_meta(Yb::Relations &rels);\n";
}

void CppCodeGenerator::write_h_file_footer(ostream &out)
{
    out << "};\n\n"
        << "struct "<< class_name_ << "Registrator\n{\n"
        << "\tstatic void register_domain();\n"
        << "\t" << class_name_ << "Registrator();\n"
        << "};\n\n"
        << "} // namespace Domain\n\n"
        << "// vim:ts=4:sts=4:sw=4:et:\n"
        << "#endif\n";
}

void CppCodeGenerator::update_h_file()
{
    string file_path = path_ + "/" + class_name_ + ".h";
    ORM_LOG("Generating file: " << file_path
            << " for table '" << table_name_ << "'");
    bool update_h = create_backup(file_path.c_str());
    ostringstream out;
    if (update_h) {
        vector<string> parts;
        vector<int> stypes;
        split_by_autogen(file_path, parts, stypes);
        if (parts.size() != stypes.size() + 1)
            throw CodeGenError(
                _T("Error while parsing AUTOGEN sections in file: ")
                + WIDEN(file_path));
        for (int i = 0; i < stypes.size(); ++i) {
            out << parts[i];
            write_autogen_body(out, stypes[i]);
        }
        out << parts[stypes.size()];
    }
    else {
        write_h_file_header(out);
        write_autogen(out, 3); // properties
        write_h_file_footer(out);
    }
    ofstream file(file_path.c_str());
    if (!file.good())
        throw CodeGenError(_T("Can't write to file"));
    expand_tabs_to_stream(out.str(), file);
}

void CppCodeGenerator::write_cpp_meta_globals(ostream &out)
{
    out << "namespace {\n"
        << "\tYb::Tables tbls;\n"
        << "\tYb::Relations rels;\n"
        << "\tYb::DomainMetaDataCreator<" << class_name_ << "> mdc(tbls, rels);\n"
        << "}\n\n";
}

void CppCodeGenerator::write_cpp_create_table_meta(ostream &out)
{
    string xml_name = NARROW(table_.get_xml_name());
    out << "void " << class_name_ << "::create_tables_meta(Yb::Tables &tbls)\n"
        << "{\n"
        << "\tYb::Table::Ptr t(new Yb::Table(_T(\"" << table_name_
        << "\"), _T(\"" << xml_name << "\"), _T(\"" << class_name_ << "\")));\n";
    if (!str_empty(table_.get_seq_name()))
        out << "\tt->set_seq_name(_T(\"" << NARROW(table_.get_seq_name()) << "\"));\n";
    Columns::const_iterator i = table_.begin(), iend = table_.end();
    for (; i != iend; ++i) {
        const Column &c = *i;
        out << "\t{\n"
            << "\t\tYb::Column c(_T(\"" << NARROW(c.get_name()) << "\"), "
            << type_code_by_handle(c.get_type()) << ", " << c.get_size()
            << ", " << flags_code_by_handle(c.get_flags())
            << ", _T(\"" << NARROW(c.get_fk_table_name())
            << "\"), _T(\"" << NARROW(c.get_fk_name())
            << "\"), _T(\"" << NARROW(c.get_xml_name())
            << "\"), _T(\"" << NARROW(c.get_prop_name()) << "\"));\n";
        if (!c.get_default_value().is_null())
            out << "\t\tc.set_default_value(Yb::Value(_T(\""
                << NARROW(c.get_default_value().as_string()) << "\")));\n";
        out << "\t\tt->add_column(c);\n"
            << "\t}\n";
    }
    out << "\ttbls.push_back(t);\n"
        << "}\n";
}

void CppCodeGenerator::write_cpp_create_relations_meta(ostream &out)
{
    string xml_name = NARROW(table_.get_xml_name());
    out << "void " << class_name_ << "::create_relations_meta(Yb::Relations &rels)\n"
        << "{\n";
    Schema::RelMap::const_iterator
        i = schema_.rels_lower_bound(table_.get_class_name()),
        iend = schema_.rels_upper_bound(table_.get_class_name());
    for (; i != iend; ++i)
    {
        out << "\t{\n"
            << "\t\tYb::Relation::AttrMap attr1, attr2;\n";
        dump_rel_attr(i->second->attr_map(0), 1, out);
        dump_rel_attr(i->second->attr_map(1), 2, out);
        out << "\t\tYb::Relation::Ptr r(new Yb::Relation("
            << relation_type_to_str(i->second->type()) << ", _T(\""
            << NARROW(i->second->side(0)) << "\"), attr1, _T(\""
            << NARROW(i->second->side(1)) << "\"), attr2, "
            << cascade_code_to_str(i->second->cascade()) << "));\n"
            << "\t\trels.push_back(r);\n"
            << "\t}\n";
    }
    out << "}\n";
}

void CppCodeGenerator::write_meta(ostream &out)
{
    write_cpp_create_table_meta(out);
    out << "\n";
    write_cpp_create_relations_meta(out);
}

void CppCodeGenerator::write_props_cons_calls(ostream &out)
{
    typedef map<String, String> MapString;
    MapString map_fk;
    Columns::const_iterator it = table_.begin(), end = table_.end();
    for (int count = 0; it != end; ++it, ++count) {
        if (!it->has_fk()) {
            out << "\t, " << fix_name(NARROW(it->get_prop_name()))
                << "(this)\n";
        }
        else {
            map_fk.insert(MapString::value_type(
                        it->get_fk_table_name(), it->get_name()));
        }
    }
    Schema::RelMap::const_iterator
        i = schema_.rels_lower_bound(table_.get_class_name()),
        iend = schema_.rels_upper_bound(table_.get_class_name());
    for (; i != iend; ++i) {
        if (i->second->type() == Relation::ONE2MANY &&
                table_.get_class_name() == i->second->side(1) &&
                i->second->has_attr(1, _T("property")))
        {
            String prop = i->second->attr(1, _T("property"));
            out << "\t, " << NARROW(prop) << "(this, _T(\"" << NARROW(prop)
                << "\"))\n";
        }
    }
    i = schema_.rels_lower_bound(table_.get_class_name());
    iend = schema_.rels_upper_bound(table_.get_class_name());
    for (; i != iend; ++i) {
        if (i->second->type() == Relation::ONE2MANY &&
                table_.get_class_name() == i->second->side(0) &&
                i->second->has_attr(0, _T("property")))
        {
            String prop = i->second->attr(0, _T("property"));
            out << "\t, " << NARROW(prop) << "(this, _T(\"" << NARROW(prop)
                << "\"))\n";
        }
    }
}

void CppCodeGenerator::do_write_cpp_ctor_body(ostream &out)
{
    Columns::const_iterator it = table_.begin(), end = table_.end();
    for (int i = 0; it != end; ++it, ++i)
        if (!it->get_default_value().is_null()) {
            out << "\tset(" << i << ", Yb::Value(";
            string default_value = NARROW(it->get_default_value().as_string());
            switch (it->get_type()) {
                case Value::INTEGER:
                    out << "(int)" << default_value;
                    break;
                case Value::LONGINT:
                    out << "(Yb::LongInt)" << default_value;
                    break;
                case Value::DECIMAL:
                    out << "Yb::Decimal(_T(\"" << default_value << "\"))";
                    break;
                case Value::DATETIME:
                    out << "Yb::now()";
                    break;
            }
            out << "));\n";
        }
}

void CppCodeGenerator::write_cpp_ctor_body(ostream &out, bool save_to_session)
{
    out << "{\n";
    write_autogen(out, 6);
    if (save_to_session)
        out << "\tsave(session);\n";
    out << "}\n";
}

void CppCodeGenerator::write_cpp_file(ostream &out)
{
    out << "#include \"" << inc_prefix_ << class_name_ << ".h\"\n"
        << "#include <orm/DomainFactorySingleton.h>\n"
        << "namespace Domain {\n\n";
    write_cpp_meta_globals(out);
    write_autogen(out, 4); // meta
    out << "\n"
        << class_name_ << "::"
        << class_name_ << "(Yb::DomainObject *owner, const Yb::String &prop_name)\n"
        << "\t: Yb::DomainObject(*tbls[0], owner, prop_name)\n";
    write_autogen(out, 5); // props_cons_calls
    out << "{}\n\n"
        << class_name_ << "::"
        << class_name_ << "()\n"
        << "\t: Yb::DomainObject(*tbls[0])\n";
    write_autogen(out, 5); // props_cons_calls
    write_cpp_ctor_body(out, false);
    out << "\n"
        << class_name_ << "::"
        << class_name_ << "(const " << class_name_ << " &other)\n"
        << "\t: Yb::DomainObject(other)\n";
    write_autogen(out, 5); // props_cons_calls
    out << "{}\n\n"
        << class_name_ << "::"
        << class_name_ << "(Yb::Session &session)\n"
        << "\t: Yb::DomainObject(session.schema(), _T(\"" << table_name_ << "\"))\n";
    write_autogen(out, 5); // props_cons_calls
    write_cpp_ctor_body(out, true);
    out << "\n"
        << class_name_ << "::"
        << class_name_ << "(Yb::DataObject::Ptr d)\n"
        << "\t: Yb::DomainObject(d)\n";
    write_autogen(out, 5); // props_cons_calls
    out << "{}\n\n"
        << class_name_ << "::"
        << class_name_ << "(Yb::Session &session, const Yb::Key &key)\n"
        << "\t: Yb::DomainObject(session, key)\n";
    write_autogen(out, 5); // props_cons_calls
    out << "{}\n\n";
    try {
        String mega_key = table_.get_unique_pk();
        out << class_name_ << "::"
            << class_name_ << "(Yb::Session &session, Yb::LongInt id)\n"
            << "\t: Yb::DomainObject(session, _T(\""
            << table_name_ << "\"), id)\n";
        write_autogen(out, 5); // props_cons_calls
        out << "{}\n\n";
    }
    catch (const AmbiguousPK &)
    {}
    out << class_name_ << " &" << class_name_ << "::operator=(const "
        << class_name_ << " &other)\n"
        << "{\n"
        << "\tif (this != &other) {\n"
        << "\t\t*(Yb::DomainObject *)this = (const Yb::DomainObject &)other;\n"
        << "\t}\n"
        << "\treturn *this;\n"
        << "}\n\n"
        << class_name_ << "::ListPtr\n"
        << class_name_ << "::find(Yb::Session &session,\n"
        << "\t\tconst Yb::Filter &filter, const Yb::StrList order_by, int max_n)\n"
        << "{\n"
        << "\t" << class_name_ << "::ListPtr lst(new "
        << class_name_ << "::List());\n"
        << "\tYb::ObjectList rows;\n"
        << "\tsession.load_collection(rows, _T(\"" << table_name_ << "\"), filter, order_by, max_n);\n"
        << "\tif (rows.size()) {\n"
        << "\t\tYb::ObjectList::iterator it = rows.begin(), end = rows.end();\n"
        << "\t\tfor (; it != end; ++it)\n"
        << "\t\t\tlst->push_back(" << class_name_ << "(*it));\n"
        << "\t}\n"
        << "\treturn lst;\n"
        << "}\n\n"
        << "void " << class_name_ << "Registrator::register_domain()\n{\n"
        << "\tYb::theDomainFactory::instance().register_creator(_T(\"" << table_name_ << "\"),\n"
        << "\t\tYb::CreatorPtr(new Yb::DomainCreator<" << class_name_ << ">()));\n"
        << "}\n\n"
        << class_name_ << "Registrator::"
        << class_name_ << "Registrator()\n{\n"
        << "\tregister_domain();\n}\n\n"
        << "static " << class_name_ << "Registrator " << class_name_ << "_registrator;\n\n"
        << "} // end namespace Domain\n\n"
        << "// vim:ts=4:sts=4:sw=4:et:\n";
}

void CppCodeGenerator::update_cpp_file()
{
    string file_path = path_ + "/" + class_name_ + ".cpp";
    ORM_LOG("Generating cpp file: " << file_path
            << " for table '" << table_name_ << "'");
    bool update_cpp = create_backup(file_path.c_str());
    ostringstream out;
    if (update_cpp) {
        vector<string> parts;
        vector<int> stypes;
        split_by_autogen(file_path, parts, stypes);
        if (parts.size() != stypes.size() + 1)
            throw CodeGenError(
                _T("Error while parsing AUTOGEN sections in file: ")
                + WIDEN(file_path));
        for (int i = 0; i < stypes.size(); ++i) {
            out << parts[i];
            write_autogen_body(out, stypes[i]);
        }
        out << parts[stypes.size()];
    }
    else
        write_cpp_file(out);
    ofstream cpp_file(file_path.c_str());
    if (!cpp_file.good())
        throw CodeGenError(_T("Can't write to file"));
    expand_tabs_to_stream(out.str(), cpp_file);
}

void generate_domain(const Schema &schema,
        const string &path, const string &inc_prefix)
{
    Schema::TblMap::const_iterator it = schema.tbl_begin(),
        end = schema.tbl_end();
    for (; it != end; ++it)
        if (!str_empty(schema.table(it->first).get_class_name())) {
            CppCodeGenerator cgen(schema, it->first, path, inc_prefix);
            cgen.update_h_file();
            cgen.update_cpp_file();
        }
}

void generate_ddl(const Schema &schema,
        const string &path, const string &dialect_name)
{
    SqlSchemaGenerator sql_gen(schema, sql_dialect(WIDEN(dialect_name)));
    ostringstream out;
    sql_gen.generate(out);
    if (path.empty())
        expand_tabs_to_stream(out.str(), cout);
    else {
        ofstream sql_file(path.c_str());
        if (!sql_file.good())
            throw CodeGenError(_T("Can't write to file"));
        expand_tabs_to_stream(out.str(), sql_file);
    }
}

} // end of namespace Yb
// vim:ts=4:sts=4:sw=4:et:
