#include <iostream>
#include <map>
#include <fstream>
#include <util/str_utils.hpp>
//#include <util/logger.hpp>
#include "orm/MetaData.h"
#include "orm/Value.h"
#include "orm/XMLMetaDataConfig.h"

//#define ORM_LOG(x) LOG(LogLevel::INFO, "ORM Domain generator: " << x)
#define ORM_LOG(x) cout << "ORM Domain generator: " << x << "\n";

using namespace std;
using namespace Yb::StrUtils;

namespace Yb {

class OrmDomainGenerator
{
public:
    OrmDomainGenerator(const string &path, const Schema &reg)
        : path_(path)
        , reg_(reg)
        , mem_weak(false)
    {}

    void generate(const XMLMetaDataConfig &cfg)
    {
        ORM_LOG("generation started...");
        Schema::Map::const_iterator it = reg_.begin(), end = reg_.end();
        for (; it != end; ++it)
            if(cfg.need_generation(it->first))
                generate_table(it->second);
    }

private:
    void init_weak(const Table &t)
    {
        mem_weak = false;
        try {
            reg_.get_table(t.get_name()).get_synth_pk();
        } catch (const NotSuitableForAutoCreating &) {
            mem_weak = true;
        }
    }

    void write_header(const Table &t, ostream &str)
    {
        string class_name = NARROW(t.get_class_name()), table_name = NARROW(t.get_name());
        init_weak(t);
        string name = "ORM_DOMAIN__" + NARROW(str_to_upper(t.get_name().substr(2))) + "__INCLUDED";
        str << "#ifndef " << name << "\n"
            << "#define " << name << "\n\n";
        str << "#include <orm/DomainObj.h>\n";
        write_include_dependencies(t, str);
        str << "\nnamespace Domain {\n\n";
        if (!mem_weak) {
            str << "class " << class_name << "NotFoundByID: public Yb::ObjectNotFoundByKey\n"
                << "{\n"
                << "public:\n"
                << "\t" << class_name << "NotFoundByID(Yb::LongInt id)\n"
                << "\t\t: Yb::ObjectNotFoundByKey(_T(\"" << class_name << " with ID = \") +\n"
                << "\t\t\tboost::lexical_cast<Yb::String>(id) + _T(\" not found\"))\n"
                << "\t{}\n"
                << "};\n\n";
        }
        write_rel_one2many_fwdecl_classes(t, str);
        str << "class " << class_name << ": public Yb::DomainObject\n"
            << "{\n";
        write_rel_one2many_managed_lists(t, str);
        str << "public:\n"
            << "\tstatic const Yb::String get_table_name() { return _T(\""
            << table_name << "\"); }\n"
            << "\ttypedef Yb::DomainObjectResultSet<"
            << class_name << "> ResultSet;\n"
            << "\t// static method 'find'\n"
            << "\ttypedef std::vector<" << class_name << "> List;\n"
            << "\ttypedef std::auto_ptr<List> ListPtr;\n"
            << "\tstatic ListPtr find(Yb::Session &session,\n"
            << "\t\t\tconst Yb::Filter &filter, const Yb::StrList order_by = _T(\"\"), int max_n = -1);\n";

        str << "\t// constructors\n"
            << "\t" << class_name << "();\n"
            << "\texplicit " << class_name << "(Yb::Session &session);\n";
        str << "\texplicit " << class_name << "(Yb::DataObject::Ptr d)\n"
            << "\t\t: Yb::DomainObject(d)\n"
            << "\t{}\n"
            << "\t" << class_name << "(Yb::Session &session, const Yb::Key &key)\n"
            << "\t\t: Yb::DomainObject(session, key)\n";
        //write_rel_one2many_managed_lists_init(t, str, "key.get_id()"); // FIXME
        str << "\t" << "{}\n";
        try {
            String mega_key = reg_.get_table(t.get_name()).get_unique_pk();
            str << "\t" << class_name << "(Yb::Session &session, Yb::LongInt id)\n"
                << "\t\t: Yb::DomainObject(session, _T(\"" << table_name << "\"), id)\n";
            //write_rel_one2many_managed_lists_init(t, str, "id");
            str << "\t{}\n";
        }
        catch (const AmbiguousPK &) {}

        if (mem_weak) {
            str << "\tconst Yb::Value get_attr_ex(const Yb::String &field) const {\n"
                << "\t\treturn get_attr(field);\n"
                << "\t}\n";
        }
        else {
            str << "\tconst Yb::Value get_attr_ex(const Yb::String &field) const {\n"
                << "\t\ttry {\n"
                << "\t\t\treturn get(field);\n"
                << "\t\t}\n"
                << "\t\tcatch (const Yb::ObjectNotFoundByKey &) {\n"
                << "\t\t\tthrow " << class_name << "NotFoundByID("
                << "get(_T(\"ID\")).as_longint());\n"
                << "\t\t}\n"
                << "\t}\n";
            str << "\tvoid set_attr_ex(const Yb::String &field, const Yb::Value &x) {\n"
                << "\t\ttry {\n"
                << "\t\t\tset(field, x);\n"
                << "\t\t}\n"
                << "\t\tcatch (const Yb::ObjectNotFoundByKey &) {\n"
                << "\t\t\tthrow " << class_name << "NotFoundByID("
                << "get(_T(\"ID\")).as_longint());\n"
                << "\t\t}\n"
                << "\t}\n";
        }
    }

