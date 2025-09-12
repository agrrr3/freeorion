from focs._effects import (
    BlackHole,
    EffectsGroup,
    InSystem,
    LocalCandidate,
    Min,
    NamedReal,
    Neutron,
    NoOpCondition,
    NoOpValue,
    NoStar,
    OwnedBy,
    Red,
    RemoveSpecial,
    SetSpecialCapacity,
    SetStealth,
    Ship,
    Source,
    SpecialCapacity,
    Star,
    Statistic,
    StatisticCount,
    StatisticElse,
    Target,
    Value,
)
from focs._tech import *
from macros.priorities import (
    EARLY_AFTER_ALL_TARGET_MAX_METERS_PRIORITY,
    AFTER_ALL_TARGET_MAX_METERS_PRIORITY,
    LATE_AFTER_ALL_TARGET_MAX_METERS_PRIORITY,
)

lower_stealth_count_special = "LOWER_STEALTH_COUNT_SPECIAL"
base_stealth_special = "BASE_STEALTH_SPECIAL"

def count_lower_stealth_ships_statistic_valref(base_cond):
    return StatisticCount(
    float,
    condition=base_cond
    & (Value(Target.Stealth) >= Value(LocalCandidate.Stealth)),
)

def target_has_less_stealth_cond(base_cond):
    return base_cond & (Value(Target.Stealth) < Value(LocalCandidate.Stealth))

other_own_ships_insystem = (
    Ship
    & InSystem(id=Target.SystemID)
    & ~IsTarget
    & OwnedBy(empire=Source.Owner)
)
own_ships_on_targetz_starlane = (
    Ship
    & ~InSystem()
    & (
        (
            (LocalCandidate.Fleet.NextSystemID == Target.Fleet.NextSystemID)
            & (LocalCandidate.Fleet.PreviousSystemID == Target.Fleet.PreviousSystemID)
        )
        | (
            (LocalCandidate.Fleet.NextSystemID == Target.Fleet.PreviousSystemID)
            & (LocalCandidate.Fleet.PreviousSystemID == Target.Fleet.NextSystemID)
        )
    )
    & OwnedBy(empire=Source.Owner)
)

# works: there are ships which have more stealth than the candidate
def candidate_has_less_stealth_cond(base_cond):
    return base_cond & (
        SpecialCapacity(name=base_stealth_special, object=Target.ID)
        < SpecialCapacity(name=base_stealth_special, object=LocalCandidate.ID)
    )


def stealth_result(obj, debug=False):
    if debug is True:
        return NoOpValue(float, NoOpValue(float, SpecialCapacity(name=base_stealth_special, object=obj)) - NoOpValue(float, SpecialCapacity(
            name=lower_stealth_count_special, object=obj
        )))
    else:
        return SpecialCapacity(name=base_stealth_special, object=obj) - SpecialCapacity(
            name=lower_stealth_count_special, object=obj
        )


# setting highest base stealth ships, always has ( base_stealth - unstealthiness )

#    return 100 + 

def min_effective_stealth_of_more_stealthy_ships_valref_debug(base_cond):
    return StatisticElse(float, condition=candidate_has_less_stealth_cond(base_cond)) * stealth_result(Target.ID, False) + min_effective_stealth_of_more_stealthy_ships_valref_for_not_max_stealth_ships(base_cond)

def min_effective_stealth_of_more_stealthy_ships_valref_for_not_max_stealth_ships(base_cond):
    return MinOf(
        float,
        Statistic(
            float,
            Min,
            value=stealth_result(LocalCandidate.ID),
            condition=target_has_less_stealth_cond(base_cond),
        ),
        (
            SpecialCapacity(name=base_stealth_special, object=Target.ID)
            - SpecialCapacity(name=lower_stealth_count_special, object=Target.ID)
        ),
    )


def min_effective_stealth_of_more_stealthy_ships_valref(base_cond):
    return (
        StatisticElse(float, condition=candidate_has_less_stealth_cond(base_cond)) * stealth_result(Target.ID)
            + min_effective_stealth_of_more_stealthy_ships_valref_for_not_max_stealth_ships(base_cond)
        )


