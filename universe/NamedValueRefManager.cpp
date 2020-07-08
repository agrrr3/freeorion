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
    ErrorLogger() << "NamedValueRefManager::NameValueRefManager constructs singleton " << this;
    s_instance = this;
}

NamedValueRefManager::~NamedValueRefManager() {
    ErrorLogger() << "NamedValueRefManager::~NameValueRefManager destruct " << this ; // FIXME
}
template <typename T>
ValueRef::ValueRef<T>* NamedValueRefManager::GetValueRef(const std::string& name) /*const*/ {
    InfoLogger() << "NamedValueRefManager::GetValueRef<T> look for registered valueref for \"" << name << '"';
    auto* vref = GetAnyValueRef(name);
    if (vref) {
        return dynamic_cast<ValueRef::ValueRef<T>*>(vref);
    }
    ErrorLogger() << "NamedValueRefManager::GetValueRef<T> found no registered valueref for \"" << name << '"';
    return nullptr;
}

template <>
ValueRef::ValueRef<int>* NamedValueRefManager::GetValueRef(const std::string& name) /*const*/ {
    InfoLogger() << "NamedValueRefManager::GetValueRef<int> look for registered valueref for \"" << name << '"';
    auto* vref = GetAnyValueRef(name);
    ErrorLogger() << "Number of registered ValueRefs: " << m_value_refs.size() << " in " << this;
    if (vref) {
        return dynamic_cast<ValueRef::ValueRef<int>*>(vref);
    }
    ErrorLogger() << "NamedValueRefManager::GetValueRef<int> found no registered valueref for \"" << name << '"';
    for(auto& k_v : m_value_refs) {
            ErrorLogger() << "NamedValueRefManager::GetValueRef<int> contains registered valueref for \"" << k_v.first << '"';
    }
    return nullptr;
}

template <>
ValueRef::ValueRef<double>* NamedValueRefManager::GetValueRef(const std::string& name) /*const*/ {
    InfoLogger() << "NamedValueRefManager::GetValueRef<double> look for registered valueref for \"" << name << '"';
    auto* vref = GetAnyValueRef(name);
    ErrorLogger() << "Number of registered ValueRefs: " << m_value_refs.size() << " in " << this;
    if (vref) {
        return dynamic_cast<ValueRef::ValueRef<double>*>(vref);
    }
    ErrorLogger() << "NamedValueRefManager::GetValueRef<double> found no registered valueref for \"" << name << '"';
    for(auto& k_v : m_value_refs) {
            ErrorLogger() << "NamedValueRefManager::GetValueRef<double> contains registered valueref for \"" << k_v.first << '"';
    }
    return nullptr;
}