    void write_include_dependencies(const Table &t, ostream &str)
    {
        Columns::const_iterator it = t.begin(), end = t.end();
        for (; it != end; ++it)
            if (it->has_fk())
                str << "#include \"domain/" << 
                    NARROW(t.schema().get_table(it->get_fk_table_name()).get_class_name())
                    << ".h\"\n";
    }

    void write_footer(const Table &t, ostream &str)
    {
        str << "};\n\n"
            << "struct "<< NARROW(t.get_class_name()) << "Registrator\n{\n"
            << "\tstatic void register_domain();\n"
            << "\t" << NARROW(t.get_class_name()) << "Registrator();\n"
            << "};\n\n"
            << "} // namespace Domain\n\n"
            << "// vim:ts=4:sts=4:sw=4:et:\n"
            << "#endif\n";
    }

    void write_cpp_ctor_body(const Table &t, ostream &str, bool save_to_session = false)
    {
        if(mem_weak) {
            str << "{}\n";
        } 
        else {
            str << "{\n";
            Columns::const_iterator it = t.begin(), end = t.end();
            for (; it != end; ++it)
            {
                Yb::Value def_val = it->get_default_value();
                string field_name = NARROW(it->get_name());
                if (!def_val.is_null()) {
                    string default_value = NARROW(it->get_default_value().as_string()); 
                    switch (it->get_type()) {
                        case Value::LONGINT:
                            str << "\tset(_T(\"" << field_name
                                << "\"), Yb::Value((Yb::LongInt)" << default_value << "));\n";
                            break;
                        case Value::DECIMAL:
                            str << "\tset(_T(\"" << field_name
                                << "\"), Yb::Value(Yb::Decimal(" << default_value << ")));\n"; 
                            break;
                        case Value::DATETIME:
                            str << "\tset(_T(\"" << field_name << "\"), Yb::Value(Yb::now()));\n"; 
                            break;
                    }
                }
            }
            if (save_to_session)
                str << "\tsave(session);\n";
            str << "}\n";
        }
    }

