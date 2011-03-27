#include <iostream>
#include <map>
#include <fstream>
#include <util/str_utils.hpp>
//#include <util/logger.hpp>
#include "orm/MetaData.h"
#include "orm/Value.h"
#include "orm/XMLMetaDataConfig.h"

//#define ORM_LOG(x) LOG(LogLevel::INFO, "ORM Domain generator: " << x)
#define ORM_LOG(x) std::cout << "ORM Domain generator: " << x << "\n";

using namespace std;
using namespace Yb::StrUtils;

namespace Yb {

class OrmDomainGenerator
{
public:
    OrmDomainGenerator(const std::string &path, const TableMetaDataRegistry &reg)
        : path_(path)
        , reg_(reg)
        , mem_weak(false)
    {}
    void generate(const XMLMetaDataConfig &cfg)
    {
        ORM_LOG("generation started...");
        TableMetaDataRegistry::Map::const_iterator it = reg_.begin(), end = reg_.end();
        for (; it != end; ++it)
            if(cfg.need_generation(it->first))
                generate_table(it->second);
    }
private:
    void init_weak(const TableMetaData &t)
    {
        mem_weak = false;
        try {
            reg_.get_table(t.get_name()).get_synth_pk();
        } catch (const NotSuitableForAutoCreating &) {
            mem_weak = true;
        }
    }
    void write_header(const TableMetaData &t, std::ostream &str)
    {
        init_weak(t);
        string name = "ORM_DOMAIN__" + str_to_upper(t.get_name().substr(2)) + "__INCLUDED";
        str << "#ifndef " << name << "\n"
            << "#define " << name << "\n\n";
        if(!mem_weak)
            str << "#include \"orm/RowData.h\"\n";
        str << "#include \"orm/DataObj.h\"\n"
            << "#include \"orm/AutoXMLizable.h\"\n";
        write_include_dependencies(t, str);
        str << "\nnamespace Domain {\n\n";
        if (!mem_weak) {
            str << "class " << get_file_class_name(t.get_name()) << "NotFoundByID: public Yb::ObjectNotFoundByKey\n"
                << "{\n"
                << "public:\n"
                << "\t" << get_file_class_name(t.get_name()) << "NotFoundByID(Yb::LongInt id)\n"
                << "\t\t: ObjectNotFoundByKey(\"" << get_file_class_name(t.get_name()) << " with ID = \" +\n"
                << "\t\t\tboost::lexical_cast<std::string>(id) + \" not found\")\n"
                << "\t{}\n"
                << "};\n\n";
        }
        str << "class " << get_file_class_name(t.get_name()) << ": public Yb::AutoXMLizable\n"
            << "{\n"
            << "\tYb::Mapper *mapper_;\n";
        write_members(t, str);

        str << "public:\n"
            << "\t// static method 'find'\n"
            << "\ttypedef std::vector<" << get_file_class_name(t.get_name()) << "> List;\n"
            << "\ttypedef std::auto_ptr<List> ListPtr;\n"
            << "\tstatic ListPtr find(Yb::Mapper &mapper,\n"
            << "\t\t\tconst Yb::Filter &filter, const Yb::StrList order_by = \"\", int max_n = -1);\n";

        str << "\t// constructors\n"
            << "\t" << get_file_class_name(t.get_name()) << "()\n"
            << "\t\t: mapper_(NULL)\n"
            << "\t{}\n";
        str << "\t" << get_file_class_name(t.get_name()) << "(Yb::Mapper &mapper, const Yb::RowData &key)\n"
            << "\t\t: mapper_(&mapper)\n"
            << "\t\t, " << get_member_name(t.get_name()) << "(mapper, key)\n"
            << "\t" << "{}\n";
        try {
            std::string mega_key = reg_.get_table(t.get_name()).get_unique_pk();
            str << "\t" << get_file_class_name(t.get_name()) << "(Yb::Mapper &mapper, Yb::LongInt id)\n"
                << "\t\t: mapper_(&mapper)\n"
                << "\t\t, " << get_member_name(t.get_name()) << "(mapper, \"" << t.get_name() << "\", id)\n"
                << "\t{}\n";
        }
        catch (const AmbiguousPK &) {}
        str << "\t" << get_file_class_name(t.get_name()) << "(Yb::Mapper &mapper);\n\n";
        
        str << "\tvoid set(const std::string &field, const Yb::Value &val) {\n"
            << "\t\t" << get_member_name(t.get_name()) << ".set(field, val);\n"
            << "\t}\n";

        if (mem_weak) {
            str << "\tconst Yb::Value get(const std::string &field) const {\n"
                << "\t\treturn " << get_member_name(t.get_name()) << ".get(field);\n"
                << "\t}\n";
        }
        else {
            str << "\tconst Yb::Value get(const std::string &field) const {\n"
                << "\t\ttry {\n"
                << "\t\t\treturn " << get_member_name(t.get_name()) << ".get(field);\n"
                << "\t\t}\n"
                << "\t\tcatch (const Yb::ObjectNotFoundByKey &) {\n"
                << "\t\t\tthrow " << get_file_class_name(t.get_name()) << "NotFoundByID("
                << get_member_name(t.get_name()) << ".get(\"ID\").as_longint());\n"
                << "\t\t}\n"
                << "\t}\n";
        }
        str << "\tconst Yb::XMLNode auto_xmlize(int deep = 0) const\n"
            << "\t{\n"
            << "\t\treturn " << get_member_name(t.get_name()) << ".auto_xmlize(*mapper_, deep);\n"
            << "\t}\n";
    }

