#include "Parse.h"

#include "ParseImpl.h"
#include "ConditionParser.h"
#include "ValueRefParser.h"

#include "MovableEnvelope.h"
#include "../universe/ValueRefs.h"
#include "../universe/ValueRef.h"
#include "../util/Directories.h"

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>

#include <typeinfo>

#define DEBUG_PARSERS 0

#if DEBUG_PARSERS
namespace std {
    inline ostream& operator<<(ostream& os, const std::map<int, int>&) { return os; }
    inline ostream& operator<<(ostream& os, const std::map<std::string, std::map<int, int>>&) { return os; }
}
#endif

namespace parse {

  template <typename T>
      void insert_named_ref(std::map<std::string, std::unique_ptr<ValueRef::AnyValueRef>>& named_refs,
                         const std::string& name,
			 const parse::detail::MovableEnvelope<ValueRef::ValueRef<T>>& ref_envelope,
                         bool& pass)
    {
      ErrorLogger() << "Registering from named_values.focs.txt : " << name << " ! ValueRef<" << typeid(T).name() << ">";
       if (true) {
	 ErrorLogger() << "Registering from named_values.focs.txt : opening envelope";
	 std::unique_ptr<ValueRef::ValueRef<T>> ref = std::move(ref_envelope.OpenEnvelope(pass));
	 ErrorLogger() << "Registering from named_values.focs.txt : add a AnyValueRef to named_refs map";
         named_refs.emplace(name, std::move(std::unique_ptr<ValueRef::AnyValueRef>(std::move(ref))));
	 ErrorLogger() << "Registering from named_values.focs.txt : go on...";
       } else {
	 ::RegisterValueRef(name, std::move(ref_envelope.OpenEnvelope(pass)));
       }
    }

    BOOST_PHOENIX_ADAPT_FUNCTION(void, insert_named_ref_, insert_named_ref, 4)

    using start_rule_payload = std::map<std::string, std::unique_ptr<ValueRef::AnyValueRef>>;
    using start_rule_signature = void(start_rule_payload&);

    struct grammar : public parse::detail::grammar<start_rule_signature> {
        grammar(const parse::lexer& tok,
                const std::string& filename,
                const parse::text_iterator& first,
                const parse::text_iterator& last) :
            grammar::base_type(start, "named_value_ref_grammar"),
            condition_parser(tok, label),
            string_grammar(tok, label, condition_parser),
            int_rules(tok, label, condition_parser, string_grammar),
            double_rules(tok, label, condition_parser, string_grammar)
        {
            namespace phoenix = boost::phoenix;
            namespace qi = boost::spirit::qi;

            qi::_1_type _1;
            qi::_2_type _2;
            qi::_3_type _3;
            qi::_4_type _4;
            qi::_val_type _val;
            qi::_pass_type _pass;
            qi::omit_type omit_;
            qi::_r1_type _r1;
	    const boost::phoenix::function<detail::open_val_and_register> open_val_and_register_;

            named_ref
               = 
	      // FIXME the second branch is broken / gets an "incompletely parsed"
                    ( omit_[tok.Named_]   >> tok.Integer_
                   > label(tok.Name_) > tok.string
                   > label(tok.Value_) >  qi::as<parse::detail::MovableEnvelope<ValueRef::ValueRef<int>>>()[int_rules.expr]
                    ) [ insert_named_ref_(_r1, _2, _3, _pass) ]
                    |
                    ( omit_[tok.Named_]   >> tok.Real_
                   > label(tok.Name_) > tok.string
                   > label(tok.Value_) >  qi::as<parse::detail::MovableEnvelope<ValueRef::ValueRef<double>>>()[double_rules.expr]
                    ) [ insert_named_ref_(_r1, _2, _3, _pass) ]
                ;


            start
              = +named_ref(_r1)
                ;

            named_ref.name("NamedValueRef");

#if DEBUG_PARSERS
            debug(named_ref);
#endif

	    qi::on_error<qi::fail>(start, parse::report_error(filename, first, last, _1, _2, _3, _4));
        }

        using named_value_ref_rule = parse::detail::rule<
            void (std::map<std::string, std::unique_ptr<ValueRef::AnyValueRef>>&)>;

        using start_rule = parse::detail::rule<start_rule_signature>;

        parse::detail::Labeller                 label;
      //      const parse::detail::condition_parser_grammar condition_parser;
        const parse::conditions_parser_grammar  condition_parser;
      //const parse::detail::value_ref_grammar<std::string>& string_grammar
        const parse::string_parser_grammar      string_grammar;
        const parse::int_arithmetic_rules       int_rules;  // &
        const parse::double_parser_rules        double_rules;  // &
      //parse::detail::common_params_rules      common_rules;
      //parse::capture_result_enum_grammar      capture_result_enum;
      //parse::detail::rule<CaptureResult ()>   capture;
      	named_value_ref_rule                    named_ref;
      //      parse::detail::rule<void (std::map<std::string, std::unique_ptr<ValueRef::AnyValueRef>>&)> named_ref;
        start_rule                              start;
    };
}

namespace parse {
    start_rule_payload named_value_refs(const boost::filesystem::path& path) {
        const lexer lexer;
        start_rule_payload named_value_refs;

        ScopedTimer timer("Named ValueRef Parsing", true);

        for (const auto& file : ListDir(path, IsFOCScript))
            detail::parse_file<grammar, start_rule_payload>(lexer, file, named_value_refs);

       	for (auto& k_v : named_value_refs )
		ErrorLogger() << "named_value_refs : " << k_v.first;

        return named_value_refs;
    }
}
