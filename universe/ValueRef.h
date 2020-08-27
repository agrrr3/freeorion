#ifndef _ValueRef_h_
#define _ValueRef_h_

#include <mutex>
#include "ScriptingContext.h"
#include "../util/Export.h"

namespace ValueRef {

//! The common base class for all ValueRef classes. This class provides
//! some the return-type-independent interface.
struct FO_COMMON_API ValueRefBase {
    ValueRefBase() = default;
    virtual ~ValueRefBase() = default;

    bool RootCandidateInvariant() const  { return m_root_candidate_invariant; }
    bool LocalCandidateInvariant() const { return m_local_candidate_invariant; }
    bool TargetInvariant() const         { return m_target_invariant; }
    bool SourceInvariant() const         { return m_source_invariant; }
    virtual bool SimpleIncrement() const { return false; }
    virtual bool ConstantExpr() const    { return false; }

    virtual std::string Description() const = 0;                    //! Returns a user-readable text description of this ValueRef
    virtual std::string StringResult() const = 0;                   //! Returns a textual representation of the evaluation result of this ValueRef
    virtual std::string Dump(unsigned short ntabs = 0) const = 0;   //! Returns a textual representation that should be parseable to recreate this ValueRef

    virtual void SetTopLevelContent(const std::string& content_name) {}

    virtual unsigned int GetCheckSum() const { return 0; }

protected:
    bool m_root_candidate_invariant = false;
    bool m_local_candidate_invariant = false;
    bool m_target_invariant = false;
    bool m_source_invariant = false;
};

//! The base class for all ValueRef classes returning type T. This class
//! provides the public interface for a ValueRef expression tree.
template <typename T>
struct FO_COMMON_API ValueRef : public ValueRefBase
{
    ValueRef() = default;
    virtual ~ValueRef() = default;

    virtual bool operator==(const ValueRef<T>& rhs) const;

    bool operator!=(const ValueRef<T>& rhs) const
    { return !(*this == rhs); }

    /** Evaluates the expression tree with an empty context.  Useful for
      * evaluating expressions that do not depend on context. */
    T Eval() const
    { return Eval(::ScriptingContext()); }

    /** Evaluates the expression tree and return the results; \a context
      * is used to fill in any instances of the "Value" variable or references
      * to objects such as the source, effect target, or condition candidates
      * that exist in the tree. */
    virtual T Eval(const ScriptingContext& context) const = 0;

    /** Evaluates the expression tree with an empty context and retuns the
      * a string representation of the result value iff the result type is
      * supported (currently std::string, int, float, double).
      * See ValueRefs.cpp for specialisation implementations. */
    std::string StringResult() const;
};

enum StatisticType : int {
    INVALID_STATISTIC_TYPE = -1,
    IF,     // returns T(1) if anything matches the condition, or T(0) otherwise

    COUNT,          // returns the number of objects matching the condition
    UNIQUE_COUNT,   // returns the number of distinct property values of objects matching the condition. eg. if 3 objects have the property value "small", and two have "big", then this value is 2, as there are 2 unique property values.
    HISTO_MAX,      // returns the maximum number of times a unique property value appears in the property values. eg. if 5 objects have values 1, 2, 3, 2, 2, then there are 3x2, 1x1, and 1x3, and the histo max is 3.
    HISTO_MIN,      // returns the maximum number of times a unique property value appears in the property values. eg. if 5 objects have values 1, 2, 3, 2, 2, then there are 3x2, 1x1, and 1x3, and the histo min is 1.
    HISTO_SPREAD,   // returns the (positive) difference between the maximum and minimum numbers of distinct property values. eg. if 5 objects have values, 1, 2, 3, 2, 2, then there are 3x2, 1x1, and 1x3, and the Histo Spread is 3-1 = 2.

    SUM,    // returns the sum of the property values of all objects matching the condition
    MEAN,   // returns the mean of the property values of all objects matching the condition
    RMS,    // returns the sqrt of the mean of the squares of the property values of all objects matching the condition
    MODE,   // returns the most common property value of objects matching the condition.  supported for non-numeric types such as enums.
    MAX,    // returns the maximum value of the property amongst objects matching the condition
    MIN,    // returns the minimum value of the property amongst objects matching the condition
    SPREAD, // returns the (positive) difference between the maximum and minimum values of the property amongst objects matching the condition
    STDEV,  // returns the standard deviation of the property values of all objects matching the condition
    PRODUCT // returns the product of the property values of all objects matching the condition
};

}


