#include "game_ui.hpp"
#include "font.hpp"
#include "FramebufferIO.hpp"
#include <cmath>
#include <cstdio>

namespace Game {
namespace GameUi {

namespace {

constexpr int kSkipBandHeightDivisor = 5;

void fillRect(uint16_t* framebuffer, int width, int height,
              int x0, int y0, int x1, int y1, uint16_t color) {
    if (x0 < 0) {
        x0 = 0;
    }
    if (y0 < 0) {
        y0 = 0;
    }
    if (x1 > width) {
        x1 = width;
    }
    if (y1 > height) {
        y1 = height;
    }
    if (x0 >= x1 || y0 >= y1) {
        return;
    }

    const uint16_t packed = fbPack(color);
    for (int y = y0; y < y1; ++y) {
        uint16_t* row = framebuffer + y * width;
        for (int x = x0; x < x1; ++x) {
            row[x] = packed;
        }
    }
}

void drawUpgradeHalf(uint16_t* framebuffer, int width, int height,
                     int x0, int x1, int y0, int y1,
                     const UpgradeOption& option, bool selected) {
    const uint16_t bgColor = selected ? Colors::BLACK : option.color;
    const uint16_t textColor = selected ? option.color : Colors::BLACK;

    fillRect(framebuffer, width, height, x0, y0, x1, y1, bgColor);

    const int cx = (x0 + x1) / 2;
    const int cy = (y0 + y1) / 2;
    Font::drawTextCentered(framebuffer, width, height, option.title,
                           cx, cy - 34, 2, textColor);
    if (option.tierLabel[0] != '\0') {
        Font::drawTextCentered(framebuffer, width, height, option.tierLabel,
                               cx, cy - 10, 2, textColor);
    }
}

} // namespace

void fillScreen(uint16_t* framebuffer, int width, int height, uint16_t color) {
    fillRect(framebuffer, width, height, 0, 0, width, height, color);
}

void drawMenu(uint16_t* framebuffer, int width, int height,
              const int highScores[HighScores::kTopCount], float pulseSec) {
    const int cx = width / 2;
    Font::drawTextCentered(framebuffer, width, height, "MEKA",
                           cx, 28, 2, Colors::MECH_WHITE);
    Font::drawTextCentered(framebuffer, width, height, "CODE SIGMA",
                           cx, 50, 2, Colors::MECH_WHITE);
    Font::drawTextCentered(framebuffer, width, height, "HIGH SCORES",
                           cx, 82, 1, Colors::HUD_TEXT);

    for (int i = 0; i < HighScores::kTopCount; ++i) {
        char line[16];
        snprintf(line, sizeof(line), "%d %d", i + 1, highScores[i]);
        const int rowY = 102 + i * 16;
        Font::drawTextCentered(framebuffer, width, height, line,
                               cx, rowY, 1, Colors::HUD_TEXT);
    }

    const bool pulseOn = fmodf(pulseSec, 1.2f) < 0.7f;
    if (pulseOn) {
        Font::drawTextCentered(framebuffer, width, height, "TAP TO START",
                               cx, height - 36, 1, Colors::OBJECTIVE_ARROW);
    }
}

void drawDefeat(uint16_t* framebuffer, int width, int height,
                const RunScore& score,
                const int highScores[HighScores::kTopCount],
                bool newHighScore) {
    fillScreen(framebuffer, width, height, Colors::BLACK);

    const int cx = width / 2;
    const int total = score.total();

    Font::drawTextCentered(framebuffer, width, height, "GAME OVER",
                           cx, 40, 2, Colors::DAMAGE_RED);
    Font::drawTextCentered(framebuffer, width, height, "SCORE",
                           cx, 82, 1, Colors::HUD_TEXT);
    Font::drawNumber(framebuffer, width, height, total,
                     cx, 98, 2,
                     newHighScore ? Colors::VICTORY_GREEN : Colors::HUD_TEXT);

    Font::drawTextCentered(framebuffer, width, height, "HIGH SCORES",
                           cx, 138, 1, Colors::HUD_TEXT);
    for (int i = 0; i < HighScores::kTopCount; ++i) {
        char line[16];
        snprintf(line, sizeof(line), "%d %d", i + 1, highScores[i]);
        const int rowY = 156 + i * 16;
        Font::drawTextCentered(framebuffer, width, height, line,
                               cx, rowY, 1, Colors::HUD_TEXT);
    }

    Font::drawTextCentered(framebuffer, width, height, "TAP TO CONTINUE",
                           cx, height - 32, 1, Colors::HUD_TEXT);
}

void drawUpgradePick(uint16_t* framebuffer, int width, int height,
                     const UpgradeOption& left, const UpgradeOption& right,
                     int selectedChoice, int score) {
    const int skipBottom = height / kSkipBandHeightDivisor;
    fillRect(framebuffer, width, height, 0, 0, width, skipBottom,
             Colors::OBJECTIVE_ARROW);
    Font::drawTextCentered(framebuffer, width, height, "SKIP",
                           width / 2, skipBottom / 2 - 1, 2, Colors::BLACK);

    const int choiceTop = skipBottom;
    drawUpgradeHalf(framebuffer, width, height,
                    0, width / 2, choiceTop, height,
                    left, selectedChoice == 0);
    drawUpgradeHalf(framebuffer, width, height,
                    width / 2, width, choiceTop, height,
                    right, selectedChoice == 1);

    Font::drawTextCentered(framebuffer, width, height, "SCORE",
                           width / 2, height - 44, 1, Colors::BLACK);
    Font::drawNumber(framebuffer, width, height, score,
                     width / 2, height - 32, 2, Colors::BLACK);
}

int upgradePickFromTouch(int touchX, int touchY, int width, int height) {
    (void)touchX;

    const int skipTouchMinY = (height * (kSkipBandHeightDivisor - 1))
                              / kSkipBandHeightDivisor;
    if (touchY >= skipTouchMinY) {
        return 2;
    }

    return touchX < width / 2 ? 0 : 1;
}

} // namespace GameUi
} // namespace Game
