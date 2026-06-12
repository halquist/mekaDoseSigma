#pragma once

#include "high_scores.hpp"
#include "run_upgrades.hpp"
#include "score.hpp"
#include "types.hpp"
#include <cstdint>

namespace Game {

namespace GameUi {

void fillScreen(uint16_t* framebuffer, int width, int height, uint16_t color);

void resetMenuStreaks();
void drawMenuStreaks(uint16_t* framebuffer, int width, int height, float deltaTime);

struct MenuDoorAnim {
    float cover = 0.0f;
    bool showGameplay = false;
};

float menuDoorTransitionDuration();
float menuDoorGameplayStartTime();
MenuDoorAnim menuDoorAnimForTime(float elapsedSec);
void drawMenuDoors(uint16_t* framebuffer, int width, int height,
                   const MenuDoorAnim& anim);

struct UpgradePickAnim {
    float sideCover    = 0.0f;
    float topCover     = 0.0f;
    float botCover     = 0.0f;
    int   invertChoice = -1;
};

UpgradePickAnim upgradePickAnimForTime(float entrySec, float confirmSec,
                                       int selectedChoice);

void drawMenu(uint16_t* framebuffer, int width, int height,
              const int highScores[HighScores::kTopCount], float pulseSec);
void drawDefeat(uint16_t* framebuffer, int width, int height,
                const RunScore& score,
                const int highScores[HighScores::kTopCount],
                bool newHighScore);
void drawUpgradePick(uint16_t* framebuffer, int width, int height,
                     const UpgradeOption& left, const UpgradeOption& right,
                     int selectedChoice, int score,
                     const UpgradePickAnim& anim);

/// Full-screen portal wipe: circle grows from center with @p alpha (0..255).
void drawPortalTransitionSphere(uint16_t* framebuffer, int width, int height,
                                float radiusPx, uint8_t alpha);

/// Returns 0 = left, 1 = right, 2 = skip, -1 = no action.
int upgradePickFromTouch(int touchX, int touchY, int width, int height);

} // namespace GameUi

} // namespace Game