    void write_cpp_data(const Table &t, ostream &str)
    {
        string class_name = NARROW(t.get_class_name()), table_name = NARROW(t.get_name());
        str << "#include \"domain/" << class_name << ".h\"\n"
            << "#include <orm/DomainFactorySingleton.h>\n"
            << "#include <orm/MetaDataSingleton.h>\n\n"
            << "namespace Domain {\n\n";

// Constructor for creating new objects

        str << class_name << "::" 
            << class_name << "()\n"
            << "\t: Yb::DomainObject(Yb::theMetaData::instance(), _T(\"" << table_name << "\"))\n";
        write_cpp_ctor_body(t, str);
        str << "\n"
            << class_name << "::" 
            << class_name << "(Yb::Session &session)\n"
            << "\t: Yb::DomainObject(session.schema(), _T(\"" << table_name << "\"))\n";
        write_cpp_ctor_body(t, str, true);

        str << "\n" << class_name << "::ListPtr\n"
            << class_name << "::find(Yb::Session &session,\n"
            << "\t\tconst Yb::Filter &filter, const Yb::StrList order_by, int max_n)\n"
            << "{\n"
            << "\t" << class_name << "::ListPtr lst(new "
            << class_name << "::List());\n"
            << "\tYb::ObjectList rows;\n"
            << "\tsession.load_collection(rows, _T(\"" << table_name << "\"), filter, order_by, max_n);\n"
            << "\tif (rows.size()) {\n"
            << "\t\tYb::ObjectList::iterator it = rows.begin(), end = rows.end();\n"
            << "\t\tfor (; it != end; ++it)\n"
            << "\t\t\tlst->push_back(" << class_name << "(*it));\n"
            << "\t}\n"
            << "\treturn lst;\n"
            << "}\n\n"
            << "void " << class_name << "Registrator::register_domain()\n{\n"
            << "\tYb::theDomainFactory::instance().register_creator(_T(\"" << table_name << "\"),\n"
            << "\t\tYb::CreatorPtr(new Yb::DomainCreator<" << class_name << ">()));\n"
            << "}\n\n"
            << class_name << "Registrator::"
            << class_name << "Registrator()\n{\n"
            << "\tregister_domain();\n}\n\n"
            << "//static " << class_name << "Registrator " << class_name << "_registrator;\n\n"
            << "} // end namespace Domain\n\n"
            << "// vim:ts=4:sts=4:sw=4:et:\n";
    }

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

    void generate_table(const Table &table)
    {
        string table_name = NARROW(table.get_name()), class_name = NARROW(table.get_class_name());
        string file_path = path_ + "/" + class_name + ".h";
        ORM_LOG("Generating file: " << file_path << " for table '" << table_name << "'");
        ostringstream header;
        write_header(table, header);
        write_getters(table, header);
        write_setters(table, header);
        write_is_nulls(table, header);
        //write_foreign_keys_link(table, header);
        write_rel_one2many_link(table, header);
        write_footer(table, header);
        ofstream file(file_path.c_str());
        expand_tabs_to_stream(header.str(), file);

        string cpp_path = path_ + "/" + class_name + ".cpp";
        ORM_LOG("Generating cpp file: " << cpp_path << " for table '" << table_name << "'");
        ostringstream cpp;
        write_cpp_data(table, cpp);
        ofstream cpp_file(cpp_path.c_str());
        expand_tabs_to_stream(cpp.str(), cpp_file);
    }

    void write_is_nulls(const Table &t, ostream &str)
    {
        str << "\t// on null checkers\n";
        Columns::const_iterator it = t.begin(), end = t.end();
        for (; it != end; ++it)
            if (!it->is_pk() && it->is_nullable()) {
                str << "\tbool is_" << NARROW(str_to_lower(it->get_name())) << "_null() const {\n"
                    << "\t\treturn " << "get_attr_ex(_T(\"" << NARROW(it->get_name()) << "\"))" << ".is_null();\n"
                    << "\t}\n";
            }
    }

    void write_getters(const Table &t, ostream &str)
    {
        str << "\t// getters\n";
        Columns::const_iterator it = t.begin(), end = t.end();
        for (; it != end; ++it)
            if (!it->has_fk()) {
                if (it->get_type() == Value::STRING) {
                    str << "\t" << type_by_handle(it->get_type())
                        << " get_" << NARROW(it->get_prop_name()) << "() const {\n"
                        << "\t\tYb::Value v(" << "get_attr_ex(_T(\"" << NARROW(it->get_name()) << "\")));\n"
                        << "\t\treturn v.is_null()? Yb::String(): v.as_string();\n"
                        << "\t}\n";
                } 
                else {
                    int type = it->get_type();
                    str << "\t" << type_by_handle(type)
                        << " get_" << NARROW(it->get_prop_name()) << "() const {\n"
                        << "\t\treturn " << "get_attr_ex(_T(\"" << NARROW(it->get_name()) << "\"))"
                        << "." << value_type_by_handle(type) << ";\n"
                        << "\t}\n";
                }
            }
    }

