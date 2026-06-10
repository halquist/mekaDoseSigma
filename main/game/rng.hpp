#pragma once

#include <cstdint>

namespace Game {

/// Session RNG seeded from the ESP32 hardware random source.
class Rng {
public:
    static void init();
    static uint32_t nextU32();
    static uint32_t nextRange(uint32_t maxExclusive);
    static bool coinFlip();
    static float nextFloat01();

private:
    static uint32_t s_state;
};

} // namespace Game
