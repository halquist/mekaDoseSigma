#include "run_upgrades.hpp"
#include "mech.hpp"
#include "rng.hpp"
#include <cstdio>

namespace Game {

int maxHpForArmorTier(uint8_t tier) {
    if (tier > kMaxUpgradeTier) {
        tier = kMaxUpgradeTier;
    }
    return kBaseMaxHp + static_cast<int>(tier) * 25;
}

int shieldCapacityForTier(uint8_t tier) {
    if (tier == 0) {
        return 0;
    }
    if (tier > kMaxUpgradeTier) {
        tier = kMaxUpgradeTier;
    }
    return static_cast<int>(tier) * kShieldHpPerTier;
}

float moveBonusForSpeedTier(uint8_t tier) {
    if (tier > kMaxUpgradeTier) {
        tier = kMaxUpgradeTier;
    }
    return static_cast<float>(tier) * (kSpeedMoveBonusMk6 / static_cast<float>(kMaxUpgradeTier));
}

float turnBonusForSpeedTier(uint8_t tier) {
    if (tier > kMaxUpgradeTier) {
        tier = kMaxUpgradeTier;
    }
    return static_cast<float>(tier) * (kSpeedTurnBonusMk6 / static_cast<float>(kMaxUpgradeTier));
}

namespace {

constexpr int kDroneDamageByTier[] = {0, 2, 3, 4, 5, 6, 8};
constexpr float kDroneCooldownByTier[] = {0.0f, 0.85f, 0.72f, 0.60f, 0.50f, 0.42f, 0.35f};
constexpr float kDroneRangeByTier[] = {0.0f, 300.0f, 320.0f, 340.0f, 360.0f, 380.0f, 400.0f};

constexpr int kLaserDamageByTier[] = {0, 3, 4, 5, 6, 7, 9};
constexpr float kLaserCooldownByTier[] = {0.0f, 0.30f, 0.26f, 0.22f, 0.19f, 0.16f, 0.14f};
constexpr float kLaserRangeByTier[] = {0.0f, 320.0f, 335.0f, 350.0f, 365.0f, 380.0f, 400.0f};

constexpr int kMissileDamageByTier[] = {0, 6, 7, 8, 9, 10, 12};
constexpr float kMissileCooldownByTier[] = {0.0f, 0.50f, 0.42f, 0.36f, 0.31f, 0.28f, 0.24f};
constexpr float kMissileRangeByTier[] = {0.0f, 340.0f, 355.0f, 370.0f, 385.0f, 400.0f, 420.0f};

constexpr int kAirStrikeDamageByTier[] = {0, 12, 14, 16, 18, 21, 25};
constexpr float kAirStrikeCooldownByTier[] = {0.0f, 30.0f, 26.0f, 22.0f, 18.0f, 15.0f, 12.0f};
constexpr float kAirStrikeRadiusByTier[] = {0.0f, 55.0f, 62.0f, 69.0f, 76.0f, 84.0f, 92.0f};
constexpr float kAirStrikeRangeByTier[] = {0.0f, 450.0f, 480.0f, 510.0f, 540.0f, 570.0f, 600.0f};

uint8_t clampDroneTier(uint8_t tier) {
    if (tier > kMaxUpgradeTier) {
        return kMaxUpgradeTier;
    }
    return tier;
}

uint8_t clampUpgradeTier(uint8_t tier) {
    if (tier > kMaxUpgradeTier) {
        return kMaxUpgradeTier;
    }
    return tier;
}

} // namespace

int droneDamageForTier(uint8_t tier) {
    tier = clampDroneTier(tier);
    return kDroneDamageByTier[tier];
}

float droneCooldownForTier(uint8_t tier) {
    tier = clampDroneTier(tier);
    return kDroneCooldownByTier[tier];
}

float droneRangeForTier(uint8_t tier) {
    tier = clampDroneTier(tier);
    return kDroneRangeByTier[tier];
}

float droneAimConeDeg() {
    return 55.0f;
}

int laserDamageForTier(uint8_t tier) {
    tier = clampUpgradeTier(tier);
    return kLaserDamageByTier[tier];
}

float laserCooldownForTier(uint8_t tier) {
    tier = clampUpgradeTier(tier);
    return kLaserCooldownByTier[tier];
}

float laserRangeForTier(uint8_t tier) {
    tier = clampUpgradeTier(tier);
    return kLaserRangeByTier[tier];
}

float laserAimConeDeg() {
    return 48.0f;
}

int missileDamageForTier(uint8_t tier) {
    tier = clampUpgradeTier(tier);
    return kMissileDamageByTier[tier];
}

float missileCooldownForTier(uint8_t tier) {
    tier = clampUpgradeTier(tier);
    return kMissileCooldownByTier[tier];
}

float missileRangeForTier(uint8_t tier) {
    tier = clampUpgradeTier(tier);
    return kMissileRangeByTier[tier];
}

float missileAimConeDeg() {
    return 45.0f;
}

int airStrikeDamageForTier(uint8_t tier) {
    tier = clampUpgradeTier(tier);
    return kAirStrikeDamageByTier[tier];
}

float airStrikeCooldownForTier(uint8_t tier) {
    tier = clampUpgradeTier(tier);
    return kAirStrikeCooldownByTier[tier];
}

float airStrikeRadiusForTier(uint8_t tier) {
    tier = clampUpgradeTier(tier);
    return kAirStrikeRadiusByTier[tier];
}

float airStrikeRangeForTier(uint8_t tier) {
    tier = clampUpgradeTier(tier);
    return kAirStrikeRangeByTier[tier];
}

float airStrikeAimConeDeg() {
    return 48.0f;
}

float playerAutoFireRange(const RunBonuses& bonuses) {
    float range = laserRangeForTier(bonuses.laserTier);
    if (bonuses.missileTier > 0) {
        const float missileRange = missileRangeForTier(bonuses.missileTier);
        if (missileRange > range) {
            range = missileRange;
        }
    }
    return range;
}

uint16_t UpgradePicker::colorFor(UpgradeId id) {
    switch (id) {
    case UpgradeId::Armor:
        return Colors::HEALTH_FILL;
    case UpgradeId::Shield:
        return Colors::SHIELD_HUD;
    case UpgradeId::Speed:
        return Colors::ORANGE;
    case UpgradeId::Drone:
        return Colors::PORTAL_VOID;
    case UpgradeId::Laser:
        return Colors::TRACER_YELLOW;
    case UpgradeId::Missile:
        return Colors::MISSILE_GREY;
    case UpgradeId::AirStrike:
        return Colors::DAMAGE_RED;
    default:
        return Colors::HUD_TEXT;
    }
}

uint8_t UpgradePicker::currentTier(UpgradeId id, const RunBonuses& bonuses) {
    switch (id) {
    case UpgradeId::Armor:
        return bonuses.armorTier;
    case UpgradeId::Shield:
        return bonuses.shieldTier;
    case UpgradeId::Speed:
        return bonuses.speedTier;
    case UpgradeId::Drone:
        return bonuses.droneTier;
    case UpgradeId::Laser:
        return bonuses.laserTier;
    case UpgradeId::Missile:
        return bonuses.missileTier;
    case UpgradeId::AirStrike:
        return bonuses.airStrikeTier;
    default:
        return kMaxUpgradeTier;
    }
}

void UpgradePicker::buildOption(UpgradeOption& out, UpgradeId id, uint8_t tier) {
    out.id = id;
    out.tier = tier;
    out.color = colorFor(id);

    const char* name = "???";
    switch (id) {
    case UpgradeId::Armor:
        name = "ARMOR";
        break;
    case UpgradeId::Shield:
        name = "SHIELD";
        break;
    case UpgradeId::Speed:
        name = "SPEED";
        break;
    case UpgradeId::Drone:
        name = "LASER";
        break;
    case UpgradeId::Laser:
        name = "GUN";
        break;
    case UpgradeId::Missile:
        name = "MISSILE";
        break;
    case UpgradeId::AirStrike:
        name = "AIR";
        break;
    default:
        break;
    }

    snprintf(out.title, sizeof(out.title), "%s", name);
    if (id == UpgradeId::Drone) {
        snprintf(out.subtitle, sizeof(out.subtitle), "DRONE");
    } else if (id == UpgradeId::Laser) {
        snprintf(out.subtitle, sizeof(out.subtitle), "LASER");
    } else if (id == UpgradeId::AirStrike) {
        snprintf(out.subtitle, sizeof(out.subtitle), "STRIKE");
    } else {
        out.subtitle[0] = '\0';
    }
    snprintf(out.tierLabel, sizeof(out.tierLabel), "MK%u", static_cast<unsigned>(tier));
}

bool UpgradePicker::canApply(UpgradeId id, const RunBonuses& bonuses) {
    return currentTier(id, bonuses) < kMaxUpgradeTier;
}

void UpgradePicker::apply(const UpgradeOption& choice, Mech& mech, int& health, int& maxHealth) {
    MechLoadout& loadout = mech.loadout();
    RunBonuses& bonuses = loadout.bonuses();

    switch (choice.id) {
    case UpgradeId::Armor: {
        const int oldMax = mech.getMaxHp();
        bonuses.armorTier = choice.tier;
        const int newMax = mech.getMaxHp();
        const int gain = newMax - oldMax;
        maxHealth = newMax;
        if (gain > 0) {
            health += gain;
        }
        if (health > maxHealth) {
            health = maxHealth;
        }
        break;
    }
    case UpgradeId::Shield: {
        const uint8_t oldTier = bonuses.shieldTier;
        bonuses.shieldTier = choice.tier;
        mech.refreshShieldCapacity();
        if (oldTier == 0 && choice.tier >= 1) {
            mech.queueShieldDeploy();
        }
        break;
    }
    case UpgradeId::Speed:
        bonuses.speedTier = choice.tier;
        break;
    case UpgradeId::Drone:
        bonuses.droneTier = choice.tier;
        break;
    case UpgradeId::Laser:
        bonuses.laserTier = choice.tier;
        break;
    case UpgradeId::Missile:
        bonuses.missileTier = choice.tier;
        break;
    case UpgradeId::AirStrike:
        bonuses.airStrikeTier = choice.tier;
        break;
    default:
        break;
    }
}

void UpgradePicker::roll(const MechLoadout& loadout) {
    const RunBonuses& bonuses = loadout.bonuses();
    uint8_t pool[static_cast<uint8_t>(UpgradeId::COUNT)];
    uint8_t poolCount = 0;

    for (uint8_t i = 0; i < static_cast<uint8_t>(UpgradeId::COUNT); ++i) {
        const auto id = static_cast<UpgradeId>(i);
        if (canApply(id, bonuses)) {
            pool[poolCount++] = i;
        }
    }

    if (poolCount == 0) {
        buildOption(m_options[0], UpgradeId::Armor, kMaxUpgradeTier);
        buildOption(m_options[1], UpgradeId::Shield, kMaxUpgradeTier);
        snprintf(m_options[0].title, sizeof(m_options[0].title), "MAX");
        snprintf(m_options[1].title, sizeof(m_options[1].title), "MAX");
        m_options[0].tierLabel[0] = '\0';
        m_options[1].tierLabel[0] = '\0';
        return;
    }

    const uint8_t firstIdx = static_cast<uint8_t>(Rng::nextRange(poolCount));
    const UpgradeId firstId = static_cast<UpgradeId>(pool[firstIdx]);
    buildOption(m_options[0], firstId, static_cast<uint8_t>(currentTier(firstId, bonuses) + 1));

    if (poolCount == 1) {
        m_options[1] = m_options[0];
        return;
    }

    uint8_t secondIdx = static_cast<uint8_t>(Rng::nextRange(poolCount - 1));
    if (secondIdx >= firstIdx) {
        ++secondIdx;
    }
    const UpgradeId secondId = static_cast<UpgradeId>(pool[secondIdx]);
    buildOption(m_options[1], secondId, static_cast<uint8_t>(currentTier(secondId, bonuses) + 1));
}

} // namespace Game