Tech(
    name="SPY_ROOT_DECEPTION",
    description="SPY_ROOT_DECEPTION_DESC",
    short_description="THEORY_SHORT_DESC",
    category="SPY_CATEGORY",
    researchcost=1,
    researchturns=1,
    tags=["PEDIA_SPY_CATEGORY", "THEORY"],
    effectsgroups=[
        EffectsGroup(
            scope=Ship & OwnedBy(empire=Source.Owner), # & Stealth(high=0.0),
            priority=0,
            accountinglabel="SPY_DECEPTION_RESET",
            effects= [
                SetStealth(value=0.0),
                AddSpecial(name=lower_stealth_count_special, capacity=0),
                AddSpecial(name=base_stealth_special, capacity=0),
            ]
                
        ),
        EffectsGroup(
            scope=Ship & OwnedBy(empire=Source.Owner) & Star(type=[NoStar]),
            accountinglabel="SPY_DECEPTION_EMPTY_SPACE_PENALTY",
            effects=SetStealth(value=Value + NamedReal(name="SPY_DECEPTION_EMPTY_SPACE_PENALTY", value=-10.0)),
        ),
        EffectsGroup(
            scope=Ship & OwnedBy(empire=Source.Owner) & Star(type=[Red]),
            accountinglabel="SPY_DECEPTION_DIM_STAR_PENALTY",
            effects=SetStealth(value=Value + NamedReal(name="SPY_DECEPTION_DIM_STAR_PENALTY", value=-5.0)),
        ),
        EffectsGroup(
            scope=Ship & OwnedBy(empire=Source.Owner) & Star(type=[Neutron]),
            accountinglabel="SPY_DECEPTION_SUBSTELLAR_INTERFERENCE",
            effects=SetStealth(value=Value + NamedReal(name="SPY_DECEPTION_NEUTRON_INTERFERENCE", value=5.0)),
        ),
        EffectsGroup(
            scope=Ship & OwnedBy(empire=Source.Owner) & Star(type=[BlackHole]),
            accountinglabel="SPY_DECEPTION_SUBSTELLAR_INTERFERENCE",
            effects=SetStealth(value=Value + NamedReal(name="SPY_DECEPTION_BLACK_INTERFERENCE", value=10.0)),
        ),
        # clean up specials for debugging
        EffectsGroup(
            scope=Ship & OwnedBy(empire=Source.Owner),
            priority=EARLY_AFTER_ALL_TARGET_MAX_METERS_PRIORITY,
            effects=[
                RemoveSpecial(name=base_stealth_special, object=Target.ID),
                RemoveSpecial(name=lower_stealth_count_special, object=Target.ID),
            ]
        ),
        # temporarily note amount of fleet unstealthiness before capping
        EffectsGroup(
            scope=Ship & InSystem() & OwnedBy(empire=Source.Owner),
            priority=AFTER_ALL_TARGET_MAX_METERS_PRIORITY,
            effects=[
                SetSpecialCapacity(
                    name=lower_stealth_count_special, capacity=count_lower_stealth_ships_statistic_valref(Ship & InSystem() & OwnedBy(empire=Source.Owner))
                ),
                AddSpecial(name=base_stealth_special, capacity=Value(Target.Stealth)),
            ],
        ),
        EffectsGroup(
            scope=Ship & ~InSystem() & OwnedBy(empire=Source.Owner),
            priority=AFTER_ALL_TARGET_MAX_METERS_PRIORITY,
            effects=[
                SetSpecialCapacity(
                    name=lower_stealth_count_special, capacity=count_lower_stealth_ships_statistic_valref(own_ships_on_targetz_starlane)
                ),
                AddSpecial(name=base_stealth_special, capacity=NoOpValue(float,Value(Target.Stealth))),
            ],
        ),
        # apply the lowest resulting stealth of ships of higher/equal stealth
        EffectsGroup(
            scope=Ship & InSystem() & OwnedBy(empire=Source.Owner),
            accountinglabel="FLEET_UNSTEALTHINESS_INSYSTEM",
            priority=LATE_AFTER_ALL_TARGET_MAX_METERS_PRIORITY,
            effects=[
                SetStealth(value=min_effective_stealth_of_more_stealthy_ships_valref(other_own_ships_insystem)),
            ],
        ),
        # Do test a) ships going via different starlanes to/from the same system
        EffectsGroup(
            scope=Ship & ~InSystem() & OwnedBy(empire=Source.Owner),
            accountinglabel="FLEET_UNSTEALTHINESS",
            priority=LATE_AFTER_ALL_TARGET_MAX_METERS_PRIORITY,
            effects=[
                SetMaxStructure(value=100+count_lower_stealth_ships_statistic_valref(own_ships_on_targetz_starlane)),
                SetStructure(value=100+count_lower_stealth_ships_statistic_valref(own_ships_on_targetz_starlane)),
                SetMaxShield(value=StatisticCount(float, condition=own_ships_on_targetz_starlane)),
                SetShield(value=StatisticCount(float, condition=own_ships_on_targetz_starlane)),
                SetStealth(value=100+min_effective_stealth_of_more_stealthy_ships_valref_debug(own_ships_on_targetz_starlane & ~IsTarget)),
            ],
        ),
    ],
)
