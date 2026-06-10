#include "rng.hpp"
#include "esp_random.h"

namespace Game {

uint32_t Rng::s_state = 0;

void Rng::init() {
    s_state = esp_random();
    if (s_state == 0) {
        s_state = 0xA341316Cu;
    }
    // Stir once so the first nextU32() is not the raw hardware word.
    (void)nextU32();
}

uint32_t Rng::nextU32() {
    uint32_t x = s_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    s_state = x;
    return x;
}

uint32_t Rng::nextRange(uint32_t maxExclusive) {
    if (maxExclusive <= 1) {
        return 0;
    }
    return nextU32() % maxExclusive;
}

bool Rng::coinFlip() {
    return (nextU32() & 1u) != 0;
}

float Rng::nextFloat01() {
    return static_cast<float>(nextU32() >> 8) * (1.0f / 16777216.0f);
}

} // namespace Game
