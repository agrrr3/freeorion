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
    target_classes_cnt = 0
    # TODO the expected number of targets is usually fighters > ships > planets, so e.g. planets should not distract much from other targets; so probably the result should depend on a certain target class
    #target_classes_cnt += int (allowed_targets & AIDependencies.CombatTarget.FIGHTER) # point defense is not valued in the ship designer yet
    target_classes_cnt += int (allowed_targets & AIDependencies.CombatTarget.PLANET)
    target_classes_cnt += int (allowed_targets & AIDependencies.CombatTarget.SHIP)
    if target_classes_cnt == 2:
        return 0.7
    if target_classes_cnt == 3:
        return 0.5
    return 1.0