    void write_setters(const Table &t, ostream &str)
    {
        str << "\t// setters\n";
        Columns::const_iterator it = t.begin(), end = t.end();
        for (; it != end; ++it)
            if (!it->has_fk() && !it->is_ro() && !it->is_pk()) {
                str << "\tvoid set_" << NARROW(it->get_prop_name())
                    << "(" << type_by_handle(it->get_type())
                    << (it->get_type() == Value::STRING ? " &" : " ")
                    << NARROW(str_to_lower(it->get_prop_name())) << "__) {\n"
                    << "\t\tset_attr_ex(_T(\"" << NARROW(it->get_name()) << "\"), Yb::Value("
                    << NARROW(str_to_lower(it->get_prop_name())) << "__));\n"
                    << "\t}\n";
            }
    }

    string type_by_handle(int type)
    {
        switch (type) {
            case Value::LONGINT:
                return "Yb::LongInt";
            case Value::DATETIME:
                return "Yb::DateTime";
            case Value::STRING:
                return "const Yb::String";
            case Value::DECIMAL:
                return "Yb::Decimal";
            default:
                throw runtime_error("Unknown type while parsing metadata");
        }
    }

    string value_type_by_handle(int type)
    {
        switch (type) {
            case Value::LONGINT:
                return "as_longint()";
            case Value::DATETIME:
                return "as_date_time()";
            case Value::STRING:
                return "as_string()";
            case Value::DECIMAL:
                return "as_decimal()";
            default:
                throw runtime_error("Unknown type while parsing metadata");
        }
    }

    void write_rel_one2many_fwdecl_classes(const Table &t, ostream &str)
    {
        Schema::RelMap::const_iterator
            i = t.schema().rels_lower_bound(t.get_class_name()),
            iend = t.schema().rels_upper_bound(t.get_class_name());
        set<String> classes;
        for (; i != iend; ++i) {
            if (i->second.type() == Relation::ONE2MANY &&
                    t.get_class_name() == i->second.side(0) &&
                    i->second.has_attr(0, _T("property")))
            {
                classes.insert(i->second.side(1));
            }
        }
        set<String>::const_iterator j = classes.begin(), jend = classes.end();
        for (; j != jend; ++j)
            str << "class " << NARROW(*j) << ";\n";
        str << "\n";
    }

    void write_rel_one2many_managed_lists(const Table &t, ostream &str)
    {
        Schema::RelMap::const_iterator
            i = t.schema().rels_lower_bound(t.get_class_name()),
            iend = t.schema().rels_upper_bound(t.get_class_name());
        for (; i != iend; ++i) {
            if (i->second.type() == Relation::ONE2MANY &&
                    t.get_class_name() == i->second.side(0) &&
                    i->second.has_attr(0, _T("property")))
            {
                String prop = i->second.attr(0, _T("property")),
                       class_many = i->second.side(1);
                str << "\tYb::ManagedList<" << NARROW(class_many) << "> "
                    << NARROW(prop) << "_;\n";
            }
        }
    }

    void write_rel_one2many_managed_lists_init(const Table &t, ostream &str, const string &id)
    {
        Schema::RelMap::const_iterator
            i = t.schema().rels_lower_bound(t.get_class_name()),
            iend = t.schema().rels_upper_bound(t.get_class_name());
        for (; i != iend; ++i) {
            if (i->second.type() == Relation::ONE2MANY &&
                    t.get_class_name() == i->second.side(0) &&
                    i->second.has_attr(0, _T("property")))
            {
                String prop = i->second.attr(0, _T("property")),
                       class_many = i->second.side(1);
                const Table &fk_table = t.schema().
                    find_table_by_class(i->second.side(1));
                String fk_name;
                Columns::const_iterator j = fk_table.begin(), jend = fk_table.end();
                for (; j != jend; ++j)
                    if (j->has_fk() && j->get_fk_table_name() == t.get_name()) {
                        fk_name = j->get_name();
                        break;
                    }
                str << "\t\t, " << NARROW(prop) << "_(&session, Yb::filter_eq(_T(\""
                    << NARROW(fk_name) << "\"), " << id << "))\n";
            }
        }
    }

