#pragma once

#include <cstdint>

namespace Game {

struct RunBonuses {
    uint8_t armorTier = 0;   // 0 = none purchased, 1–6 = MK level
    uint8_t shieldTier = 0;
    uint8_t speedTier = 0;
    uint8_t droneTier = 0;
    uint8_t laserTier = 1;   // 1 = MK1 default weapon, 2–6 = upgrades
    uint8_t missileTier = 0; // 0 = locked, 1–6 = unlock + upgrades
};

} // namespace Game
