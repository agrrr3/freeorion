from common.misc import SHIP_WEAPON_DAMAGE_FACTOR
from techs.techs import (
    ARBITRARY_BIG_NUMBER_FOR_METER_TOPUP,
    EMPIRE_OWNED_SHIP_WITH_PART,
    SHIP_PART_UPGRADE_RESUPPLY_CHECK,
)


def STATISTIC_IF_TECH_RESEARCHED(tech_name: str):
    IMPOSSIBLY_LARGE_TURN = 2**15
    valref = TurnTechResearched(empire=Source.Owner, name=tech_name) < IMPOSSIBLY_LARGE_TURN
    return StatisticIf(float, condition = Source & valref)

# @1@ part name
def WEAPON_BASE_EFFECTS(part_name: str):
    # DOES NOT WORK - empire/Source does not exist (before game start) or is not set (while game runs)
    tech_name_prefix = "SHP_" + part_name[3:-2] + "_"
    #best_tech_level_vref = STATISTIC_IF_TECH_RESEARCHED(tech_name_prefix + "4") + STATISTIC_IF_TECH_RESEARCHED(tech_name_prefix + "3") + STATISTIC_IF_TECH_RESEARCHED(tech_name_prefix + "2") + STATISTIC_IF_TECH_RESEARCHED(tech_name_prefix + "1") + TurnTechResearched(empire=Source.Owner, name="SHP_WEAPON_1_2")
    best_tech_level_vref = TurnTechResearched(empire=Source.Owner, name="SHP_WEAPON_1_2")
    #
    # this wont work... best_tech_level is a ValueRef and we do not have support for dynamic names... in principle it should be ok for the lookup part
    # best_tech_damage = NamedRealLookup(name=tech_name_prefix + best_tech_level
    # so i fake the damage
    best_tech_damage_vref = 1*best_tech_level_vref 

    return [
        EffectsGroup(
            scope=EMPIRE_OWNED_SHIP_WITH_PART(part_name),
            accountinglabel=part_name,
            effects=[
                # XXX these NamedReals exist for the pedia; if the pedia gets better support for looking up part capacity etc these should be removed
                SetMaxCapacity(
                    partname=part_name,
                    value=Value + NamedReal(name=part_name + "_PART_CAPACITY", value=PartCapacity(name=part_name)),
                ),
                SetMaxSecondaryStat(
                    partname=part_name,
                    value=Value
                    + NamedReal(name=part_name + "_PART_SECONDARY_STAT", value=PartSecondaryStat(name=part_name)),
                ),
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
        EffectsGroup(
            scope=Source,
            activation=None,
            effects = [
                SetMaxCapacity(
                    partname="fake",
                    value=NamedReal(name=part_name + "_CAPACITY_WITH_CURRENT_TECH", value=best_tech_damage_vref)
                ),
            ],
        ),
    ]


# @1@ part name
# @2@ value added to max capacity
def WEAPON_UPGRADE_CAPACITY_EFFECTS(tech_name: str, part_name: str, value: int, upgraded_damage_override: int = -1):
    # the following recursive lookup works, but is not acceptable because of delays. as long as the parser is sequential, the parallel waiting feature is kind of a bug
    # previous_upgrade_effect = PartCapacity(name=part_name) if (tech_name[-1] == "2") else NamedRealLookup(name = tech_name[0:-1] + "2_UPGRADED_DAMAGE")  # + str(int(tech_name[-1]) - 1))
    upgraded_damage = upgraded_damage_override if upgraded_damage_override != -1 else value * (int(tech_name[-1]) - 1)
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
                    "dam": NamedReal(name=tech_name + "_UPGRADE_DAMAGE", value=value * SHIP_WEAPON_DAMAGE_FACTOR),
                    "sum": NamedReal(
                        name=tech_name + "_UPGRADED_DAMAGE",
                        value=PartCapacity(name=part_name) + (SHIP_WEAPON_DAMAGE_FACTOR * upgraded_damage),
                    ),
                },
                empire=Target.Owner,
            ),
        ),
    ]
