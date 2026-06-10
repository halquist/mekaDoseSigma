#pragma once

#include <cstdint>

namespace Game {

class Digits {
public:
    /// Upper semicircular health arc hugging the round screen bezel.
    static void drawHealthArc(uint16_t* framebuffer, int screenWidth, int screenHeight,
                              int health, int maxHealth,
                              uint16_t fillColor, uint16_t bgColor);
};

} // namespace Game
