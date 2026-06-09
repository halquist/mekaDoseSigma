#pragma once

#include <cstdint>

namespace Game {

class Digits {
public:
    static void drawNumber(uint16_t* framebuffer, int screenWidth, int screenHeight,
                           int number, int centerX, int y, uint16_t color);

    static void drawHealthBar(uint16_t* framebuffer, int screenWidth, int screenHeight,
                              int health, int maxHealth, int centerX, int y,
                              uint16_t fillColor, uint16_t bgColor);

private:
    static void drawDigit(uint16_t* framebuffer, int screenWidth, int screenHeight,
                          int digit, int x, int y, uint16_t color);

    static constexpr int DIGIT_WIDTH = 5;
    static constexpr int DIGIT_HEIGHT = 7;
    static constexpr int DIGIT_SPACING = 2;

    static const uint8_t fontData[10][7];
};

} // namespace Game
