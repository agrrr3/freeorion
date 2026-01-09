#include "Parse.h"

#include "PythonParserImpl.h"
#include "ValueRefPythonParser.h"
#include "ConditionPythonParser.h"
#include "EffectPythonParser.h"
#include "EnumPythonParser.h"
#include "SourcePythonParser.h"

#include "../universe/BuildingType.h"
#include "../universe/Condition.h"
#include "../universe/ValueRef.h"
#include "../util/Directories.h"

#include <boost/python/import.hpp>
#include <boost/python/raw_function.hpp>

namespace {
    DeclareThreadSafeLogger(parsing);

    using start_rule_payload = std::map<std::string, std::unique_ptr<ValueRef::ValueRefBase>, std::less<>>;

    boost::python::object py_insert_named_value_definitions_(start_rule_payload& we_dont_care_and_ignore_, const boost::python::tuple& args,
                                             const boost::python::dict& kw)
    {
        // actually there is no payload, calling NamedInteger, NamedReal... registers the named values
        // we could add the names for the vrefs named via python or similar
        // we should not introduce a different syntax, so moving expressions is frictionless
        return boost::python::object();
    }

    struct py_grammar {
        boost::python::dict globals;

        py_grammar(const PythonParser& parser, start_rule_payload& named_value_definitions_) :
            globals(boost::python::import("builtins").attr("__dict__"))
        {
            RegisterGlobalsEffects(globals);
            RegisterGlobalsConditions(globals);
            RegisterGlobalsValueRefs(globals, parser);
            RegisterGlobalsSources(globals);
            RegisterGlobalsEnums(globals);

            globals["NamedValuesPyFile"] = boost::python::raw_function(
                [&named_value_definitions_](const boost::python::tuple& args, const boost::python::dict& kw)
                { return py_insert_named_value_definitions_(named_value_definitions_, args, kw); });
        }

        boost::python::dict operator()() const { return globals; }
    };
}

namespace parse {
    start_rule_payload named_value_refs_py(const PythonParser& parser, const std::filesystem::path& path, bool& success) {
    start_rule_payload named_value_definitions;

    ScopedTimer timer("NamedValue Python Parsing");

    // Importing dependencies
    parser.LoadValueRefsModule();

    bool file_success = true;
    py_grammar p = py_grammar(parser, named_value_definitions);
    for (const auto& file : ListDir(path, IsFOCPyScript))
        file_success = py_parse::detail::parse_file<py_grammar>(parser, file, p) && file_success;

        TraceLogger(parsing) << "Start parsing FOCS for NamedValue definitions: " << named_value_definitions.size();
        for (auto& [name, def] : named_value_definitions)
            TraceLogger(parsing) << "ValueRef::ValueRefBase " << name << " : " << def->GetCheckSum() << "\n" << def->Dump();
        TraceLogger(parsing) << "End parsing FOCS.py for NamedValue definitions" << named_value_definitions.size();

        success = file_success;
        return named_value_definitions;
    }
}
