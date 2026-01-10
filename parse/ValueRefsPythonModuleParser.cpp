#include "ValueRefsPythonModuleParser.h"

#include <boost/python/def.hpp>
#include <boost/python/docstring_options.hpp>
#include <boost/python/module.hpp>
#include <boost/python/raw_function.hpp>

#include "ValueRefPythonParser.h"

namespace {
  value_ref_wrapper<int> insert_complex_i1(std::string&& vname, const boost::python::tuple& args, const boost::python::dict& kw, const std::string&& keyint1) {
        std::unique_ptr<ValueRef::ValueRef<int>> int1;
        auto int1_args = boost::python::extract<value_ref_wrapper<int>>(kw[keyint1]);
        if (int1_args.check()) {
            int1 = ValueRef::CloneUnique(int1_args().value_ref);
        } else {
            int1 = std::make_unique<ValueRef::Constant<int>>(boost::python::extract<int>(kw["keyint1"])());
        }

        return value_ref_wrapper<int>(std::make_shared<ValueRef::ComplexVariable<int>>(
            std::move(vname),
            std::move(int1),
            nullptr,
            nullptr,
            nullptr,
            nullptr
        ));
    }

    value_ref_wrapper<int> insert_complex_i1_s1(std::string&& vname, const boost::python::tuple& args, const boost::python::dict& kw, const std::string&& keyint1, const std::string&& keystr1) {
        std::unique_ptr<ValueRef::ValueRef<int>> int1;
        if (kw.has_key(keyint1)) {
            int1 = pyobject_to_vref<int>(kw[keyint1]);
        }

        std::unique_ptr<ValueRef::ValueRef<std::string>> str1;
        if (kw.has_key(keystr1)) {
            str1 = pyobject_to_vref<std::string>(kw[keystr1]);
        }

        return value_ref_wrapper<int>(std::make_shared<ValueRef::ComplexVariable<int>>(
            std::move(vname),
            std::move(int1),
            nullptr,
            nullptr,
            std::move(str1),
            nullptr
        ));
    }
}


BOOST_PYTHON_MODULE(_value_refs) {
    boost::python::docstring_options doc_options(true, true, false);

    const auto f_insert_empire_ships_destroyed_ = [](const boost::python::tuple& args, const boost::python::dict& kw) { return insert_complex_i1("EmpireShipsDestroyed", args, kw, "empire"); };
    boost::python::def("EmpireShipsDestroyed", boost::python::raw_function(f_insert_empire_ships_destroyed_));

    for (std::string_view name : {"ShipDesignsDestroyed",
                                  "ShipDesignsLost",
                                  "ShipDesignsInProduction",
                                  "ShipDesignsOwned",
                                  "ShipDesignsProduced",
                                  "ShipDesignsScrapped"})
    {
            const auto f_insert_ship_designs = [name](const boost::python::tuple& args, const boost::python::dict& kw) { return insert_complex_i1_s1(name.data(),args,kw,"empire","design"); };
            boost::python::def(name.data(), boost::python::raw_function(f_insert_ship_designs));
    }

    const auto f_insert_num_part_classes_in_ship_design_ = [](const boost::python::tuple& args, const boost::python::dict& kw) { return insert_complex_i1("NumPartClassesInShipDesign", args, kw, "design"); };
    boost::python::def("NumPartClassesInShipDesign", boost::python::raw_function(f_insert_num_part_classes_in_ship_design_));
    
    const auto f_insert_part_of_class_in_ship_design_ = [](const boost::python::tuple& args, const boost::python::dict& kw) { return insert_complex_i1_s1("PartOfClassInShipDesign", args, kw, "design", "name"); };
    boost::python::def("PartOfClassInShipDesign", boost::python::raw_function(f_insert_part_of_class_in_ship_design_));

}

