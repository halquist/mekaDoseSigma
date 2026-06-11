#pragma once

#include "mech_loadout.hpp"
#include "types.hpp"
#include <cstdint>

namespace Game {

class Mech;

constexpr uint8_t kMaxUpgradeTier = 6;
constexpr int kBaseMaxHp = 100;
constexpr int kHpCapMk6 = 250;
constexpr int kShieldHpPerTier = 25;
constexpr int kShieldCapMk6 = 150;
constexpr float kShieldRedeployCooldownSec = 15.0f;
constexpr float kSpeedMoveBonusMk6 = 60.0f;
constexpr float kSpeedTurnBonusMk6 = 72.0f;

enum class UpgradeId : uint8_t {
    Armor,
    Shield,
    Speed,
    COUNT
};

struct UpgradeOption {
    UpgradeId id = UpgradeId::Armor;
    uint8_t tier = 1;
    char title[12] = {};
    char tierLabel[8] = {};
    uint16_t color = 0;
};

int maxHpForArmorTier(uint8_t tier);
int shieldCapacityForTier(uint8_t tier);
float moveBonusForSpeedTier(uint8_t tier);
float turnBonusForSpeedTier(uint8_t tier);

class UpgradePicker {
public:
    void roll(const MechLoadout& loadout);
    const UpgradeOption& option(int index) const { return m_options[index]; }

    static bool canApply(UpgradeId id, const RunBonuses& bonuses);
    static void apply(const UpgradeOption& choice, Mech& mech, int& health, int& maxHealth);

private:
    UpgradeOption m_options[2] = {};
    static void buildOption(UpgradeOption& out, UpgradeId id, uint8_t tier);
    static uint8_t currentTier(UpgradeId id, const RunBonuses& bonuses);
    static uint16_t colorFor(UpgradeId id);
};

} // namespace Game
