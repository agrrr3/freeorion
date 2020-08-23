#include "ValueRefs.h"

#include <functional>
#include <iomanip>
#include <iterator>
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

void NamedValueRefManager::SetNamedValueRefs(Pending::Pending<NamedValueRefManager::container_type>&& future)
{ m_pending_refs = std::move(future); }


void NamedValueRefManager::CheckPendingRefs() const {
    ErrorLogger() << "Number of registered ValueRefs befor swap: " << m_value_refs.size() << " in " << this;
    if (auto tt = Pending::WaitForPending(m_pending_refs)) {
      std::swap(*tt, const_cast<NamedValueRefManager::container_type&>(m_value_refs));      
	for (auto& k_v : *tt) {
	  ErrorLogger() << "NamedValueRefManager::CheckPendingRefs restore swapped away " << k_v.first;
	  const_cast<NamedValueRefManager::container_type&>(m_value_refs).emplace(k_v.first, std::move(k_v.second));
	}
	(*tt).clear();
    }
    ErrorLogger() << "Number of registered ValueRefs after swap: " << m_value_refs.size() << " in " << this;

}

// default implementation - queries the untyped registry
template <typename T>
ValueRef::ValueRef<T>* const NamedValueRefManager::GetValueRef(const std::string& name) /*const*/ {
    InfoLogger() << "NamedValueRefManager::GetValueRef<T> look for registered valueref for \"" << name << '"';
    CheckPendingRefs();
    auto& it = m_value_refs.find(name);
    if ( it != m_value_refs.end() )
        return dynamic_cast<ValueRef::ValueRef<T>*>(it->second.get());
    ErrorLogger() << "NamedValueRefManager::GetValueRef<T> found no registered valueref for \"" << name << '"';
    return nullptr;
}

// int specialisation - queries the ValueRef<int> registry
template <>
ValueRef::ValueRef<int>* const NamedValueRefManager::GetValueRef(const std::string& name) /*const*/ {
    InfoLogger() << "NamedValueRefManager::GetValueRef<int> look for registered valueref for \"" << name << '"';
    CheckPendingRefs();
    ErrorLogger() << "Number of registered ValueRefs<int>: " << m_value_refs_int.size() << " in " << this;
    const auto& it = m_value_refs_int.find(name);
    if (it != m_value_refs_int.end())
        return it->second.get();
    ErrorLogger() << "NamedValueRefManager::GetValueRef<int> found no registered valueref for \"" << name << '"';
    for(auto& k_v : m_value_refs_int) {
            ErrorLogger() << "NamedValueRefManager::GetValueRef<int> contains registered int valueref for \"" << k_v.first << '"';
    }
    return nullptr;
}

// double specialisation - queries the ValueRef<double> registry
template <>
ValueRef::ValueRef<double>* const NamedValueRefManager::GetValueRef(const std::string& name) /*const*/ {
    InfoLogger() << "NamedValueRefManager::GetValueRef<double> look for registered valueref for \"" << name << '"';
    CheckPendingRefs();
    ErrorLogger() << "Number of registered ValueRefs<double>: " << m_value_refs_double.size() << " in " << this;
    const auto& it = m_value_refs_double.find(name);
    if (it != m_value_refs_double.end())
        return it->second.get();
    ErrorLogger() << "NamedValueRefManager::GetValueRef<double> found no registered valueref for \"" << name << '"';
    for(auto& k_v : m_value_refs_double) {
            ErrorLogger() << "NamedValueRefManager::GetValueRef<double> contains registered double valueref for \"" << k_v.first << '"';
    }
    return nullptr;
}

ValueRef::ValueRefBase* const NamedValueRefManager::GetValueRefBase(const std::string& name) const {
    CheckPendingRefs();
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

NamedValueRefManager::iterator NamedValueRefManager::begin() const {
    CheckPendingRefs();
    // FIXME is this function actually necessary? would need to iterate over all three maps
    return m_value_refs.begin();
}

NamedValueRefManager::iterator NamedValueRefManager::end() const {
    CheckPendingRefs();
    // FIXME see ::begin()
    return m_value_refs.end();
}

NamedValueRefManager& NamedValueRefManager::GetNamedValueRefManager() {
    ErrorLogger() << "NamedValueRefManager::GetNamedValueRefManager starts (check the thread)"; 
    
    static NamedValueRefManager manager; // function local 
    ErrorLogger() << "NamedValueRefManager::GetNamedValueRefManager at " << &manager;
    return manager;
}


unsigned int NamedValueRefManager::GetCheckSum() const {
    CheckPendingRefs();
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


template <typename T>
void NamedValueRefManager::RegisterValueRef(std::string&& valueref_name, std::unique_ptr<ValueRef::ValueRef<T>> vref) {
    InfoLogger() << "Register valueref for " << valueref_name << ": " << vref->Description();
    if (m_value_refs.count(valueref_name)>0) {
        ErrorLogger() << "Skip registration for already registered valueref for " << valueref_name;
        ErrorLogger() << "Number of registered ValueRefs: " << m_value_refs.size();
        return;
    }
    m_value_refs.emplace(valueref_name, std::move(std::unique_ptr<ValueRef::ValueRefBase>(std::move(vref))));
    ErrorLogger() << "Number of registered ValueRefs: " << m_value_refs.size();
}

// specialisation for registering to the ValueRef<int> registry
template <>
void NamedValueRefManager::RegisterValueRef(std::string&& valueref_name, std::unique_ptr<ValueRef::ValueRef<int>> vref) {
    InfoLogger() << "Register int valueref for " << valueref_name << ": " << vref->Description();
    if (m_value_refs_int.count(valueref_name)>0) {
        ErrorLogger() << "Skip registration for already registered int valueref for " << valueref_name;
        ErrorLogger() << "Number of registered int ValueRefs: " << m_value_refs_int.size();
        return;
    }
    m_value_refs_int.emplace(valueref_name, std::move(vref));
    ErrorLogger() << "Number of registered ValueRefs: " << m_value_refs_int.size();
}

// specialisation for registering to the ValueRef<double> registry
template <>
void NamedValueRefManager::RegisterValueRef(std::string&& valueref_name, std::unique_ptr<ValueRef::ValueRef<double>> vref) {
    InfoLogger() << "Register double valueref for " << valueref_name << ": " << vref->Description();
    if (m_value_refs_double.count(valueref_name)>0) {
        ErrorLogger() << "Skip registration for already registered double valueref for " << valueref_name;
        ErrorLogger() << "Number of registered double ValueRefs: " << m_value_refs_double.size();
        return;
    }
    m_value_refs_double.emplace(valueref_name, std::move(vref));
    ErrorLogger() << "Number of registered ValueRefs: " << m_value_refs_double.size();
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
