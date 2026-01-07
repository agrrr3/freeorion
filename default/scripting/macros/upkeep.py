from focs._effects import (
    EmpireHasAdoptedPolicy,
    IsSource,
    NumPartClassesInShipDesign,
    Source,
    SpeciesColoniesOwned,
    StatisticIf,
    UsedInDesignID,
)

COLONY_UPKEEP_MULTIPLICATOR = 1 + 0.06 * SpeciesColoniesOwned(empire=Source.Owner)

COLONIZATION_POLICY_MULTIPLIER = (
    1
    - (StatisticIf(float, condition=IsSource & EmpireHasAdoptedPolicy(empire=Source.Owner, name="PLC_COLONIZATION")))
    / 3
    + (StatisticIf(float, condition=IsSource & EmpireHasAdoptedPolicy(empire=Source.Owner, name="PLC_CENTRALIZATION")))
    / 3
)

# TODO needs to add 3 if Colony class type is used
DESIGN_SIMPLICITY_SOURCE_COMPLEXITY_COUNT_VREF = NumPartClassesInShipDesign(design=UsedInDesignID)
