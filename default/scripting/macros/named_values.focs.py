from focs._effects import (
    GameRule,
    NamedInteger,
    NamedReal,
    NumPartClassesInShipDesign,
    UsedInDesignID,
)

# Proposed naming convention: <EFFECT_NAME> _ <METER_NAME> _ FLAT/PERPOP/<others>
# Examples:
# - PLC_CONFEDERATION_TARGET_HAPPINESS_FLAT (flat bonus/malus to TargetHappiness from policy PLC_CONFEDERATION)
# - GARRISON_2_TROOPREGEN_FLAT (flat bonus/malus to troop regen from tech Garrison_2
# - GRO_CYBORGS_MAX_TROOPS_PERPOP (bonus/malus per population to MaxTroops from tech GRO_CYBORGS)

# from macros.base_prod import BUILDING_COST_MULTIPLIER

NamedInteger(name="XENOPHOBIC_MAX_JUMPS", value=4)

NamedReal(name="PROTECION_FOCUS_STABILITY_BONUS", value=GameRule(type=float, name="RULE_PROTECTION_FOCUS_STABILITY"))

NamedReal(name="FU_RAMSCOOP_REFUEL", value=0.4)
NamedReal(name="BLD_NEST_RESERVE_WILD_SPAWN_FACTOR", value=0.3)

# For reduced/increased ship cost
# value is used in upkeep.focs.txt, can be migrated to upkeep.focs.py as soon as that gets used when parsing ships
# Note: colony part class increases complexity by 4; every other classe by 1
# TODO needs to add 3 if Colony class type is used
#           + (3 * Statistic If condition = And [ Source  DesignHasPartClass low = 1 class = Colony ])
NamedInteger(name="DESIGN_SIMPLICITY_SOURCE_COMPLEXITY_PC_COLONY", value=1)
NamedInteger(
    name="DESIGN_SIMPLICITY_SOURCE_COMPLEXITY_COUNT_VREF", value=NumPartClassesInShipDesign(design=UsedInDesignID)
)
