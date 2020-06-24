#ifndef _ValueRef_h_
#define _ValueRef_h_


#include <string>
#include "ScriptingContext.h"
#include "../util/Export.h"


namespace ValueRef {


struct FO_COMMON_API AnyValueRef
{
 virtual std::string Description() const = 0;

 virtual std::string StringResult() const = 0;
};

/** The base class for all ValueRef classes.  This class provides the public
  * interface for a ValueRef expression tree. */
template <typename T>
struct FO_COMMON_API ValueRef : AnyValueRef
{
    virtual ~ValueRef()
    {}

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

    virtual bool RootCandidateInvariant() const
    { return false; }

    virtual bool LocalCandidateInvariant() const
    { return false; }

    virtual bool TargetInvariant() const
    { return false; }

    virtual bool SourceInvariant() const
    { return false; }

    virtual bool SimpleIncrement() const
    { return false; }

    virtual bool ConstantExpr() const
    { return false; }

    virtual std::string Description() const = 0;

    /** Returns a text description of this type of special. */
    virtual std::string Dump(unsigned short ntabs = 0) const = 0;

    virtual void SetTopLevelContent(const std::string& content_name)
    {}

    virtual unsigned int GetCheckSum() const
    { return 0; }

private:
    friend class boost::serialization::access;
    template <typename Archive>
    void serialize(Archive& ar, const unsigned int version);
};

enum StatisticType : int {
    INVALID_STATISTIC_TYPE = -1,
    COUNT,  // returns the number of objects matching the condition
    UNIQUE_COUNT,   // returns the number of distinct property values of objects matching the condition. eg. if 3 objects have the property value "small", and two have "big", then this value is 2, as there are 2 unique property values.
    IF,     // returns T(1) if anything matches the condition, or T(0) otherwise
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
class NamedValueRefManager {
public:
    //using container_type = std::map<const std::string, const std::unique_ptr<ValueRef::AnyValueRef>>;
    using key_type = std::string;
    using value_type = std::unique_ptr<ValueRef::AnyValueRef>;
    using container_type = std::map<key_type, value_type>;
    using iterator = container_type::const_iterator;

    //! Returns the ValueRef with the name @p name; you should use the
    //! free function GetValueRef(...) instead, mainly to save some typing.
    auto GetValueRef(const std::string& name) const -> const ValueRef::AnyValueRef*;

    auto NumNamedValueRefs() const -> std::size_t { return m_value_refs.size(); }

    //! iterator to the first value ref
    FO_COMMON_API auto begin() const -> iterator;

    //! iterator to the last + 1th value ref
    FO_COMMON_API auto end() const -> iterator;

    //! Returns the instance of this singleton class; you should use the free
    //! function GetNamedValueRefManager() instead
    static auto GetNamedValueRefManager() -> NamedValueRefManager&;

    //! Returns a number, calculated from the contained data, which should be
    //! different for different contained data, and must be the same for
    //! the same contained data, and must be the same on different platforms
    //! and executions of the program and the function. Useful to verify that
    //! the parsed content is consistent without sending it all between
    //! clients and server.
    auto GetCheckSum() const -> unsigned int;

    //! Register the @p value_ref under the evaluated @p name.
    template <typename T>
    void RegisterValueRef(const ValueRef::ValueRef<std::string>* name, const std::unique_ptr<T> vref);
    //void RegisterValueRef(const ValueRef::ValueRef<std::string>* nameref, const std::unique_ptr<ValueRef::AnyValueRef> value_ref);

private:
    NamedValueRefManager();

    //! Map of ValueRef%s identified by a name
    //! mutable so that when the parse complete it can be updated.
    mutable container_type m_value_refs;

    static NamedValueRefManager* s_instance;
};

//! Returns the singleton manager for named value refs
FO_COMMON_API auto GetNamedValueRefManager() -> NamedValueRefManager&;

//! Returns the ValueRef object registered with the given
//! @p name.  If no such ValueRef exists, nullptr is returned instead.
FO_COMMON_API auto GetValueRef(const std::string& name) -> const ValueRef::AnyValueRef*;

//! Register the ValueRef object @p vref under the given @p name.
//FO_COMMON_API void RegisterValueRef(const std::string& name, const ValueRef::AnyValueRef* vref);

//! Register a copy of the ValueRef object @p vref under the evaluated @p name.
template <typename T>
FO_COMMON_API void RegisterValueRef(const ValueRef::ValueRef<std::string>* name, const T* vref);

//FO_COMMON_API void RegisterValueRef(const ValueRef::ValueRef<std::string>* name, const std::unique_ptr<ValueRef::AnyValueRef> vref); // TODO

// undefined reference to `void RegisterValueRef<ValueRef::ValueRef<double> >(
//                                  ValueRef::ValueRef<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const*,
//                                                                               ValueRef::ValueRef<double> const*)'
template <>
FO_COMMON_API void RegisterValueRef(const ValueRef::ValueRef<std::string>* name, const ValueRef::ValueRef<double>* vref);

#endif // _ValueRef_h_