//! Holds all FreeOrion named ValueRef%s.  ValueRef%s may be looked up by name.
class FO_COMMON_API NamedValueRefManager {
public:
    //using container_type = std::map<const std::string, const std::unique_ptr<ValueRef::ValueRefBase>>;
    using key_type = const std::string;
    using value_type = std::unique_ptr<ValueRef::ValueRefBase>;
    using int_value_type = std::unique_ptr<ValueRef::ValueRef<int>>;
    using double_value_type = std::unique_ptr<ValueRef::ValueRef<double>>;
    using container_type = std::map<key_type, value_type>;
    using int_container_type = std::map<key_type, int_value_type>;
    using double_container_type = std::map<key_type, double_value_type>;
    using entry_type        = std::pair<key_type, value_type>;
    using int_entry_type    = std::pair<key_type, int_value_type>;
    using double_entry_type = std::pair<key_type, double_value_type>;
    using any_container_type = std::map<key_type, std::reference_wrapper<ValueRef::ValueRefBase>>;
    using any_entry_type     = std::pair<key_type, std::reference_wrapper<ValueRef::ValueRefBase>>;

    using iterator = container_type::const_iterator;

    //! Returns the ValueRef with the name @p name or nullptr if there is nov ValueRef with such a name or of the wrong type
    //! use the free function GetValueRef(...) instead, mainly to save some typing.
    template <typename T>
    auto GetValueRef(const std::string& name) -> ValueRef::ValueRef<T>* const;

    //! Returns the ValueRef with the name @p name; you should use the
    //! free function GetValueRef(...) instead, mainly to save some typing.
    auto GetValueRefBase(const std::string& name) const -> ValueRef::ValueRefBase* const;

    /** returns a map with all named value refs */
    auto GetItems() const -> any_container_type;

    // Singleton
    NamedValueRefManager&  operator=(NamedValueRefManager const&) =delete; // no copy via assignment

    NamedValueRefManager(NamedValueRefManager const&) =delete;            // no copies via construction

    ~NamedValueRefManager();

    //! Returns the instance of this singleton class; you should use the free
    //! function GetNamedValueRefManager() instead
    static NamedValueRefManager& GetNamedValueRefManager();

    //! Returns a number, calculated from the contained data, which should be
    //! different for different contained data, and must be the same for
    //! the same contained data, and must be the same on different platforms
    //! and executions of the program and the function. Useful to verify that
    //! the parsed content is consistent without sending it all between
    //! clients and server.
    auto GetCheckSum() const -> unsigned int;

    //! Register the @p value_ref under the evaluated @p name.
    template <typename T>
    void RegisterValueRef(std::string&& name, std::unique_ptr<ValueRef::ValueRef<T>> vref);

private:
    NamedValueRefManager();

    /** Assigns any m_pending_refs to m_value_refs. */
    void CheckPendingRefs() const;

    /** Future named value refs being parsed by parser.  mutable so that it can
        be assigned to m_species_types when completed.*/
    mutable boost::optional<Pending::Pending<container_type>> m_pending_refs = boost::none;

    //! Map of ValueRef%s identified by a name and mutexes for those to allow asynchronous registration
    double_container_type m_value_refs_double; // int value refs
    std::mutex            m_value_refs_double_mutex;
    int_container_type    m_value_refs_int; // int value refs
    std::mutex            m_value_refs_int_mutex;
    container_type        m_value_refs; // everything else
    std::mutex            m_value_refs_mutex;

    // The s_instance creeation is lazily triggered via as function local.
    // There is exactly one for all translation units.
    static NamedValueRefManager* s_instance;
};

//! Returns the singleton manager for named value refs
FO_COMMON_API auto GetNamedValueRefManager() -> NamedValueRefManager&;

//! Returns the ValueRef object registered with the given
//! @p name.  If no such ValueRef exists, nullptr is returned instead.
FO_COMMON_API auto GetValueRefBase(const std::string& name) -> ValueRef::ValueRefBase* const;

template <typename T>
FO_COMMON_API auto GetValueRef(const std::string& name) -> ValueRef::ValueRef<T>* const;

//! Register and take possesion of the ValueRef object @p vref under the given @p name.
template <typename T>
FO_COMMON_API void RegisterValueRef(std::string name, std::unique_ptr<ValueRef::ValueRef<T>>&& vref);

#endif // _ValueRef_h_
