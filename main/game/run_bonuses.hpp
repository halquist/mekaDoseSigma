#pragma once

#include <cstdint>

namespace Game {

struct RunBonuses {
    uint8_t armorTier = 0;   // 0 = none purchased, 1–6 = MK level
    uint8_t shieldTier = 0;
    uint8_t speedTier = 0;
};

} // namespace Game
