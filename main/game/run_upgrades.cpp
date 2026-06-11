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
    if (tier > kMaxUpgradeTier) {
        tier = kMaxUpgradeTier;
    }
    return kBaseShieldCapacity + static_cast<int>(tier) * 12;
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

uint16_t UpgradePicker::colorFor(UpgradeId id) {
    switch (id) {
    case UpgradeId::Armor:
        return Colors::HEALTH_FILL;
    case UpgradeId::Shield:
        return Colors::SHIELD_HUD;
    case UpgradeId::Speed:
        return Colors::ORANGE;
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
    default:
        break;
    }

    snprintf(out.title, sizeof(out.title), "%s", name);
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
    case UpgradeId::Shield:
        bonuses.shieldTier = choice.tier;
        mech.refreshShieldCapacity();
        break;
    case UpgradeId::Speed:
        bonuses.speedTier = choice.tier;
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
