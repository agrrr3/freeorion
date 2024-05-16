from logging import error

from AIDependencies import CombatTarget

_issued_errors = set()


def get_allowed_targets(partname: str) -> int:
    """Return the allowed targets for a given hangar or short range weapon."""
    try:
        return CombatTarget.PART_ALLOWED_TARGETS[partname]
    except KeyError:
        if partname not in _issued_errors:
            error(
                "AI has no targeting information for weapon part %s. Will assume any target allowed."
                "Please update CombatTarget.PART_ALLOWED_TARGETS in AIDependencies.py "
            )
            _issued_errors.add(partname)
        return CombatTarget.ANY

def get_multi_target_split_damage_factor(allowed_targets: int, target_class: int) -> float:
    """
    Return a heuristic factor how much expected damage needs to be scaled down
    for a certain each class of targets in case there multiple valid target classes.
    If the military AI puts the ship to the correct use, the expected damage will be higher
    than a simple division by the number of classes.
    """
    if not (allowed_target & target_class):
        # not possible to hurt intended target - we should not get here
        return 0.0
    target_classes_cnt = 0
    target_classes_cnt += int (allowed_targets & AIDependencies.CombatTarget.FIGHTER)
    #target_classes_cnt += int (allowed_targets & AIDependencies.CombatTarget.PLANET) # damage is not considered in design value
    target_classes_cnt += int (allowed_targets & AIDependencies.CombatTarget.SHIP)
    match target_classes_cnt:
        case 0 | 1:
            # no relevant distractions, single resulting damage type
            return 1.0
        case 2:
            # one type of distraction, two types of resulting damage
            factor = 0.7
        case 3:
            # two types of distraction, three types of resulting damage
            factor = 0.5
        case _:
            # error
            return 0.0

    # factoring in distraction by other targets
    # the expected number of targets is usually fighters > ships > planets, so
    # e.g. planets should not distract much from other targets
    match target_class:
        case AIDependencies.CombatTarget.FIGHTER:
            if (allowed_targets & AIDependencies.CombatTarget.SHIP): factor *= 0.95
        case AIDependencies.CombatTarget.PLANET:
            if (allowed_targets & AIDependencies.CombatTarget.SHIP): factor *= 0.9
            if (allowed_targets & AIDependencies.CombatTarget.FIGHTER): factor *= 0.7
        case AIDependencies.CombatTarget.SHIP:
            if (allowed_targets & AIDependencies.CombatTarget.FIGHTER): factor *= 0.8

    return factor