    void write_include_dependencies(const TableMetaData &t, std::ostream &str)
    {
        TableMetaData::Map::const_iterator it = t.begin(), end = t.end();
        for (; it != end; ++it)
            if (it->second.has_fk())
                str << "#include \"domain/" << get_file_class_name(it->second.get_fk_table_name()) << ".h\"\n";
    }

    void write_members(const TableMetaData &t, std::ostream &str)
    {
        str << (mem_weak ? "\tYb::WeakObject " : "\tYb::StrongObject ") << get_member_name(t.get_name()) << ";\n";
    }

    void write_footer(std::ostream &str)
    {
        str << "};\n\n"
            << "} // namespace Domain\n\n"
            << "// vim:ts=4:sts=4:sw=4:et:\n"
            << "#endif\n";
    }

    void write_cpp_data(const TableMetaData &t, std::ostream &str)
    {
        str << "#include \"domain/" << get_file_class_name(t.get_name()) << ".h\"\n"
            << "#include \"orm/DomainFactorySingleton.h\"\n\n"
            << "namespace Domain {\n\n";

// Constructor for creating new objects

        str << get_file_class_name(t.get_name()) << "::" 
            << get_file_class_name(t.get_name()) << "(Yb::Mapper &mapper)\n"
            << "\t: mapper_(&mapper)\n"
            << "\t, " << get_member_name(t.get_name()) << "(mapper, \"" << t.get_name() << "\")\n";
        
        if(mem_weak) {
            str << "{}\n";
        } 
        else {
            str << "{\n";
            TableMetaData::Map::const_iterator it = t.begin(), end = t.end();
            for (; it != end; ++it)
            {
                Yb::Value def_val = it->second.get_default_value();
                if (!def_val.is_null()) {
                    switch (it->second.get_type()) {
                        case Value::LONGINT:
                            str << "\t" << get_member_name(t.get_name()) 
                                << ".set(\"" << it->first << "\", Yb::Value(" << def_val.as_string() << "LL));\n"; 
                            break;
                        case Value::DECIMAL:
                            str << "\t" << get_member_name(t.get_name()) 
                                << ".set(\"" << it->first << "\", Yb::Value(Yb::Decimal(" << def_val.as_string() << ")));\n"; 
                            break;
                        case Value::DATETIME:
                            str << "\t" << get_member_name(t.get_name()) 
                                << ".set(\"" << it->first << "\", Yb::Value(Yb::now());\n"; 
                            break;
                    }
                }
            }
            str << "}\n";
        }

        str << "\n" << get_file_class_name(t.get_name()) << "::ListPtr\n"
            << get_file_class_name(t.get_name()) << "::find(Yb::Mapper &mapper,\n"
            << "\t\tconst Yb::Filter &filter, const Yb::StrList order_by, int max_n)\n"
            << "{\n"
            << "\t" << get_file_class_name(t.get_name()) << "::ListPtr lst(new "
            << get_file_class_name(t.get_name()) << "::List());\n"
            << "\tYb::LoadedRows rows = mapper.load_collection(\""
            << t.get_name() << "\", filter, order_by, max_n);\n"
            << "\tif (rows.get()) {\n"
            << "\t\tstd::vector<Yb::RowData * > ::const_iterator it = rows->begin(), end = rows->end();\n"
            << "\t\tfor (; it != end; ++it)\n"
            << "\t\t\tlst->push_back(" << get_file_class_name(t.get_name()) << "(mapper, **it));\n"
            << "\t}\n"
            << "\treturn lst;\n"
            << "}\n\n"
            << "struct "<< get_file_class_name(t.get_name()) << "Registrator\n{\n"
            << "\t" << get_file_class_name(t.get_name()) << "Registrator()\n\t{\n"
            << "\t\tYb::theDomainFactory::instance().register_creator(\"" << t.get_name() << "\",\n"
            << "\t\t\tYb::CreatorPtr(new Yb::DomainCreator<" << get_file_class_name(t.get_name()) << ">()));\n"
            << "\t}\n"
            << "};\n\n"
            << "static " << get_file_class_name(t.get_name()) << "Registrator " << get_member_name(t.get_name()) << "registrator;\n\n"
            << "} // end namespace Domain\n\n"
            << "// vim:ts=4:sts=4:sw=4:et:\n";
    }

