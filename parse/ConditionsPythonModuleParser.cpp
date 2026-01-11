#include "ValueRefsPythonModuleParser.h"

#include <boost/python/def.hpp>
#include <boost/python/docstring_options.hpp>
#include <boost/python/import.hpp>
#include <boost/python/module.hpp>
#include <boost/python/raw_function.hpp>

#include "ValueRefPythonParser.h"

namespace py = boost::python;

namespace {
}

BOOST_PYTHON_MODULE(_conditions) {
    boost::python::docstring_options doc_options(true, true, false);

}

