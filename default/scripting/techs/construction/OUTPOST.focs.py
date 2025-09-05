from focs._effects import (
    Abs,
    Described,
    EffectsGroup,
    EmpireHasAdoptedPolicy,
    GenerateSitRepMessage,
    HasEmpireStockpile,
    HasSpecies,
    IsSource,
    IsTarget,
    MaxOf,
    MinOf,
    NoOpCondition,
    NoOpValue,
    NumPoliciesAdopted,
    OwnedBy,
    Planet,
    Population,
    ResourceInfluence,
    SetConstruction,
    SetEmpireStockpile,
    SetHappiness,
    SetMaxTroops,
    SetTargetConstruction,
    Ship,
    Source,
    StatisticIf,
    Target,
    Value,
)
from focs._tech import *
from macros.priorities import (
    AFTER_ALL_TARGET_MAX_METERS_PRIORITY,
    BEFORE_ANYTHING_ELSE_PRIORITY,
    METER_OVERRIDE_PRIORITY,
    TARGET_AFTER_2ND_SCALING_PRIORITY,
)

#connecty =                                            ResourceSupplyConnected(empire=Source.Owner,condition=Target.SystemID==LocalCandidate.SystemID)
#connectyy = Planet() & OwnedBy(empire=Source.Owner) & ResourceSupplyConnected(empire=Source.Owner,condition=Ship & InSystem(id=Target.SystemID))

# in MoveTo effect
## FIXME? it seems a ship on a starlane is ResourceSupplyConnect to id 133 whatever that object is (not a system)
##  also it seems the effect is done twice and on second time fails because of moving from 133 to 133 
#connecty =                                            ResourceSupplyConnected(empire=Source.Owner,condition=IsTarget)

