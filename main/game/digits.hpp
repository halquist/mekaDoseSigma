#pragma once

#include <cstdint>

namespace Game {

class Digits {
public:
    /// Health arc with optional inner shield capacity arc in the same pixel pass.
    /// Arc fill lengths scale against @p healthCapVisual / @p shieldCapVisual (MK6 caps).
    static void drawHealthAndShieldArcs(uint16_t* framebuffer, int screenWidth, int screenHeight,
                                        int health, int maxHealth,
                                        int healthCapVisual,
                                        uint16_t healthFillColor, uint16_t bgColor,
                                        bool showShieldCapacityRing, int maxShieldHp,
                                        int shieldCapVisual,
                                        bool showShieldActiveFill, int shieldHp,
                                        uint16_t shieldFillColor);

    /// Upper semicircular health arc hugging the round screen bezel.
    static void drawHealthArc(uint16_t* framebuffer, int screenWidth, int screenHeight,
                              int health, int maxHealth,
                              uint16_t fillColor, uint16_t bgColor);

    /// Inner blue arc below the health bar while shield is active.
    static void drawShieldArc(uint16_t* framebuffer, int screenWidth, int screenHeight,
                              int shieldHp, int maxShieldHp,
                              uint16_t fillColor, uint16_t bgColor);

    /// Small icon below the health arc when an ability is ready.
    static void drawAbilityReadyIcon(uint16_t* framebuffer, int screenWidth, int screenHeight,
                                     uint16_t color);
};

} // namespace Game
