#pragma once

#include <cstdint>

namespace Game {

/// 5×7 bitmap font with panel-mirror column order (see basic_engine_test_game).
class Font {
public:
    static constexpr int GLYPH_WIDTH = 5;
    static constexpr int GLYPH_HEIGHT = 7;
    static constexpr int GLYPH_SPACING = 2;

    static void drawChar(uint16_t* framebuffer, int screenWidth, int screenHeight,
                         char ch, int x, int y, int scale, uint16_t color);

    static void drawText(uint16_t* framebuffer, int screenWidth, int screenHeight,
                         const char* text, int x, int y, int scale, uint16_t color);

    static void drawTextCentered(uint16_t* framebuffer, int screenWidth, int screenHeight,
                                 const char* text, int centerX, int y, int scale,
                                 uint16_t color);

    static void drawNumber(uint16_t* framebuffer, int screenWidth, int screenHeight,
                           int value, int centerX, int y, int scale, uint16_t color);

    static void drawNumberRight(uint16_t* framebuffer, int screenWidth, int screenHeight,
                                int value, int rightX, int y, int scale, uint16_t color);

    static int measureTextWidth(const char* text, int scale);
    static int measureNumberWidth(int value, int scale);
};

} // namespace Game
