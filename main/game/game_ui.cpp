#include "game_ui.hpp"
#include "font.hpp"
#include "FramebufferIO.hpp"
#include <cmath>
#include <cstdio>

namespace Game {
namespace GameUi {

void fillScreen(uint16_t* framebuffer, int width, int height, uint16_t color) {
    const uint16_t packed = fbPack(color);
    const int pixelCount = width * height;
    for (int i = 0; i < pixelCount; ++i) {
        framebuffer[i] = packed;
    }
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
                     const UpgradeOption& left, const UpgradeOption& right) {
    fillScreen(framebuffer, width, height, Colors::BLACK);

    const int cx = width / 2;
    Font::drawTextCentered(framebuffer, width, height, "CHOOSE UPGRADE",
                           cx, 36, 1, Colors::OBJECTIVE_ARROW);

    const int choiceY = height / 2 + 18;
    const int leftCx = width / 4;
    const int rightCx = (width * 3) / 4;

    Font::drawTextCentered(framebuffer, width, height, left.tag,
                           leftCx, choiceY - 10, 2, left.color);
    Font::drawTextCentered(framebuffer, width, height, "LEFT",
                           leftCx, choiceY + 24, 1, Colors::HUD_TEXT);
    Font::drawTextCentered(framebuffer, width, height, right.tag,
                           rightCx, choiceY - 10, 2, right.color);
    Font::drawTextCentered(framebuffer, width, height, "RIGHT",
                           rightCx, choiceY + 24, 1, Colors::HUD_TEXT);
}

int upgradePickFromTouch(int touchX, int touchY, int width, int height) {
    if (touchY < height / 2) {
        return -1;
    }
    if (touchX < width / 2) {
        return 0;
    }
    return 1;
}

} // namespace GameUi
} // namespace Game
