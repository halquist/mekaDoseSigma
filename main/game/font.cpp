#include "font.hpp"
#include "FramebufferIO.hpp"

namespace Game {

namespace {

constexpr uint8_t kGlyphHeight = 7;

const uint8_t kDigits[10][7] = {
    {0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110},
    {0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110},
    {0b01110, 0b10001, 0b00001, 0b00110, 0b01000, 0b10000, 0b11111},
    {0b01110, 0b10001, 0b00001, 0b00110, 0b00001, 0b10001, 0b01110},
    {0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010},
    {0b11111, 0b10000, 0b11110, 0b00001, 0b00001, 0b10001, 0b01110},
    {0b00110, 0b01000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110},
    {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000},
    {0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110},
    {0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00010, 0b01100},
};

const uint8_t kLetters[26][7] = {
    {0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001},
    {0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110},
    {0b01110, 0b10001, 0b10000, 0b10000, 0b10000, 0b10001, 0b01110},
    {0b11110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11110},
    {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111},
    {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000},
    {0b01110, 0b10001, 0b10000, 0b10111, 0b10001, 0b10001, 0b01110},
    {0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001},
    {0b01110, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110},
    {0b00111, 0b00010, 0b00010, 0b00010, 0b00010, 0b10010, 0b01100},
    {0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001},
    {0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111},
    {0b10001, 0b11011, 0b10101, 0b10001, 0b10001, 0b10001, 0b10001},
    {0b10001, 0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001},
    {0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110},
    {0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000},
    {0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101},
    {0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001},
    {0b01110, 0b10001, 0b10000, 0b01110, 0b00001, 0b10001, 0b01110},
    {0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100},
    {0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110},
    {0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100},
    {0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b11011, 0b10001},
    {0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001},
    {0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100},
    {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111},
};

const uint8_t kSpace[7] = {0, 0, 0, 0, 0, 0, 0};

const uint8_t* glyphRows(char ch) {
    if (ch >= '0' && ch <= '9') {
        return kDigits[ch - '0'];
    }
    if (ch >= 'a' && ch <= 'z') {
        return kLetters[ch - 'a'];
    }
    if (ch >= 'A' && ch <= 'Z') {
        return kLetters[ch - 'A'];
    }
    if (ch == ' ') {
        return kSpace;
    }
    return kSpace;
}

void drawGlyphRows(uint16_t* framebuffer, int screenWidth, int screenHeight,
                   const uint8_t* rows, int x, int y, int scale, uint16_t packedColor) {
    for (int row = 0; row < kGlyphHeight; ++row) {
        const uint8_t rowData = rows[row];
        for (int col = 0; col < Font::GLYPH_WIDTH; ++col) {
            if ((rowData & (0b00001 << col)) == 0) {
                continue;
            }
            for (int sy = 0; sy < scale; ++sy) {
                const int py = y + row * scale + sy;
                if (py < 0 || py >= screenHeight) {
                    continue;
                }
                for (int sx = 0; sx < scale; ++sx) {
                    const int px = x + col * scale + sx;
                    if (px < 0 || px >= screenWidth) {
                        continue;
                    }
                    framebuffer[py * screenWidth + px] = packedColor;
                }
            }
        }
    }
}

} // namespace

void Font::drawChar(uint16_t* framebuffer, int screenWidth, int screenHeight,
                    char ch, int x, int y, int scale, uint16_t color) {
    drawGlyphRows(framebuffer, screenWidth, screenHeight,
                  glyphRows(ch), x, y, scale, fbPack(color));
}

int Font::measureTextWidth(const char* text, int scale) {
    if (!text || text[0] == '\0') {
        return 0;
    }

    int chars = 0;
    for (const char* p = text; *p != '\0'; ++p) {
        ++chars;
    }

    const int advance = (GLYPH_WIDTH + GLYPH_SPACING) * scale;
    return chars * advance - GLYPH_SPACING * scale;
}

int Font::measureNumberWidth(int value, int scale) {
    char buffer[12];
    int len = 0;
    if (value <= 0) {
        buffer[len++] = '0';
    } else {
        int tmp = value;
        while (tmp > 0 && len < 11) {
            buffer[len++] = static_cast<char>('0' + (tmp % 10));
            tmp /= 10;
        }
    }
    buffer[len] = '\0';
    return measureTextWidth(buffer, scale);
}

void Font::drawText(uint16_t* framebuffer, int screenWidth, int screenHeight,
                    const char* text, int x, int y, int scale, uint16_t color) {
    if (!text) {
        return;
    }

    int len = 0;
    while (text[len] != '\0') {
        ++len;
    }
    if (len == 0) {
        return;
    }

    // GC9A01 swap_xy + mirror flips horizontal order; emit glyphs last-to-first.
    const int advance = (GLYPH_WIDTH + GLYPH_SPACING) * scale;
    int cursorX = x;
    for (int i = len - 1; i >= 0; --i) {
        drawChar(framebuffer, screenWidth, screenHeight,
                 text[i], cursorX, y, scale, color);
        cursorX += advance;
    }
}

void Font::drawTextCentered(uint16_t* framebuffer, int screenWidth, int screenHeight,
                            const char* text, int centerX, int y, int scale,
                            uint16_t color) {
    const int width = measureTextWidth(text, scale);
    drawText(framebuffer, screenWidth, screenHeight,
             text, centerX - width / 2, y, scale, color);
}

void Font::drawNumber(uint16_t* framebuffer, int screenWidth, int screenHeight,
                      int value, int centerX, int y, int scale, uint16_t color) {
    if (value < 0) {
        value = 0;
    }
    if (value > 999999) {
        value = 999999;
    }

    char buffer[12];
    int len = 0;
    if (value == 0) {
        buffer[len++] = '0';
    } else {
        int tmp = value;
        while (tmp > 0 && len < 11) {
            buffer[len++] = static_cast<char>('0' + (tmp % 10));
            tmp /= 10;
        }
    }
    buffer[len] = '\0';

    drawTextCentered(framebuffer, screenWidth, screenHeight,
                     buffer, centerX, y, scale, color);
}

void Font::drawNumberRight(uint16_t* framebuffer, int screenWidth, int screenHeight,
                           int value, int rightX, int y, int scale, uint16_t color) {
    if (value < 0) {
        value = 0;
    }
    if (value > 999999) {
        value = 999999;
    }

    char buffer[12];
    int len = 0;
    if (value == 0) {
        buffer[len++] = '0';
    } else {
        int tmp = value;
        while (tmp > 0 && len < 11) {
            buffer[len++] = static_cast<char>('0' + (tmp % 10));
            tmp /= 10;
        }
    }
    buffer[len] = '\0';

    const int width = measureTextWidth(buffer, scale);
    drawText(framebuffer, screenWidth, screenHeight,
             buffer, rightX - width, y, scale, color);
}

} // namespace Game
