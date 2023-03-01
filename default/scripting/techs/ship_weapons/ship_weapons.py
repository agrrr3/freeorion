from common.misc import SHIP_WEAPON_DAMAGE_FACTOR
from techs.techs import (
    ARBITRARY_BIG_NUMBER_FOR_METER_TOPUP,
    EMPIRE_OWNED_SHIP_WITH_PART,
    SHIP_PART_UPGRADE_RESUPPLY_CHECK,
)


# @1@ part name
def WEAPON_BASE_EFFECTS(part_name: str):
    return [
        EffectsGroup(
            scope=EMPIRE_OWNED_SHIP_WITH_PART(part_name),
            accountinglabel=part_name,
            effects=[
                # XXX these NamedReals exist for the pedia; if the pedia gets better support for looking up part capacity etc these should be removed
                SetMaxCapacity(partname=part_name, value=Value + NamedReal(name=part_name+"_PART_CAPACITY", value=PartCapacity(name=part_name))),
                SetMaxSecondaryStat(partname=part_name, value=Value + NamedReal(name=part_name + "_PART_SECONDARY_STAT", value=PartSecondaryStat(name=part_name))),
            ],
        ),
        # The following is really only needed on the first resupplied turn after an upgrade is researched, since the resupply currently
        # takes place in a portion of the turn before meters are updated, but currently there is no good way to restrict this to
        # only that first resupply (and it is simply mildly inefficient to repeat the topup later).
        EffectsGroup(
            scope=EMPIRE_OWNED_SHIP_WITH_PART(part_name) & Turn(high=LocalCandidate.LastTurnResupplied),
            accountinglabel=part_name,
            effects=SetCapacity(partname=part_name, value=Value + ARBITRARY_BIG_NUMBER_FOR_METER_TOPUP),
        ),
        # this is currently only used for registering a NamedReal per part_name
        EffectsGroup(
            scope=Source & Ship & Planet(), # i.e. impossible condition -> empty scope
            activation=(CurrentTurn == TurnTechResearched(empire=Source.Owner, name=CurrentContent)),
            effects=GenerateSitRepMessage(
                message="SITREP_WEAPONS_RESEARCHED",
                label="SITREP_WEAPONS_RESEARCHED_LABEL",
                icon="icons/sitrep/weapon_upgrade.png",
                parameters={
                    "empire": Source.Owner,
                    "shippart": part_name,
                    "tech": CurrentContent,
                    "dam": NamedReal(name=part_name+"_SCALED_PART_CAPACITY", value=PartCapacity(name=part_name) * SHIP_WEAPON_DAMAGE_FACTOR),
                },
                empire=Target.Owner,
            ),
        ),
    ]


# @1@ part name
# @2@ value added to max capacity
def WEAPON_UPGRADE_CAPACITY_EFFECTS(tech_name: str, part_name: str, value: int):
    return [
        EffectsGroup(
            scope=EMPIRE_OWNED_SHIP_WITH_PART(part_name) & SHIP_PART_UPGRADE_RESUPPLY_CHECK(CurrentContent),
            accountinglabel=part_name,
            effects=SetMaxCapacity(partname=part_name, value=Value + value * SHIP_WEAPON_DAMAGE_FACTOR),
        ),
        # Inform the researching empire that ships in supply will get upgraded before next combat
        EffectsGroup(
            scope=Source,
            activation=(CurrentTurn == TurnTechResearched(empire=Source.Owner, name=CurrentContent)),
            effects=GenerateSitRepMessage(
                message="SITREP_WEAPONS_UPGRADED",
                label="SITREP_WEAPONS_UPGRADED_LABEL",
                icon="icons/sitrep/weapon_upgrade.png",
                parameters={
                    "empire": Source.Owner,
                    "shippart": part_name,
                    "tech": CurrentContent,
                    # str(CurrentContent) -> <ValueRefString object at 0x...>
                    "dam": "test" + str(type(CurrentContent)) + "test",
                    #"dam": NamedReal(name=CurrentContent + "_UPGRADE_DAMAGE", value=value * SHIP_WEAPON_DAMAGE_FACTOR),
                    # this does some string-magic based on the _Y naming convention for ship weapon upgrade tech where Y is the weapon level
                    # TypeError: No registered converter was able to produce a C++ rvalue of type std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > from this Python object of type ValueRefString

                    "sum": NamedReal(name=tech_name + "_UPGRADED_DAMAGE", value=SHIP_WEAPON_DAMAGE_FACTOR * (PartCapacity(name=part_name) + (value * (int(tech_name[-1])-1)))),
                },
                empire=Target.Owner,
            ),
        ),
    ]
