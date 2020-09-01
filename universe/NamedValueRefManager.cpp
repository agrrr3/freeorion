#include "ValueRefs.h"

#include <functional>
#include <iomanip>
#include <iterator>
#include <mutex>
#include <unordered_map>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/log/expressions.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/numeric.hpp>
#include "Enums.h"
#include "Field.h"
#include "Fighter.h"
#include "Fleet.h"
#include "Pathfinder.h"
#include "Planet.h"
#include "ShipDesign.h"
#include "ShipHull.h"
#include "ShipPart.h"
#include "Ship.h"
#include "Species.h"
#include "System.h"
#include "Tech.h"
#include "UniverseObjectVisitors.h"
#include "UniverseObject.h"
#include "Universe.h"
#include "../Empire/EmpireManager.h"
#include "../Empire/Empire.h"
#include "../Empire/Supply.h"
#include "../util/GameRules.h"
#include "../util/Logger.h"
#include "../util/MultiplayerCommon.h"
#include "../util/Random.h"

namespace expr = boost::log::expressions;

NamedValueRefManager* NamedValueRefManager::s_instance = nullptr;

NamedValueRefManager::NamedValueRefManager() {
    if (s_instance)
        throw std::runtime_error("Attempted to create more than one NamedValueRefManager.");

    // Only update the global pointer on sucessful construction.
    ErrorLogger() << "NamedValueRefManager::NameValueRefManager constructs singleton " << this;
    s_instance = this;
}

NamedValueRefManager::~NamedValueRefManager() {
    ErrorLogger() << "NamedValueRefManager::~NameValueRefManager destruct " << this ; // FIXME
}

// default implementation - queries the untyped registry
template <typename T>
ValueRef::ValueRef<T>* const NamedValueRefManager::GetValueRef(const std::string& name) /*const*/ {
    DebugLogger() << "NamedValueRefManager::GetValueRef<T> look for registered valueref for \"" << name << '"';
    auto& it = m_value_refs.find(name);
    if ( it != m_value_refs.end() )
        return dynamic_cast<ValueRef::ValueRef<T>*>(it->second.get());
    ErrorLogger() << "NamedValueRefManager::GetValueRef<T> found no registered valueref for \"" << name << '"';
    return nullptr;
}

// int specialisation - queries the ValueRef<int> registry
template <>
ValueRef::ValueRef<int>* const NamedValueRefManager::GetValueRef(const std::string& name) /*const*/ {
    DebugLogger() << "NamedValueRefManager::GetValueRef<int> look for registered valueref for \"" << name << '"';
    TraceLogger() << "Number of registered ValueRefs<int>: " << m_value_refs_int.size() << " in " << this;
    const auto& it = m_value_refs_int.find(name);
    if (it != m_value_refs_int.end())
        return it->second.get();
    ErrorLogger() << "NamedValueRefManager::GetValueRef<int> found no registered valueref for \"" << name << '"';
    return nullptr;
}

// double specialisation - queries the ValueRef<double> registry
template <>
ValueRef::ValueRef<double>* const NamedValueRefManager::GetValueRef(const std::string& name) /*const*/ {
    DebugLogger() << "NamedValueRefManager::GetValueRef<double> look for registered valueref for \"" << name << '"';
    TraceLogger() << "Number of registered ValueRefs<double>: " << m_value_refs_double.size() << " in " << this;
    const auto& it = m_value_refs_double.find(name);
    if (it != m_value_refs_double.end())
        return it->second.get();
    ErrorLogger() << "NamedValueRefManager::GetValueRef<double> found no registered valueref for \"" << name << '"';
    return nullptr;
}

ValueRef::ValueRefBase* const NamedValueRefManager::GetValueRefBase(const std::string& name) const {
    /* TODO straighten out const shtuff */
    auto* drefp = const_cast<NamedValueRefManager*>(this)->GetValueRef<double>(name);
    //if (auto* drefp = const_cast<NamedValueRefManager*>(this)->GetValueRef<double>(name)) // TODO C++17
    if (drefp)
        return drefp;
    auto* irefp = const_cast<NamedValueRefManager*>(this)->GetValueRef<int>(name);
    //if (auto* irefp = const_cast<NamedValueRefManager*>(this)->GetValueRef<int>(name)) // TODO C++17
    if (irefp)
        return irefp;
    const auto& it = m_value_refs.find(name);
    return it != m_value_refs.end() ? it->second.get() : nullptr;
}

NamedValueRefManager& NamedValueRefManager::GetNamedValueRefManager() {
    ErrorLogger() << "NamedValueRefManager::GetNamedValueRefManager starts (check the thread)"; 
    
    static NamedValueRefManager manager; // function local 
    ErrorLogger() << "NamedValueRefManager::GetNamedValueRefManager at " << &manager;
    return manager;
}


unsigned int NamedValueRefManager::GetCheckSum() const {
    unsigned int retval{0};
    for (auto const& name_type_pair : m_value_refs)
        CheckSums::CheckSumCombine(retval, name_type_pair);
    for (auto const& name_type_pair : m_value_refs_int)
        CheckSums::CheckSumCombine(retval, name_type_pair);
    for (auto const& name_type_pair : m_value_refs_double)
        CheckSums::CheckSumCombine(retval, name_type_pair);
    //CheckSums::CheckSumCombine(retval, m_value_refs.size()); // why also add size?
    
    DebugLogger() << "NamedValueRefManager checksum: " << retval;
    return retval;
}


