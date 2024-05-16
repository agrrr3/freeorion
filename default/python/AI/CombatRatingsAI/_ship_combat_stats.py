import freeOrionAIInterface as fo
from logging import warning
from typing import Optional

from AIDependencies import CombatTarget
from aistate_interface import get_aistate
from CombatRatingsAI._targets import get_allowed_targets
from common.fo_typing import AttackCount, AttackDamage, ShipId
from freeorion_tools import get_ship_part
from freeorion_tools.caching import cache_for_current_turn


class ShipCombatStats:
    """Stores all relevant stats of a ship for combat strength evaluation."""

    def __init__(
        self,
        *,
        attacks: Optional[dict[AttackDamage, AttackCount]] = None,
        structure=1.0,
        shields=0.0,
        fighter_capacity=0,
        fighter_launch_rate=0,
        fighter_damage=0,
        flak_shots=0,
        has_interceptors=False,
        damage_vs_planets=0,
        has_bomber=False,
    ):
        self._structure = structure
        self.shields = shields
        self.attacks: dict[AttackDamage, AttackCount] = {} if attacks is None else attacks

        self._fighter_capacity = fighter_capacity
        self._fighter_launch_rate = fighter_launch_rate
        self._fighter_damage = fighter_damage

        self._flak_shots = flak_shots
        self._has_interceptors = has_interceptors

        self._damage_vs_planets = damage_vs_planets
        self._has_bomber = has_bomber

    def __getstate__(self):
        return {
            "_structure": self._structure,
            "shields": self.shields,
            "attacks": self.attacks,
            "_fighter_capacity": self._fighter_capacity,
            "_fighter_launch_rate": self._fighter_launch_rate,
            "_fighter_damage": self._fighter_damage,
            "_flak_shots": self._flak_shots,
            "_has_interceptors": self._has_interceptors,
            "_damage_vs_planets": self._damage_vs_planets,
            "_has_bomber": self._has_bomber,
        }

    def __hash__(self):
        return hash((tuple(self.attacks.items()), self._structure, self.shields))

    def __eq__(self, other):
        return self.__getstate__() == other.__getstate__()

    def __str__(self):
        return str(self.__getstate__())

    def get_rating(self, enemy_stats: "ShipCombatStats" = None) -> float:
        """Calculate a rating against specified enemy.

        If no enemy is specified, will rate against the empire standard enemy

        :param enemy_stats: Enemy stats to be rated against - if None
        :return: rating against specified enemy
        """

        # adjust base stats according to enemy stats
        # The fighter rating calculations are heavily based upon the enemy stats.
        # So, for now, we compare at least against a certain standard enemy.
        enemy_stats = enemy_stats or get_aistate().get_standard_enemy()

        my_hit_points = self._structure
        if enemy_stats:
            shield_factor = self._calculate_shield_factor(enemy_stats.attacks, self.shields)
            flak_factor = self._calculate_flak_factor(enemy_stats, self._flak_shots)
            # XXX not sure how those factors combine if an enemy has both weapon types
            my_hit_points *= max(shield_factor, flak_factor)
            my_total_attack = sum(n * max(dmg - enemy_stats.shields, 0.001) for dmg, n in self.attacks.items())
        else:
            # assumes no shields
            my_total_attack = sum(n * dmg for dmg, n in self.attacks.items())
            # assuming we get hit 3 times before dying
            my_hit_points += self.shields * 3
            # assuming point defens prevents being hit once by fighter
            # XXX check if AI uses damage scaling / use AI Dependency here
            my_hit_points += self.shots * 4

        my_total_attack += self._estimate_fighter_damage()
        # TODO: Consider enemy fighters
        return my_total_attack * my_hit_points

    def _calculate_flak_factor(self, enemy_stats : "ShipCombatStats", my_flak_shots: float) -> float:
        """
        Calculates flak factor based on enemy fighter attacks and our flak_shots
        """
        capacity = enemy_stats._fighter_capacity
        launch_rate = enemy_stats._fighter_launch_rate
        damage = enemy_stats._fighter_damage
        if enemy_stats._has_interceptors or damage <= 0 or capacity < 1 or launch_rate < 1 or my_flak_shots <= 0.0:
            return 1.0
        enemy_fighter_dmg = self._estimate_fighter_damage_vs_flak(capacity, launch_rate, damage, 0.0)
        enemy_fighter_dmg_vs_flak = self._estimate_fighter_damage_vs_flak(capacity, launch_rate, damage, my_flak_shots)
        # TODO sanity check
        flak_factor = enemy_fighter_dmg_vs_flak / enemy_fighter_dmg
        return max(1.0, flak_factor)


    def _calculate_shield_factor(self, e_attacks: dict[AttackDamage, AttackCount], my_shields: float) -> float:
        """
        Calculates shield factor based on enemy attacks and our shields.
        It is possible to have e_attacks with number attacks == 0,
        in that case we consider that there is an issue with the enemy stats and we jut set value to 1.0.
        """
        if not e_attacks:
            return 1.0
        e_total_attack = sum(n * dmg for dmg, n in e_attacks.items())
        if e_total_attack:
            e_net_attack = sum(n * max(dmg - my_shields, 0.001) for dmg, n in e_attacks.items())
            e_net_attack = max(e_net_attack, 0.1 * e_total_attack)
            shield_factor = e_total_attack / e_net_attack
            return max(1.0, shield_factor)
        else:
            return 1.0

    def _estimate_fighter_damage(self):
        """Estimates how much structural damage the fighters of this carrier in an average bout"""
        capacity = self._fighter_capacity
        launch_rate = self._fighter_launch_rate
        damage = self._fighter_damage
        enemy_flak_shots = -1
        return self._estimate_fighter_damage_vs_flak(capacity,launch_rate,damage,enemy_flak_shots)

    def _estimate_fighter_damage_vs_flak(self, capacity, launch_rate, damage, opposing_flak):
        """Estimates how much structural damage the given fighters do in an average bout"""
        if launch_rate <= 0:
            return 0
        full_launch_bouts = capacity // launch_rate
        generic_survival_rate = 0.2  # TODO estimate chance of a fighter not to be shot down in a bout
        flying_fighters = 0
        total_fighter_damage = 0
        # Cut that values down to a single turn (four bouts means max three launch bouts)
        num_bouts = fo.getGameRules().getInt("RULE_NUM_COMBAT_ROUNDS")
        for firing_bout in range(num_bouts - 1):
            if firing_bout < full_launch_bouts:
                flying_fighters = flying_fighters + launch_rate
            elif firing_bout == full_launch_bouts:
                # now handle a bout with lower capacity launch
                flying_fighters = flying_fighters + (
                    capacity % launch_rate
                )
            total_fighter_damage += damage * flying_fighters
            if opposing_flak == -1:
                flying_fighters = flying_fighters * generic_survival_rate
            else:
                # TODO check if there is overkilling of fighters or not
                flying_fighters = max(0, flying_fighters - opposing_flak)
        return total_fighter_damage / num_bouts

    def get_rating_vs_planets(self) -> float:
        """Heuristic to estimate combat strength against planets"""
        damage = self._damage_vs_planets
        if self._has_bomber:
            damage += self._estimate_fighter_damage()
        return damage * (self._structure + self.shields)