    void write_rel_one2many_link(const Table &t, ostream &str)
    {
        str << "\t// relations\n";
        typedef map<String, String> MapString;
        MapString map_fk;
        Columns::const_iterator it = t.begin(), end = t.end();
        for (; it != end; ++it) {
            if (it->has_fk())
                map_fk.insert(MapString::value_type(
                            it->get_fk_table_name(), it->get_name()));
        }
        Schema::RelMap::const_iterator
            i = t.schema().rels_lower_bound(t.get_class_name()),
            iend = t.schema().rels_upper_bound(t.get_class_name());
        for (; i != iend; ++i) {
            if (i->second.type() == Relation::ONE2MANY &&
                    t.get_class_name() == i->second.side(1) &&
                    i->second.has_attr(1, _T("property")))
            {
                String prop = i->second.attr(1, _T("property")),
                       class_one = i->second.side(0),
                       fk_table = i->second.table(0), fk_name = map_fk[fk_table];
                const Table &table_one = t.schema().get_table(fk_table);
                String fk_table_pk_prop = table_one.get_column(
                        table_one.find_synth_pk()).get_prop_name();
                const Column &c = t.get_column(fk_name);
                if (c.is_nullable()) {
                    str << "\tbool has_" << NARROW(prop) << "() const {\n"
                        << "\t\treturn !get_attr_ex(_T(\""
                        << NARROW(fk_name) << "\")).is_null();\n"
                        << "\t}\n";

                    str << "\tvoid reset_" << NARROW(prop) << "() {\n"
                        << "\t\tset_attr_ex(_T(\""
                        << NARROW(fk_name) << "\"), Yb::Value());\n"
                        << "\t}\n";
                }
                str << "\tvoid set_" << NARROW(prop) << "("
                    << NARROW(class_one) << " &" << NARROW(prop) << "__) {\n"
                    << "\t\tcheck_ptr();\n"
                    << "\t\tlink_to_master(" << NARROW(prop) << "__, _T(\"" << NARROW(prop) << "\"));\n"
                    << "\t}\n"
                    << "\t" << NARROW(class_one) <<  " get_"
                    << NARROW(prop) << "() const {\n"
                    << "\t\tcheck_ptr();\n"
                    << "\t\treturn " << NARROW(class_one) << "(Yb::DataObject::get_master(d_, _T(\""
                    << NARROW(prop) << "\")));\n"
                    << "\t}\n";
            }
        }
        i = t.schema().rels_lower_bound(t.get_class_name()),
        iend = t.schema().rels_upper_bound(t.get_class_name());
        for (; i != iend; ++i) {
            if (i->second.type() == Relation::ONE2MANY &&
                    t.get_class_name() == i->second.side(0) &&
                    i->second.has_attr(0, _T("property")))
            {
                String prop = i->second.attr(0, _T("property")),
                       class_many = i->second.side(1);
                str << "\tYb::ManagedList<" << NARROW(class_many) << "> &get_"
                    << NARROW(prop) << "() {\n"
                    << "\t\tif (!" << NARROW(prop) << "_.relation_object())\n"
                    << "\t\t\t" << NARROW(prop) << "_ = Yb::ManagedList<" << NARROW(class_many)
                    << ">(get_slaves_ro(_T(\"" << NARROW(prop) << "\")), d_);\n"
                    << "\t\treturn " << NARROW(prop) << "_;\n"
                    << "\t}\n";
            }
        }
    }

    string path_;
    const Schema &reg_;
    bool mem_weak;
};

} // end of namespace Yb

using namespace Yb;

int main(int argc, char *argv[])
{
    if(argc < 3)
    {
        cerr << "orm_domain_generator config.xml output_path " << endl;
        return 0;
    }
    try {
        string config = argv[1];
        string output_path = argv[2];
        string config_contents;
        load_xml_file(WIDEN(config), config_contents);
        XMLMetaDataConfig cfg(config_contents);
        Schema r;
        cfg.parse(r);
        r.check();
        OrmDomainGenerator generator(output_path, r);
        generator.generate(cfg);
        ORM_LOG("generation successfully finished");
    }
    catch (exception &e) {
        string error = string("Exception: ") + e.what();
        cerr << error << endl;
        ORM_LOG(error);
    }
    catch (...) {
        cerr << "Unknown exception" << endl;
        ORM_LOG("Unknown exception");
    }
    return 0;
}
// vim:ts=4:sts=4:sw=4:et:
