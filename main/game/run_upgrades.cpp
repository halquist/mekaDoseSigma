#include "run_upgrades.hpp"
#include "mech.hpp"
#include "mech_catalog.hpp"
#include "rng.hpp"
#include <cstring>

namespace Game {

namespace {

bool weaponIs(const WeaponDef* weapon, const WeaponDef& def) {
    return weapon && weapon->id && def.id && strcmp(weapon->id, def.id) == 0;
}

} // namespace

const UpgradeOption& UpgradePicker::catalogEntry(UpgradeId id) {
    static const UpgradeOption kCatalog[] = {
        {UpgradeId::MissileMk2, "MK2", Colors::MISSILE_GREY},
        {UpgradeId::RapidBolt, "BLT", Colors::TRACER_YELLOW},
        {UpgradeId::LegBoost, "LEG", Colors::MECH_BLUE},
        {UpgradeId::MaxHp, "+HP", Colors::HEALTH_FILL},
        {UpgradeId::MoveSpeed, "SPD", Colors::ORANGE},
        {UpgradeId::TurnRate, "TRN", Colors::STEEL_BLUE},
        {UpgradeId::ShieldBoost, "SHD", Colors::SHIELD_HUD},
        {UpgradeId::DamageUp, "DMG", Colors::DAMAGE_RED},
    };
    return kCatalog[static_cast<uint8_t>(id)];
}

bool UpgradePicker::canApply(UpgradeId id, const MechLoadout& loadout) {
    const RunBonuses& bonuses = loadout.bonuses();
    switch (id) {
    case UpgradeId::MissileMk2:
        return weaponIs(loadout.weapon(0), MechCatalog::WEAPON_homing_missile_mk1);
    case UpgradeId::RapidBolt:
        return loadout.weapon(1) == nullptr;
    case UpgradeId::LegBoost:
        if (loadout.part(MechPartSlot::LegUpperL) != nullptr ||
            loadout.part(MechPartSlot::LegUpperR) != nullptr) {
            const MechComponentDef* legL = loadout.part(MechPartSlot::LegUpperL);
            const MechComponentDef* legR = loadout.part(MechPartSlot::LegUpperR);
            const bool hasMk1 =
                (legL && legL->upgradeTo != nullptr) ||
                (legR && legR->upgradeTo != nullptr);
            return hasMk1;
        }
        return bonuses.speed < 30.0f;
    case UpgradeId::MaxHp:
        return bonuses.hp < 75;
    case UpgradeId::MoveSpeed:
        return bonuses.speed < 45.0f;
    case UpgradeId::TurnRate:
        return bonuses.turnRate < 60.0f;
    case UpgradeId::ShieldBoost:
        return bonuses.shieldCapacity < 60;
    case UpgradeId::DamageUp:
        return bonuses.weaponDamage < 8;
    default:
        return false;
    }
}

void UpgradePicker::apply(UpgradeId id, Mech& mech, int& health, int& maxHealth) {
    MechLoadout& loadout = mech.loadout();
    RunBonuses& bonuses = loadout.bonuses();

    switch (id) {
    case UpgradeId::MissileMk2:
        loadout.upgradeWeapon(0);
        mech.rebuildVisual();
        break;
    case UpgradeId::RapidBolt:
        loadout.equipWeapon(1, &MechCatalog::WEAPON_rapid_bolt_mk1);
        mech.rebuildVisual();
        break;
    case UpgradeId::LegBoost:
        if (loadout.part(MechPartSlot::LegUpperL) || loadout.part(MechPartSlot::LegUpperR)) {
            loadout.upgradePart(MechPartSlot::LegUpperL);
            loadout.upgradePart(MechPartSlot::LegUpperR);
            mech.rebuildVisual();
        } else {
            bonuses.speed += 15.0f;
        }
        break;
    case UpgradeId::MaxHp:
        bonuses.hp += 25;
        maxHealth = mech.getMaxHp();
        health += 25;
        if (health > maxHealth) {
            health = maxHealth;
        }
        break;
    case UpgradeId::MoveSpeed:
        bonuses.speed += 15.0f;
        break;
    case UpgradeId::TurnRate:
        bonuses.turnRate += 20.0f;
        break;
    case UpgradeId::ShieldBoost:
        bonuses.shieldCapacity += 20;
        mech.refreshShieldCapacity();
        break;
    case UpgradeId::DamageUp:
        bonuses.weaponDamage += 2;
        break;
    default:
        break;
    }
}

void UpgradePicker::roll(const MechLoadout& loadout) {
    uint8_t pool[static_cast<uint8_t>(UpgradeId::COUNT)];
    uint8_t poolCount = 0;

    for (uint8_t i = 0; i < static_cast<uint8_t>(UpgradeId::COUNT); ++i) {
        const auto id = static_cast<UpgradeId>(i);
        if (canApply(id, loadout)) {
            pool[poolCount++] = i;
        }
    }

    if (poolCount == 0) {
        m_options[0] = catalogEntry(UpgradeId::DamageUp);
        m_options[1] = catalogEntry(UpgradeId::MaxHp);
        return;
    }

    if (poolCount == 1) {
        m_options[0] = catalogEntry(static_cast<UpgradeId>(pool[0]));
        m_options[1] = catalogEntry(UpgradeId::MaxHp);
        return;
    }

    const uint8_t firstIdx = static_cast<uint8_t>(Rng::nextRange(poolCount));
    m_options[0] = catalogEntry(static_cast<UpgradeId>(pool[firstIdx]));

    uint8_t secondIdx = static_cast<uint8_t>(Rng::nextRange(poolCount - 1));
    if (secondIdx >= firstIdx) {
        ++secondIdx;
    }
    m_options[1] = catalogEntry(static_cast<UpgradeId>(pool[secondIdx]));
}

} // namespace Game