    void expand_tabs_to_stream(const std::string &in, std::ostream &out)
    {
        string::const_iterator it = in.begin(), end = in.end();
        for (; it != end; ++it) {
            if (*it == '\t')
                out << "    ";
            else
                out << *it;
        }
    }

    void generate_table(const TableMetaData &table)
    {
        string file_path = path_ + "/" + get_file_class_name(table.get_name()) + ".h";
        ORM_LOG("Generating file: " << file_path << " for table '" << table.get_name() << "'");
        std::ostringstream header;
        write_header(table, header);
        write_getters(table, header);
        write_setters(table, header);
        write_is_nulls(table, header);
        write_foreign_keys_link(table, header);
        write_footer(header);
        ofstream file(file_path.c_str());
        expand_tabs_to_stream(header.str(), file);

        string cpp_path = path_ + "/" + get_file_class_name(table.get_name()) + ".cpp";
        ORM_LOG("Generating cpp file: " << cpp_path << " for table '" << table.get_name() << "'");
        std::ostringstream cpp;
        write_cpp_data(table, cpp);
        ofstream cpp_file(cpp_path.c_str());
        expand_tabs_to_stream(cpp.str(), cpp_file);
    }

    void write_is_nulls(const TableMetaData &t, std::ostream &str)
    {
        string pk_name = t.find_synth_pk();
        str << "\t// on null checkers\n";
        TableMetaData::Map::const_iterator it = t.begin(), end = t.end();
        for (; it != end; ++it)
            if (!it->second.has_fk() && it->first != pk_name) {
                str << "\tbool is_" << str_to_lower(it->second.get_name()) << "_null() const {\n"
                    << "\t\treturn " << "get(\"" << it->second.get_name() << "\")" << ".is_null();\n"
                    << "\t}\n";
            }
    }
    void write_getters(const TableMetaData &t, std::ostream &str)
    {
        string pk_name = t.find_synth_pk();
        str << "\t// getters\n";
        TableMetaData::Map::const_iterator it = t.begin(), end = t.end();
        for (; it != end; ++it)
            if (!it->second.has_fk()) {
                if (it->second.get_type() == Value::STRING) {
                    str << "\t" << type_by_handle(it->second.get_type())
                        << " get_" << str_to_lower(it->second.get_name()) << "() const {\n"
                        << "\t\tYb::Value v(" << "get(\"" << it->second.get_name() << "\"));\n"
                        << "\t\treturn v.is_null()? std::string(): v.as_string();\n"
                        << "\t}\n";
                } 
                else {
                    int type = it->first == pk_name? Value::PKID: it->second.get_type();
                    str << "\t" << type_by_handle(type)
                        << " get_" << str_to_lower(it->second.get_name()) << "() const {\n"
                        << "\t\treturn " << "get(\"" << it->second.get_name() << "\")"
                        << "." << value_type_by_handle(type) << ";\n"
                        << "\t}\n";
                }
            }
    }

    void write_setters(const TableMetaData &t, std::ostream &str)
    {
        str << "\t// setters\n";
        TableMetaData::Map::const_iterator it = t.begin(), end = t.end();
        for (; it != end; ++it)
            if (!it->second.has_fk() && !it->second.is_ro()) {
                str << "\tvoid set_" << str_to_lower(it->second.get_name())
                    << "(" << type_by_handle(it->second.get_type())
                    << (it->second.get_type() == Value::STRING ? " &" : " ")
                    << str_to_lower(it->second.get_name()) << "__) {\n"
                    << "\t\tset(\"" << it->second.get_name() << "\", Yb::Value("
                    << str_to_lower(it->second.get_name()) << "__));\n"
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
                return "const std::string";
            case Value::DECIMAL:
                return "Yb::Decimal";
            case Value::PKID:
                return "const Yb::PKIDValue";
            default:
                throw std::runtime_error("Unknown type while parsing metadata");
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
            case Value::PKID:
                return "as_pkid()";
            default:
                throw std::runtime_error("Unknown type while parsing metadata");
        }
    }

