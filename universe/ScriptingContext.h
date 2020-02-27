#ifndef _ScriptingContext_h_
#define _ScriptingContext_h_

#include "Universe.h"

#include <boost/any.hpp>

#include <memory>

class UniverseObject;

/** combat/CombatInfo extends this ScriptingCombatInfo in order
  * to give Conditions and ValueRefs access to combat related data */
struct FO_COMMON_API ScriptingCombatInfo {
    ScriptingCombatInfo() {}
    ScriptingCombatInfo(int bout_, const Universe::EmpireObjectVisibilityMap& vis) :
        bout(bout_),
        empire_object_visibility(vis)
    {}

    int bout = 0; ///< current combat bout, used with CombatBout ValueRef for implementing bout dependent targeting. First combat bout is 1
    Universe::EmpireObjectVisibilityMap empire_object_visibility; ///< indexed by empire id and object id, the visibility level the empire has of each object.  may be increased during battle
};
const ScriptingCombatInfo EMPTY_COMBAT_INFO{};

struct ScriptingContext {
    /** Empty context.  Useful for evaluating ValueRef::Constant that don't
      * depend on their context. */
    ScriptingContext() = default;

    /** Context with only a source object.  Useful for evaluating effectsgroup
      * scope and activation conditions that have no external candidates or
      * effect target to propagate. */
    explicit ScriptingContext(std::shared_ptr<const UniverseObject> source_) :
        source(source_)
    {}

    /** Context with source and visibility map to use when evalulating Visiblity
      * conditions. Useful in combat resolution when the visibility of objects
      * may be different from the overall universe visibility. */
    ScriptingContext(std::shared_ptr<const UniverseObject> source_,
                     const ScriptingCombatInfo& combat_info_) :
        source(source_),
        combat_info(combat_info_)
    {}

    ScriptingContext(std::shared_ptr<const UniverseObject> source_,
                     std::shared_ptr<UniverseObject> target_) :
        source(source_),
        effect_target(target_)
    {}

    ScriptingContext(std::shared_ptr<const UniverseObject> source_,
                     std::shared_ptr<UniverseObject> target_,
                     const boost::any& current_value_) :
        source(source_),
        effect_target(target_),
        current_value(current_value_)
    {}

    /** For evaluating ValueRef in an Effect::Execute function.  Keeps input
      * context, but specifies the current value. */
    ScriptingContext(const ScriptingContext& parent_context,
                     const boost::any& current_value_) :
        source(parent_context.source),
        effect_target(parent_context.effect_target),
        condition_root_candidate(parent_context.condition_root_candidate),
        condition_local_candidate(parent_context.condition_local_candidate),
        current_value(current_value_),
        combat_info(parent_context.combat_info)
    {}

    /** For recursive evaluation of Conditions.  Keeps source and effect_target
      * from input context, but sets local candidate with input object, and if
      * there is no root candidate in the parent context, then the input object
      * becomes the root candidate. */
    ScriptingContext(const ScriptingContext& parent_context,
                     std::shared_ptr<const UniverseObject> condition_local_candidate_) :
        source(                         parent_context.source),
        effect_target(                  parent_context.effect_target),
        condition_root_candidate(       parent_context.condition_root_candidate ?
                                            parent_context.condition_root_candidate :
                                            condition_local_candidate_),    // if parent context doesn't already have a root candidate, the new local candidate is the root
        condition_local_candidate(      condition_local_candidate_),        // new local candidate
        current_value(                  parent_context.current_value),
        combat_info( parent_context.combat_info)
    {}

    ScriptingContext(std::shared_ptr<const UniverseObject> source_,
                     std::shared_ptr<UniverseObject> target_,
                     const boost::any& current_value_,
                     std::shared_ptr<const UniverseObject> condition_root_candidate_,
                     std::shared_ptr<const UniverseObject> condition_local_candidate_) :
        source(source_),
        condition_root_candidate(condition_root_candidate_),
        condition_local_candidate(condition_local_candidate_),
        current_value(current_value_)
    {}

    std::shared_ptr<const UniverseObject>   source;
    std::shared_ptr<UniverseObject>         effect_target;
    std::shared_ptr<const UniverseObject>   condition_root_candidate;
    std::shared_ptr<const UniverseObject>   condition_local_candidate;
    const boost::any                        current_value;
    const ScriptingCombatInfo&              combat_info = EMPTY_COMBAT_INFO;
};

#endif // _ScriptingContext_h_