@cache_for_current_turn
def get_ship_combat_stats(ship_id: ShipId, max_stats=False) -> ShipCombatStats:  # noqa: C901
    """
    Return combat stats for the ship.

    If max_stats is True use maximum values.
    """
    universe = fo.getUniverse()
    ship = universe.getShip(ship_id)
    if not ship:
        return ShipCombatStats()  # TODO: Add some estimate for stealthed ships

    if max_stats:
        structure = ship.initialMeterValue(fo.meterType.maxStructure)
        shields = ship.initialMeterValue(fo.meterType.maxShield)
    else:
        structure = ship.initialMeterValue(fo.meterType.structure)
        shields = ship.initialMeterValue(fo.meterType.shield)
    attacks: dict[AttackDamage, AttackCount] = {}
    fighter_launch_rate = 0
    fighter_capacity = 0
    fighter_damage = 0
    flak_shots = 0
    has_bomber = False
    has_interceptors = False
    damage_vs_planets = 0
    design = ship.design
    if design and (ship.isArmed or ship.hasFighters):
        meter_choice = fo.meterType.maxCapacity if max_stats else fo.meterType.capacity
        for partname in design.parts:
            if not partname:
                continue
            pc = get_ship_part(partname).partClass
            if pc == fo.shipPartClass.shortRange:
                allowed_targets = get_allowed_targets(partname)
                damage = AttackDamage(ship.currentPartMeterValue(meter_choice, partname))
                shots = int(ship.currentPartMeterValue(fo.meterType.secondaryStat, partname))
                if allowed_targets & CombatTarget.SHIP:
                    attacks[damage] = AttackCount(attacks.get(damage, 0) + shots)
                if allowed_targets & CombatTarget.FIGHTER:
                    flak_shots += 1
                if allowed_targets & CombatTarget.PLANET:
                    damage_vs_planets += damage * shots
            elif pc == fo.shipPartClass.fighterBay:
                fighter_launch_rate += ship.currentPartMeterValue(fo.meterType.capacity, partname)
            elif pc == fo.shipPartClass.fighterHangar:
                allowed_targets = get_allowed_targets(partname)
                # for hangars, capacity meter is already counting contributions from ALL hangars.
                fighter_capacity = ship.currentPartMeterValue(meter_choice, partname)
                part_damage = ship.currentPartMeterValue(fo.meterType.secondaryStat, partname)
                if part_damage != fighter_damage and fighter_damage > 0:
                    # the C++ code fails also in this regard, so FOCS content *should* not allow this.
                    # TODO: Depending on future implementation, might actually need to handle this case.
                    warning("Multiple hangar types present on one ship, estimates expected to be wrong.")
                if allowed_targets & CombatTarget.SHIP:
                    fighter_damage = max(fighter_damage, part_damage)
                if allowed_targets & CombatTarget.PLANET:
                    has_bomber = True
                if allowed_targets & CombatTarget.FIGHTER:
                    has_interceptors = True

    return ShipCombatStats(
        attacks=attacks,
        structure=structure,
        shields=shields,
        fighter_capacity=fighter_capacity,
        fighter_launch_rate=fighter_launch_rate,
        fighter_damage=fighter_damage,
        flak_shots=flak_shots,
        has_interceptors=has_interceptors,
        damage_vs_planets=damage_vs_planets,
        has_bomber=has_bomber,
    )
