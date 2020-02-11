#ifndef _CombatSystem_h_
#define _CombatSystem_h_

#include "../universe/Universe.h"
#include "../util/AppInterface.h"
#include "CombatEvents.h"

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>

/** Contains information about the state of a combat before or after the combat
  * occurs. */
struct CombatInfo {
public:
    /** \name Structors */ //@{
    CombatInfo() = default;
    CombatInfo(int system_id_, int turn_);  ///< ctor taking system id where combat occurs and game turn on which combat occurs
    //@}

    /** \name Accessors */ //@{
    /** Returns System object in this CombatInfo's objects if one exists with
        id system_id. */
    std::shared_ptr<const System> GetSystem() const;
    //@}

    /** \name Mutators */ //@{
    /** Returns System object in this CombatInfo's objects if one exists with
        id system_id. */
    std::shared_ptr<System> GetSystem();

    int                                 turn = INVALID_GAME_TURN;       ///< main game turn
    int                                 system_id = INVALID_OBJECT_ID;  ///< ID of system where combat is occurring (could be INVALID_OBJECT_ID ?)
    std::set<int>                       empire_ids;                     ///< IDs of empires involved in combat
    ObjectMap                           objects;                        ///< actual state of objects relevant to combat
    std::set<int>                       damaged_object_ids;             ///< ids of objects damaged during this battle
    std::set<int>                       destroyed_object_ids;           ///< ids of objects destroyed during this battle
    std::map<int, std::set<int>>        destroyed_object_knowers;       ///< indexed by empire ID, the set of ids of objects the empire knows were destroyed during the combat
    Universe::EmpireObjectVisibilityMap empire_object_visibility;       ///< indexed by empire id and object id, the visibility level the empire has of each object.  may be increased during battle
    std::vector<CombatEventPtr>         combat_events;                  ///< list of combat attack events that occur in combat

    float   GetMonsterDetection() const;

private:
    void    GetEmpireIdsToSerialize(             std::set<int>&                         filtered_empire_ids,                int encoding_empire) const;
    void    GetObjectsToSerialize(               ObjectMap&                             filtered_objects,                   int encoding_empire) const;
    void    GetDamagedObjectsToSerialize(        std::set<int>&                         filtered_damaged_objects,           int encoding_empire) const;
    void    GetDestroyedObjectsToSerialize(      std::set<int>&                         filtered_destroyed_objects,         int encoding_empire) const;
    void    GetDestroyedObjectKnowersToSerialize(std::map<int, std::set<int>>&          filtered_destroyed_object_knowers,  int encoding_empire) const;
    void    GetEmpireObjectVisibilityToSerialize(Universe::EmpireObjectVisibilityMap&   filtered_empire_object_visibility,  int encoding_empire) const;
    void    GetCombatEventsToSerialize(          std::vector<CombatEventPtr>&           filtered_combat_events,             int encoding_empire) const;

    void    InitializeObjectVisibility();

    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive & ar, const unsigned int version) const;
    template<class Archive>
    void load(Archive & ar, const unsigned int version);
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

/** Auto-resolves a battle. */
void AutoResolveCombat(CombatInfo& combat_info);

struct EmpireCombatInfo {
public:
    std::set<int> attacker_ids;

    bool HasAttackers() const;
    bool HasUnlauchedArmedFighters(const CombatInfo& combat_info) const;
};

struct PartAttackInfo {
    PartAttackInfo(ShipPartClass part_class_, const std::string& part_name_,
                   float part_attack_,
                   const ::Condition::Condition* combat_targets_ = nullptr) :
        part_class(part_class_),
        part_type_name(part_name_),
        part_attack(part_attack_),
        combat_targets(combat_targets_)
    {}
    PartAttackInfo(ShipPartClass part_class_, const std::string& part_name_,
                   int fighters_launched_, float fighter_damage_,
                   const std::string& fighter_type_name_,
                   const ::Condition::Condition* combat_targets_ = nullptr) :
        part_class(part_class_),
        part_type_name(part_name_),
        combat_targets(combat_targets_),
        fighters_launched(fighters_launched_),
        fighter_damage(fighter_damage_),
        fighter_type_name(fighter_type_name_)
    {}

    ShipPartClass                       part_class;
    std::string                         part_type_name;
    float                               part_attack = 0.0f;     // for direct damage parts
    const ::Condition::Condition*   combat_targets = nullptr;
    int                                 fighters_launched = 0;  // for fighter bays, input value should be limited by ship available fighters to launch
    float                               fighter_damage = 0.0f;  // for fighter bays, input value should be determined by ship fighter weapon setup
    std::string                         fighter_type_name;
};

struct AutoresolveInfo {
public:
    AutoresolveInfo() = default;
    explicit AutoresolveInfo(CombatInfo& combat_info_);