ValueRef::AnyValueRef* NamedValueRefManager::GetAnyValueRef(const std::string& name) const {
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
    ErrorLogger() << "NamedValueRefManager::GetNamedValueRefManager starts in process " << ::getpid() << " thread: " << std::this_thread::get_id();
    static NamedValueRefManager manager; // function local 
    ErrorLogger() << "NamedValueRefManager::GetNamedValueRefManager at " << &manager << " in process " << ::getpid() << " thread: " << std::this_thread::get_id();
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
std::string NamedValueRefManager::RegisterValueRef(std::string valueref_name, std::unique_ptr<T> vref) {
    //std::string valueref_name = nameref->Eval();
    InfoLogger() << "Register valueref for " << valueref_name << ": " << vref->Description();
    if (m_value_refs.count(valueref_name)>0) {
        ErrorLogger() << "Skip registration for already registered valueref for " << valueref_name;
        ErrorLogger() << "Number of registered ValueRefs: " << m_value_refs.size();
        return valueref_name;
    }
    //    m_value_refs.insert(std::make_pair<std::string&&,std::unique_ptr<ValueRef::AnyValueRef>>(valueref_name, std::move(vref))); 
    // m_value_refs.emplace(valueref_name, std::move(vref)); //FIXME
    //NamedValueRefManager::key_type bla{"bla"};
    //const ValueRef::AnyValueRef* base_ptr = vref.get();
    
    //std::pair<NamedValueRefManager::key_type,NamedValueRefManager::value_type> p = make_pair(valueref_name, std::move(std::unique_ptr<const ValueRef::AnyValueRef>(vref))); // OK works
    //m_value_refs.insert(p); still doesnt work

    //m_value_refs.insert(std::make_pair(valueref_name, std::move(vref)));
    /** works
    std::unique_ptr<const ValueRef::AnyValueRef> tmp_ptr(std::move(vref));
    m_value_refs.emplace(valueref_name, std::move(tmp_ptr));
    */
    // Could not make this work without construction another unique_ptr (although i think it should implicitly convert

    m_value_refs.emplace(valueref_name, std::move(std::unique_ptr<ValueRef::AnyValueRef>(std::move(vref))));
    ErrorLogger() << "Number of registered ValueRefs: " << m_value_refs.size();
    return valueref_name;
}


//void RegisterValueRef(const std::string& name, const ValueRef::AnyValueRef* vref)
//{ ValueRef::registered_valuerefs[name] = vref; }



NamedValueRefManager& GetNamedValueRefManager()
{ return NamedValueRefManager::GetNamedValueRefManager(); }

ValueRef::AnyValueRef* GetAnyValueRef(const std::string& name)
{
    InfoLogger() << "NamedValueRefManager::GetAnyValueRef look for registered valueref for \"" << name << '"';
    if ( GetNamedValueRefManager().GetAnyValueRef(name) )
        return GetNamedValueRefManager().GetAnyValueRef(name);
    InfoLogger() << "NamedValueRefManager::GetAnyValueRef could not find registered valueref for \"" << name << '"';
    return nullptr;
}

template <typename T>
ValueRef::ValueRef<T>* GetValueRef(const std::string& name)
{ return GetNamedValueRefManager().GetValueRef<T>(name); }

template <>
ValueRef::ValueRef<int>* GetValueRef(const std::string& name)
{ return GetNamedValueRefManager().GetValueRef<int>(name); }

template <>
ValueRef::ValueRef<double>* GetValueRef(const std::string& name)
{ return GetNamedValueRefManager().GetValueRef<double>(name); }

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


// void RegisterValueRef(const ValueRef::ValueRef<std::string>* nameref, const ValueRef::AnyValueRef* vref) // this worked once upon a time
 //std::string
void RegisterAnyValueRef(ValueRef::ValueRef<std::string>* nameref, ValueRef::AnyValueRef* vref) {
    //    return GetNamedValueRefManager().RegisterValueRef(nameref, move(std::unique_ptr<const ValueRef::ComplexVariable<int>>(vref)));//FIXME TODO
    //    return "NUFFING";
}

// implementation does not get generated(?)
template <typename T>
std::string RegisterValueRef(ValueRef::ValueRef<std::string>* nameref, T* vref) {
    static_assert(
                  std::is_base_of<ValueRef::AnyValueRef, T>::value, 
                  "T must be a descendant of AnyValueRef"
                  );
    return GetNamedValueRefManager().RegisterValueRef(nameref->Eval(), std::move(std::unique_ptr<T>(vref)));
}

template <typename T>
std::string RegisterValueRefT(std::unique_ptr<ValueRef::ValueRef<std::string>> nameref, std::unique_ptr<ValueRef::ValueRef<double>> vref) {
    return GetNamedValueRefManager().RegisterValueRef<T>(nameref->Eval(), std::move(vref));
}


template <typename T>
std::string RegisterValueRefU(/*std::unique_ptr<ValueRef::ValueRef<std::string>>&& nameref*/std::string name, std::unique_ptr<T>&& vref) {
    return GetNamedValueRefManager().RegisterValueRef<T>(name, move(vref));
}

template <>
std::string RegisterValueRefU(std::string name, std::unique_ptr<ValueRef::ValueRef<int>>&& vref);

template <>
std::string RegisterValueRefU(std::string name, std::unique_ptr<ValueRef::ValueRef<double>>&& vref);


template <>
std::string RegisterValueRef(/*const*/ ValueRef::ValueRef<std::string>* nameref, /*const*/ ValueRef::ComplexVariable<int>* vref) {
    return GetNamedValueRefManager().RegisterValueRef(nameref->Eval(), std::move(std::unique_ptr</*const*/ ValueRef::ComplexVariable<int>>(vref)));
}

template <>
std::string RegisterValueRef(/*const*/ ValueRef::ValueRef<std::string>* nameref, /*const*/ ValueRef::ValueRef<int>* vref) {
    return GetNamedValueRefManager().RegisterValueRef(nameref->Eval(), std::move(std::unique_ptr</*const*/ ValueRef::ValueRef<int>>(vref)));
}

template <>
std::string RegisterValueRef(/*const*/ ValueRef::ValueRef<std::string>* nameref, /*const*/ ValueRef::ValueRef<double>* vref) {
    return GetNamedValueRefManager().RegisterValueRef(nameref->Eval(), std::move(std::unique_ptr</*const*/ ValueRef::ValueRef<double>>(vref)));
}


 // these would need copies:
 /*
template <>
void RegisterValueRef(const ValueRef::ValueRef<std::string>* nameref, const ValueRef::ComplexVariable<int>* vref)
{
    GetNamedValueRefManager().RegisterValueRef(nameref, move(std::make_unique<const ValueRef::ComplexVariable<int>>(*vref)));
}
 */

/*
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
*/

