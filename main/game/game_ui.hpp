#pragma once

#include "run_upgrades.hpp"
#include "score.hpp"
#include "types.hpp"
#include <cstdint>

namespace Game {

namespace GameUi {

void drawMenu(uint16_t* framebuffer, int width, int height, int bestScore, float pulseSec);
void drawDefeat(uint16_t* framebuffer, int width, int height,
                const RunScore& score, int bestScore, bool newHighScore);
void drawUpgradePick(uint16_t* framebuffer, int width, int height,
                     const UpgradeOption& left, const UpgradeOption& right);

/// Returns 0 = left choice, 1 = right choice, -1 if no valid pick.
int upgradePickFromTouch(int touchX, int touchY, int width, int height);

} // namespace GameUi

} // namespace Game