# take the last system as a reference, that will also work for a ship on a starlane
connectyInSystem =                                                               ResourceSupplyConnected(empire=Source.Owner,condition=IsTarget)
connectyStarlane =                                                               ResourceSupplyConnected(empire=Source.Owner,condition=Object(id=Target.Fleet.PreviousSystemID))
#connecty = ( InSystem() & connectyInSystem ) | ( (~InSystem()) & connectyStarlane )
#connecty = ( InSystem() & connectyInSystem ) | ~( InSystem() | ~connectyStarlane )
#targetSystemAlsoOnStarlane = (IsTarget & InSystem()) | (Object(id=Target.Fleet.PreviousSystemID) & ~InSystem())
targetSystemAlsoOnStarlane = Object(id=Target.Fleet.PreviousSystemID) & NoOpCondition & ~InSystem() & NoOpCondition
#connecty = ResourceSupplyConnected(empire=Source.Owner,condition=targetSystemAlsoOnStarlane)
#connecty = ResourceSupplyConnected(empire=Source.Owner,condition=Object(id=Target.Fleet.PreviousSystemID) & NoOpCondition & ~InSystem() & NoOpCondition)
# ~InSystem does not work to check if the Target is on a starlane. Note that the target is a ship and not a fleet.
connecty = ResourceSupplyConnected(empire=Source.Owner,condition=Object(id=Target.Fleet.PreviousSystemID)|Object(id=Target.ID))
#connectyy = System & Contains(Planet() & OwnedBy(empire=Source.Owner)) & NoOpCondition & ResourceSupplyConnected(empire=Source.Owner,condition=NoOpCondition & Object(id=Target.Fleet.PreviousSystemID)) & NoOpCondition
connectyyInSystem = System & Contains(Planet() & OwnedBy(empire=Source.Owner)) & ResourceSupplyConnected(empire=Source.Owner,condition=IsTarget)
connectyyStarlane = System & Contains(Planet() & OwnedBy(empire=Source.Owner)) & NoOpCondition & ResourceSupplyConnected(empire=Source.Owner,condition=Object(id=Target.Fleet.PreviousSystemID)) & NoOpCondition
#connectyy = ( InSystem() & connectyyInSystem ) | ( (~InSystem()) & connectyyStarlane )
connectyy = System & Contains(Planet() & OwnedBy(empire=Source.Owner)) & connecty
# Object(id=Source.SystemID)
Tech(
    name="CON_OUTPOST",
    description="CON_OUTPOST_DESC",
    short_description="CON_OUTPOST",
    category="CONSTRUCTION_CATEGORY",
    researchcost=1,
    researchturns=1,
    tags=["PEDIA_CONSTRUCTION_CATEGORY"],
    unlock=Item(type=UnlockBuilding, name="BLD_ABANDON_OUTPOST"),
    # Effects for outposts
    effectsgroups=[
        # PROTOTYPE move newly built ships in supply group
        # maybe this needs two cases; mixed fleets and fleets which only contain newly built ships
        # but we dont care and hope the backend takes care of empty fleets
        EffectsGroup(
            scope=Ship
            & OwnedBy(empire=Source.Owner)
            & (LocalCandidate.Age==1),
            priority=BEFORE_ANYTHING_ELSE_PRIORITY,
            effects=[
                GenerateSitRepMessage(  # and tell the owner it happened
                    message="EFFECT send %ship% from %planet%  age: %rawtext%   targets: %rawtext:targets%  maybe: %system:maybe% (%rawtext:maybe%)",
                    label="Custom_1",
                    stringtablelookup=False,
                    icon="icons/tech/categories/spy.png",
                    parameters={
                        "planet":Source.ID,
                        "ship":Target.ID,
                        "rawtext":Target.Age,
                        "targets":StatisticCount(
                            int,
                            condition=connectyy
                        ),
                        "maybe":Statistic(int, Mode, value=NoOpValue(int, LocalCandidate.SystemID), condition=NumberOf(number=1, condition=connectyy)),
                    },
                    empire=Target.Owner,
                ),
                MoveTo(
                    destination=NumberOf(number=1, condition=System
                        & Contains(Planet() & OwnedBy(empire=Source.Owner))
                        & connecty
                    )
                ),

            ]
        ),
        # Outposts only have 50% of troops
        EffectsGroup(
            scope=Planet()
            & OwnedBy(empire=Source.Owner)
            & Population(high=0)
            & ~EmpireHasAdoptedPolicy(empire=Source.Owner, name="PLC_MARINE_RECRUITMENT"),
            stackinggroup="OUTPOST_TROOPS_STACK",
            priority=TARGET_AFTER_2ND_SCALING_PRIORITY,
            effects=SetMaxTroops(value=Value * 0.5),
            accountinglabel="OUTPOST_TROOP_LABEL",
        ),
        # Ensure construction minimum value of one, as this is necessary for being attacked
        EffectsGroup(
            scope=Planet() & OwnedBy(empire=Source.Owner),
            # has to happen after e.g. FORCE_ENERGY_STRC effects which also happens at AFTER_ALL_TARGET_MAX_METERS_PRIORITY
            priority=METER_OVERRIDE_PRIORITY,
            effects=[
                SetTargetConstruction(value=MaxOf(float, Value, 1)),
                SetConstruction(value=MaxOf(float, Value, 1)),
            ],
        ),
        # Influence growth / reduction towards target, since outposts have no species to get this effect from
        EffectsGroup(
            scope=Planet() & OwnedBy(empire=Source.Owner) & (~HasSpecies()),
            priority=AFTER_ALL_TARGET_MAX_METERS_PRIORITY,
            effects=SetHappiness(
                value=Value
                + MinOf(float, Abs(float, Value(Target.TargetHappiness) - Value), 1)
                * (1 - 2 * (StatisticIf(float, condition=IsTarget & (Value > Value(Target.TargetHappiness)))))
            ),
        ),
        # Reset influence to 0 if no policies adopted. Not really relevant to Outposts, but I need somewhere to put this...
        EffectsGroup(
            scope=IsSource,
            activation=HasEmpireStockpile(empire=Source.Owner, resource=ResourceInfluence, high=0)
            & (NumPoliciesAdopted(empire=Source.Owner) == 0),
            priority=METER_OVERRIDE_PRIORITY,
            effects=SetEmpireStockpile(resource=ResourceInfluence, value=0.0),
        ),
    ],
)
