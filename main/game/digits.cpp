#include "digits.hpp"

namespace Game {

const uint8_t Digits::fontData[10][7] = {
    { 0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110 },
    { 0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110 },
    { 0b01110, 0b10001, 0b00001, 0b00110, 0b01000, 0b10000, 0b11111 },
    { 0b01110, 0b10001, 0b00001, 0b00110, 0b00001, 0b10001, 0b01110 },
    { 0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010 },
    { 0b11111, 0b10000, 0b11110, 0b00001, 0b00001, 0b10001, 0b01110 },
    { 0b00110, 0b01000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110 },
    { 0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000 },
    { 0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110 },
    { 0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00010, 0b01100 },
};

void Digits::drawDigit(uint16_t* framebuffer, int screenWidth, int screenHeight,
                       int digit, int x, int y, uint16_t color) {
    if (digit < 0 || digit > 9) return;

    for (int row = 0; row < DIGIT_HEIGHT; row++) {
        uint8_t rowData = fontData[digit][row];
        for (int col = 0; col < DIGIT_WIDTH; col++) {
            if (rowData & (0b00001 << col)) {
                int px = x + col;
                int py = y + row;
                if (px >= 0 && px < screenWidth && py >= 0 && py < screenHeight) {
                    framebuffer[py * screenWidth + px] = color;
                }
            }
        }
    }
}

void Digits::drawNumber(uint16_t* framebuffer, int screenWidth, int screenHeight,
                        int number, int centerX, int y, uint16_t color) {
    char buffer[12];
    int len = 0;

    if (number == 0) {
        buffer[len++] = '0';
    } else {
        int temp = number;
        while (temp > 0 && len < 11) {
            buffer[len++] = '0' + (temp % 10);
            temp /= 10;
        }
    }
    buffer[len] = '\0';

    int totalWidth = len * DIGIT_WIDTH + (len - 1) * DIGIT_SPACING;
    int startX = centerX - totalWidth / 2;

    for (int i = 0; i < len; i++) {
        int digit = buffer[i] - '0';
        drawDigit(framebuffer, screenWidth, screenHeight,
                  digit, startX + i * (DIGIT_WIDTH + DIGIT_SPACING), y, color);
    }
}

void Digits::drawHealthBar(uint16_t* framebuffer, int screenWidth, int screenHeight,
                           int health, int maxHealth, int centerX, int y,
                           uint16_t fillColor, uint16_t bgColor) {
    const int BAR_WIDTH = 50;
    const int BAR_HEIGHT = 6;

    int startX = centerX - BAR_WIDTH / 2;
    int fillWidth = (health * BAR_WIDTH) / maxHealth;

    for (int row = 0; row < BAR_HEIGHT; row++) {
        for (int col = 0; col < BAR_WIDTH; col++) {
            int px = startX + col;
            int py = y + row;
            if (px >= 0 && px < screenWidth && py >= 0 && py < screenHeight) {
                bool isBorder = (row == 0 || row == BAR_HEIGHT - 1 || col == 0 || col == BAR_WIDTH - 1);
                bool isFilled = (col < fillWidth);

                if (isBorder) {
                    framebuffer[py * screenWidth + px] = fillColor;
                } else if (isFilled) {
                    framebuffer[py * screenWidth + px] = fillColor;
                } else {
                    framebuffer[py * screenWidth + px] = bgColor;
                }
            }
        }
    }
}

} // namespace Game
