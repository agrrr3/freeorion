#include "ValueRefs.h"

#include <functional>
#include <iomanip>
#include <iterator>
#include <unordered_map>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
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

NamedValueRefManager* NamedValueRefManager::s_instance = nullptr;

NamedValueRefManager::NamedValueRefManager() {
    if (s_instance)
        throw std::runtime_error("Attempted to create more than one NamedValueRefManager.");

    // Only update the global pointer on sucessful construction.
    s_instance = this;
}

const ValueRef::AnyValueRef* NamedValueRefManager::GetValueRef(const std::string& name) const {
    //CheckPendingBuildingTypes();
    const auto& it = m_value_refs.find(name);
    return it != m_value_refs.end() ? it->second.get() : nullptr;
}

NamedValueRefManager::iterator NamedValueRefManager::begin() const {
    //CheckPendingBuildingTypes();
    return m_value_refs.begin();
}

NamedValueRefManager::iterator NamedValueRefManager::end() const {
    //CheckPendingBuildingTypes();
    return m_value_refs.end();
}

NamedValueRefManager& NamedValueRefManager::GetNamedValueRefManager() {
    static NamedValueRefManager manager;
    return manager;
}


unsigned int NamedValueRefManager::GetCheckSum() const {
    //CheckPendingBuildingTypes();
    unsigned int retval{0};
    /*    for (auto const& name_type_pair : m_value_refs)
        CheckSums::CheckSumCombine(retval, name_type_pair);  // TODO FIXME
    CheckSums::CheckSumCombine(retval, m_value_refs.size());


    DebugLogger() << "NamedValueRefManager checksum: " << retval;*/
    return retval;
}


template <typename T>
//void NamedValueRefManager::RegisterValueRef(const ValueRef::ValueRef<std::string>* nameref, const std::unique_ptr<ValueRef::ValueRef<T>> vref) {
void NamedValueRefManager::RegisterValueRef(const ValueRef::ValueRef<std::string>* nameref, const std::unique_ptr<T> vref) {
    std::string valueref_name = nameref->Eval();
    InfoLogger() << "Register valueref for " << valueref_name << ": " << vref->Description();
    if (m_value_refs.count(valueref_name)>0)
        ErrorLogger() << "Reregister already registered valueref for " << valueref_name;
    //    m_value_refs.insert(std::make_pair<std::string&&,std::unique_ptr<ValueRef::AnyValueRef>>(valueref_name, std::move(vref))); 
    // m_value_refs.emplace(valueref_name, std::move(vref)); //FIXME
    NamedValueRefManager::key_type bla{"bla"};
    //    std::pair<NamedValueRefManager::key_type,NamedValueRefManager::value_type> p = make_pair(valueref_name, std::move(vref));
    // std::string, std::unique_ptr<ValueRef::AnyValueRef>
        //     m_value_refs.insert(std::make_pair(valueref_name, std::move(vref)));
    m_value_refs.emplace(valueref_name, std::move(vref));
}


//void RegisterValueRef(const std::string& name, const ValueRef::AnyValueRef* vref)
//{ ValueRef::registered_valuerefs[name] = vref; }



NamedValueRefManager& GetNamedValueRefManager()
{ return NamedValueRefManager::GetNamedValueRefManager(); }

const ValueRef::AnyValueRef* GetValueRef(const std::string& name)
{ return GetNamedValueRefManager().GetValueRef(name); }

/* // these are still pure classes
template <>
void RegisterValueRef(const ValueRef::ValueRef<std::string>* nameref, const ValueRef::ValueRef<int>* vref)
{
    GetNamedValueRefManager().RegisterValueRef(nameref, move(std::make_unique(new ValueRef::ValueRef<int>(*vref))));
}

template <>
void RegisterValueRef(const ValueRef::ValueRef<std::string>* nameref, const ValueRef::ValueRef<float>* vref)
{
    GetNamedValueRefManager().RegisterValueRef(nameref, move(std::make_unique(new ValueRef::ValueRef<float>(*vref))));    
}
*/

//ValueRef::ComplexVariable<int>::ComplexVariable(const ValueRef::ComplexVariable<int>&)’ is implicitly deleted because the default definition would be ill-formed
/*
template <>
void RegisterValueRef(const ValueRef::ValueRef<std::string>* nameref, const ValueRef::ComplexVariable<int>* vref)
{
    GetNamedValueRefManager().RegisterValueRef(nameref, move(std::make_unique(new ValueRef::ComplexVariable<int>(*vref))));
}
*/
template <>
void RegisterValueRef(const ValueRef::ValueRef<std::string>* nameref, const ValueRef::ComplexVariable<int>* vref)
{
    GetNamedValueRefManager().RegisterValueRef(nameref, move(std::make_unique<ValueRef::ComplexVariable<int>>(*vref)));
}

template <typename T>
void RegisterValueRef(const ValueRef::ValueRef<std::string>* nameref, const T* vref)
{
    static_assert(
                  std::is_base_of<ValueRef::AnyValueRef, T>::value, 
                  "T must be a descendant of AnyValueRef"
                  );
    // ValueRef::AnyValueRef copy = *vref; // cant copy abstract type
    T* copy = new T(*vref);
    //std::unique_ptr<const ValueRef::AnyValueRef> uptr = std::make_unique<T>(copy);
    std::unique_ptr<ValueRef::AnyValueRef> uptr = std::make_unique<T>(copy);
    GetNamedValueRefManager().RegisterValueRef(nameref, move(uptr));
}
