#pragma once

#include "high_scores.hpp"
#include "run_upgrades.hpp"
#include "score.hpp"
#include "types.hpp"
#include <cstdint>

namespace Game {

namespace GameUi {

void fillScreen(uint16_t* framebuffer, int width, int height, uint16_t color);

void drawMenu(uint16_t* framebuffer, int width, int height,
              const int highScores[HighScores::kTopCount], float pulseSec);
void drawDefeat(uint16_t* framebuffer, int width, int height,
                const RunScore& score,
                const int highScores[HighScores::kTopCount],
                bool newHighScore);
void drawUpgradePick(uint16_t* framebuffer, int width, int height,
                     const UpgradeOption& left, const UpgradeOption& right);

/// Returns 0 = left choice, 1 = right choice, -1 if no valid pick.
int upgradePickFromTouch(int touchX, int touchY, int width, int height);

} // namespace GameUi

} // namespace Game
