#pragma once

#include "mech_loadout.hpp"
#include <cstdint>

namespace Game {

class Mech;

enum class UpgradeId : uint8_t {
    MissileMk2,
    RapidBolt,
    LegBoost,
    MaxHp,
    MoveSpeed,
    TurnRate,
    ShieldBoost,
    DamageUp,
    COUNT
};

struct UpgradeOption {
    UpgradeId id = UpgradeId::MissileMk2;
    const char* tag = "";
    uint16_t color = 0;
};

class UpgradePicker {
public:
    void roll(const MechLoadout& loadout);
    const UpgradeOption& option(int index) const { return m_options[index]; }

    static bool canApply(UpgradeId id, const MechLoadout& loadout);
    static void apply(UpgradeId id, Mech& mech, int& health, int& maxHealth);

private:
    UpgradeOption m_options[2] = {};
    static const UpgradeOption& catalogEntry(UpgradeId id);
};

} // namespace Game