    void write_foreign_keys_link(const TableMetaData &t, std::ostream &str)
    {
        TableMetaData::Map::const_iterator it = t.begin(), end = t.end();
        typedef std::map<std::string, std::string> MapString;
        MapString map_fk;
        for (; it != end; ++it) {
            if(it->second.has_fk()) {
                string name = get_member_name(it->second.get_fk_table_name()).substr(0, it->second.get_fk_table_name().size()-2);
                MapString::iterator found = map_fk.find(name);
                if (found != map_fk.end()) {
                    std::string new_entity_name = get_entity_name_by_field(it->second.get_name());
                    if(new_entity_name == name) {
                        std::string field = found->second;
                        map_fk.erase(found);
                        map_fk.insert(MapString::value_type(get_entity_name_by_field(field), field));
                        map_fk.insert(MapString::value_type(name, it->second.get_name()));
                    }
                    else
                        map_fk.insert(MapString::value_type(new_entity_name, it->second.get_name()));
                }
                else
                    map_fk.insert(MapString::value_type(name, it->second.get_name()));
            }
        }

        MapString reverse_field_fk;
        MapString::const_iterator m_it = map_fk.begin(), m_end = map_fk.end();
        for (; m_it != m_end; ++m_it) {
            reverse_field_fk.insert(MapString::value_type(m_it->second, m_it->first));
        }
        if(reverse_field_fk.size()>0)
            str << "\t// foreign key operations\n";

        it = t.begin(), end = t.end();
        for (; it != end; ++it)
            if (it->second.has_fk())
            {
                string name = reverse_field_fk.find(it->second.get_name())->second;
                string fk_name = str_to_lower(it->second.get_fk_name());
                if(it->second.is_nullable()) {
                    str << "\tbool has_" << name << "() const {\n"
                        << "\t\treturn !get(\"" << it->second.get_name() << "\").is_null();\n"
                        << "\t}\n";

                    str << "\tvoid reset_" << name << "() {\n"
                        << "\t\tset(\""
                        << it->second.get_name() << "\", Yb::Value());\n"
                        << "\t}\n";
                }
                str << "\tvoid set_" << name << "(const "
                    << get_file_class_name(it->second.get_fk_table_name())
                    << " &" << name << ") {\n"
                    << "\t\tset(\"" << it->second.get_name() << "\", Yb::Value("
                    << name << ".get_" << fk_name << "()));\n"
                    << "\t}\n";

                str << "\tconst " << get_file_class_name(it->second.get_fk_table_name())
                    <<  " get_" << name << "() const {\n"
                    << "\t\treturn " << get_file_class_name(it->second.get_fk_table_name())
                    << "(*mapper_, get(\"" << it->second.get_name() << "\").as_longint());\n"
                    << "\t}\n";
            }
    }

    std::string get_entity_name_by_field(const std::string &field)
    {
        return str_to_lower(field.substr(0, field.size()-3));
    }

    std::string get_file_class_name(const std::string &table_name)
    {
        string result;
        result.push_back(to_upper(table_name[2]));
        for (size_t i = 3; i< table_name.size(); ++i) {
            if (table_name[i] == '_' && ((i+1) < table_name.size())) {
                result.push_back(to_upper(table_name[i+1]));
                ++i;
            } else {
                result.push_back(to_lower(table_name[i]));
            }
        }
        return result;
    }

    std::string get_member_name(const std::string &table_name)
    {
        return str_to_lower(table_name.substr(2, table_name.size()-2)) + "_";
    }

    string path_;
    const TableMetaDataRegistry &reg_;
    bool mem_weak;
};

} // end of namespace Yb

using namespace Yb;

int main(int argc, char *argv[])
{
    if(argc < 3)
    {
        std::cerr << "orm_domain_generator config.xml output_path " << std::endl;
        return 0;
    }
    try {
        string config = argv[1];
        string output_path = argv[2];
        string config_contents;
        load_xml_file(config, config_contents);
        XMLMetaDataConfig cfg(config_contents);
        TableMetaDataRegistry r;
        cfg.parse(r);
        r.check();
        OrmDomainGenerator generator(output_path, r);
        generator.generate(cfg);
        ORM_LOG("generation successfully finished");
    }
    catch (std::exception &e) {
        string error = string("Exception: ") + e.what();
        std::cerr << error << endl;
        ORM_LOG(error);
    }
    catch (...) {
        std::cerr << "Unknown exception" << endl;
        ORM_LOG("Unknown exception");
    }
    return 0;
}