NamedValueRefManager::any_container_type  NamedValueRefManager::GetItems() const {
    std::function<const any_entry_type(const entry_type&)> base_to_base          = [](const entry_type& kv) { return any_entry_type(kv.first, *(kv.second.get())); };
    std::function<const any_entry_type(const double_entry_type&)> double_to_base = [](const double_entry_type& kv) { return any_entry_type(kv.first, *(kv.second.get())); };
    std::function<const any_entry_type(const int_entry_type&)> int_to_base       = [](const int_entry_type& kv) { return any_entry_type(kv.first, *(kv.second.get())); };

    // should use C++20 ranges (or C++14, C++17 boost Range-v3 library) to avoid copying
    any_container_type aet;
    std::transform(m_value_refs_double.begin(), m_value_refs_double.end(), std::inserter(aet, aet.end()), double_to_base);
    std::transform(m_value_refs_int.begin(), m_value_refs_int.end(), std::inserter(aet, aet.end()), int_to_base);
    std::transform(m_value_refs.begin(), m_value_refs.end(), std::inserter(aet, aet.end()), base_to_base);
    return aet;
}

template <typename R, typename VR>
void NamedValueRefManager::RegisterValueRefImpl(R& container, std::mutex& mutex, const std::string&& label, std::string&& valueref_name, std::unique_ptr<VR>&& vref) {
    InfoLogger() << "Register " << label << " valueref for " << valueref_name << ": " << vref->Description();
    if (container.count(valueref_name)>0) {
        DebugLogger() << "Skip registration for already registered " << label << " valueref for " << valueref_name;
        DebugLogger() << "Number of registered " << label << " ValueRefs: " << m_value_refs.size();
        return;
    }
    const std::lock_guard<std::mutex> lock(mutex);
    if (!(vref->RootCandidateInvariant() && vref->LocalCandidateInvariant() && vref->TargetInvariant() && vref->SourceInvariant()))
            ErrorLogger() << "Currently only invariant value refs can be named. " << valueref_name;
    container.emplace(valueref_name, std::move(vref));
    DebugLogger() << "Number of registered " << label << " ValueRefs: " << container.size();
}

template <typename T>
void NamedValueRefManager::RegisterValueRef(std::string&& valueref_name, std::unique_ptr<ValueRef::ValueRef<T>> vref) {
    this->RegisterValueRefImpl(m_value_refs, m_value_refs_mutex, "generic", std::move(valueref_name), std::move(vref));
}

// specialisation for registering to the ValueRef<int> registry
template <>
void NamedValueRefManager::RegisterValueRef(std::string&& valueref_name, std::unique_ptr<ValueRef::ValueRef<int>> vref) {
    this->RegisterValueRefImpl(m_value_refs_int, m_value_refs_int_mutex, "int", std::move(valueref_name), std::move(vref));
}

// specialisation for registering to the ValueRef<double> registry
template <>
void NamedValueRefManager::RegisterValueRef(std::string&& valueref_name, std::unique_ptr<ValueRef::ValueRef<double>> vref) {
    this->RegisterValueRefImpl(m_value_refs_double, m_value_refs_double_mutex, "double", std::move(valueref_name), std::move(vref));
}

NamedValueRefManager& GetNamedValueRefManager()
{ return NamedValueRefManager::GetNamedValueRefManager(); }

ValueRef::ValueRefBase* const GetValueRefBase(const std::string& name)
{
    InfoLogger() << "NamedValueRefManager::GetValueRefBase look for registered valueref for \"" << name << '"';
    if ( GetNamedValueRefManager().GetValueRefBase(name) )
        return GetNamedValueRefManager().GetValueRefBase(name);
    InfoLogger() << "NamedValueRefManager::GetValueRefBase could not find registered valueref for \"" << name << '"';
    return nullptr;
}

template <typename T>
ValueRef::ValueRef<T>* const GetValueRef(const std::string& name)
{ return GetNamedValueRefManager().GetValueRef<T>(name); }

template <typename T>
void RegisterValueRef(std::string name, std::unique_ptr<ValueRef::ValueRef<T>>&& vref)
{ return GetNamedValueRefManager().RegisterValueRef<T>(std::move(name), std::move(vref)); }

// trigger instantiations
template ValueRef::ValueRef<int>*    const GetValueRef(const std::string& name);
template ValueRef::ValueRef<double>* const GetValueRef(const std::string& name);
template void RegisterValueRef(std::string name, std::unique_ptr<ValueRef::ValueRef<int>>&& vref);
template void RegisterValueRef(std::string name, std::unique_ptr<ValueRef::ValueRef<double>>&& vref);
template void RegisterValueRef(std::string name, std::unique_ptr<ValueRef::ValueRef<PlanetType>>&& vref);
template void RegisterValueRef(std::string name, std::unique_ptr<ValueRef::ValueRef<PlanetEnvironment>>&& vref);