    std::set<int>                   valid_attacker_object_ids;  // all objects that can attack
    std::map<int, EmpireCombatInfo> empire_infos;               // empire specific information, indexed by empire id
    CombatInfo&                     combat_info = {};
    int                             next_fighter_id = -1000001; // give fighters negative ids so as to avoid clashes with any positive-id of persistent UniverseObjects
    std::set<int>                   destroyed_object_ids;       // objects that have been destroyed so far during this combat
    int                             bout;                       // last bout of actual combat; current bout if currently resolving a bout

    std::vector<int> AddFighters(int number, float damage, int owner_empire_id,
                                 int from_ship_id, const std::string& species,
                                 const std::string& fighter_name,
                                 const Condition::Condition* combat_targets);
    bool CanSomeoneAttackSomething() const;
    void CullTheDead(int bout, BoutEvent::BoutEventPtr& bout_event);
    bool CheckDestruction(const std::shared_ptr<const UniverseObject>& target);
    void CleanEmpires();
    void GetShuffledValidAttackerIDs(std::vector<int>& shuffled);
    float EmpireDetectionStrength(int empire_id);
    InitialStealthEvent::EmpireToObjectVisibilityMap ReportInvisibleObjects() const;

private:
    typedef std::set<int>::const_iterator const_id_iterator;

    // Populate lists of things that can attack. List attackers also by empire.
    void PopulateAttackers();
};

template <class Archive>
void CombatInfo::save(Archive & ar, const unsigned int version) const
{
    std::set<int>                       filtered_empire_ids;
    ObjectMap                           filtered_objects;
    std::set<int>                       filtered_damaged_object_ids;
    std::set<int>                       filtered_destroyed_object_ids;
    std::map<int, std::set<int>>        filtered_destroyed_object_knowers;
    Universe::EmpireObjectVisibilityMap filtered_empire_object_visibility;
    std::vector<CombatEventPtr>         filtered_combat_events;

    GetEmpireIdsToSerialize(                filtered_empire_ids,                GetUniverse().EncodingEmpire());
    GetObjectsToSerialize(                  filtered_objects,                   GetUniverse().EncodingEmpire());
    GetDamagedObjectsToSerialize(           filtered_damaged_object_ids,        GetUniverse().EncodingEmpire());
    GetDestroyedObjectsToSerialize(         filtered_destroyed_object_ids,      GetUniverse().EncodingEmpire());
    GetDestroyedObjectKnowersToSerialize(   filtered_destroyed_object_knowers,  GetUniverse().EncodingEmpire());
    GetEmpireObjectVisibilityToSerialize(   filtered_empire_object_visibility,  GetUniverse().EncodingEmpire());
    GetCombatEventsToSerialize(             filtered_combat_events,             GetUniverse().EncodingEmpire());

    ar  & BOOST_SERIALIZATION_NVP(turn)
        & BOOST_SERIALIZATION_NVP(system_id)
        & BOOST_SERIALIZATION_NVP(filtered_empire_ids)
        & BOOST_SERIALIZATION_NVP(filtered_objects)
        & BOOST_SERIALIZATION_NVP(filtered_damaged_object_ids)
        & BOOST_SERIALIZATION_NVP(filtered_destroyed_object_ids)
        & BOOST_SERIALIZATION_NVP(filtered_destroyed_object_knowers)
        & BOOST_SERIALIZATION_NVP(filtered_empire_object_visibility)
        & BOOST_SERIALIZATION_NVP(filtered_combat_events);
}

template <class Archive>
void CombatInfo::load(Archive & ar, const unsigned int version)
{
    std::set<int>                       filtered_empire_ids;
    ObjectMap                           filtered_objects;
    std::set<int>                       filtered_damaged_object_ids;
    std::set<int>                       filtered_destroyed_object_ids;
    std::map<int, std::set<int>>        filtered_destroyed_object_knowers;
    Universe::EmpireObjectVisibilityMap filtered_empire_object_visibility;
    std::vector<CombatEventPtr>         filtered_combat_events;

    ar  & BOOST_SERIALIZATION_NVP(turn)
        & BOOST_SERIALIZATION_NVP(system_id)
        & BOOST_SERIALIZATION_NVP(filtered_empire_ids)
        & BOOST_SERIALIZATION_NVP(filtered_objects)
        & BOOST_SERIALIZATION_NVP(filtered_damaged_object_ids)
        & BOOST_SERIALIZATION_NVP(filtered_destroyed_object_ids)
        & BOOST_SERIALIZATION_NVP(filtered_destroyed_object_knowers)
        & BOOST_SERIALIZATION_NVP(filtered_empire_object_visibility)
        & BOOST_SERIALIZATION_NVP(filtered_combat_events);

    empire_ids.swap(              filtered_empire_ids);
    objects.swap(                 filtered_objects);
    damaged_object_ids.swap(      filtered_damaged_object_ids);
    destroyed_object_ids.swap(    filtered_destroyed_object_ids);
    destroyed_object_knowers.swap(filtered_destroyed_object_knowers);
    empire_object_visibility.swap(filtered_empire_object_visibility);
    combat_events.swap(           filtered_combat_events);
}

#endif // _CombatSystem_h_
