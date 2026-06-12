#include "game_ui.hpp"
#include "font.hpp"
#include "FramebufferIO.hpp"
#include "rng.hpp"
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
    if (option.subtitle[0] != '\0') {
        Font::drawTextCentered(framebuffer, width, height, option.subtitle,
                               cx, cy - 14, 2, textColor);
        if (option.tierLabel[0] != '\0') {
            Font::drawTextCentered(framebuffer, width, height, option.tierLabel,
                                   cx, cy + 6, 2, textColor);
        }
    } else if (option.tierLabel[0] != '\0') {
        Font::drawTextCentered(framebuffer, width, height, option.tierLabel,
                               cx, cy - 10, 2, textColor);
    }
}

uint16_t blendRgb565(uint16_t dst, uint16_t src, uint8_t alpha) {
    const uint32_t a = alpha;
    const uint32_t inv = 255u - a;
    const uint32_t dr = (dst >> 11) & 0x1Fu;
    const uint32_t dg = (dst >> 5) & 0x3Fu;
    const uint32_t db = dst & 0x1Fu;
    const uint32_t sr = (src >> 11) & 0x1Fu;
    const uint32_t sg = (src >> 5) & 0x3Fu;
    const uint32_t sb = src & 0x1Fu;
    const uint32_t r = (sr * a + dr * inv + 127u) / 255u;
    const uint32_t g = (sg * a + dg * inv + 127u) / 255u;
    const uint32_t b = (sb * a + db * inv + 127u) / 255u;
    return static_cast<uint16_t>((r << 11) | (g << 5) | b);
}

float smoothStep(float t) {
    if (t <= 0.0f) {
        return 0.0f;
    }
    if (t >= 1.0f) {
        return 1.0f;
    }
    return t * t * (3.0f - 2.0f * t);
}

} // namespace

void drawPortalTransitionSphere(uint16_t* framebuffer, int width, int height,
                                float radiusPx, uint8_t alpha) {
    if (alpha == 0 || radiusPx <= 0.5f) {
        return;
    }

    const int cx = width / 2;
    const int cy = height / 2;
    const int r = static_cast<int>(radiusPx);
    if (r <= 0) {
        return;
    }

    const int rSq = r * r;
    const int yMin = cy - r;
    const int yMax = cy + r;
    const uint16_t srcColor = Colors::PORTAL_VOID;

    for (int y = yMin; y <= yMax; ++y) {
        if (y < 0 || y >= height) {
            continue;
        }

        const int dy = y - cy;
        const int dySq = dy * dy;
        if (dySq > rSq) {
            continue;
        }

        const int dxMax = static_cast<int>(sqrtf(static_cast<float>(rSq - dySq)));
        const int x0 = cx - dxMax;
        const int x1 = cx + dxMax;
        const int clipX0 = x0 < 0 ? 0 : x0;
        const int clipX1 = x1 >= width ? width - 1 : x1;

        uint16_t* row = framebuffer + y * width;
        for (int x = clipX0; x <= clipX1; ++x) {
            const uint16_t dst = fbUnpack(row[x]);
            row[x] = fbPack(blendRgb565(dst, srcColor, alpha));
        }
    }
}

void fillScreen(uint16_t* framebuffer, int width, int height, uint16_t color) {
    fillRect(framebuffer, width, height, 0, 0, width, height, color);
}

namespace {

struct MenuStreak {
    float along = 0.0f;
    float lateral = 0.0f;
    float speed = 50.0f;
    float length = 24.0f;
    uint16_t color = 0;
    uint8_t alpha = 120;
    int8_t thickness = 0;
};

constexpr int kMenuStreakCount = 22;
MenuStreak s_menuStreaks[kMenuStreakCount];
bool s_menuStreaksReady = false;

constexpr uint16_t kMenuStreakColors[4] = {
    Colors::rgb(150, 70, 220),
    Colors::rgb(45, 95, 230),
    Colors::rgb(255, 215, 70),
    Colors::MECH_WHITE,
};

void rollMenuStreak(MenuStreak& streak, float minLateral, float maxLateral,
                    float travelSpan, bool scatterAlong) {
    streak.lateral = minLateral + Rng::nextFloat01() * (maxLateral - minLateral);
    streak.speed = 38.0f + Rng::nextFloat01() * 62.0f;
    streak.length = 12.0f + Rng::nextFloat01() * 38.0f;
    streak.thickness = Rng::nextFloat01() > 0.82f ? 1 : 0;
    streak.color = kMenuStreakColors[Rng::nextRange(4)];
    const float brightness = Rng::nextFloat01();
    streak.alpha = static_cast<uint8_t>(95.0f + brightness * 105.0f);
    if (streak.color == Colors::MECH_WHITE) {
        streak.alpha = static_cast<uint8_t>(streak.alpha * 0.72f);
    }

    streak.along = -90.0f;
    if (scatterAlong) {
        streak.along -= Rng::nextFloat01() * (travelSpan + 80.0f);
    }
}

} // namespace

void resetMenuStreaks() {
    s_menuStreaksReady = false;
}

void drawMenuStreaks(uint16_t* framebuffer, int width, int height, float deltaTime) {
    if (deltaTime < 0.0f) {
        deltaTime = 0.0f;
    } else if (deltaTime > 0.1f) {
        deltaTime = 0.1f;
    }

    constexpr int kStreakCount = kMenuStreakCount;
    constexpr float kDiagCos = 0.70710678f;
    constexpr float kDiagSin = 0.70710678f;
    constexpr float kPerpCos = -kDiagSin;
    constexpr float kPerpSin = kDiagCos;

    const float travelSpan = static_cast<float>(width + height) + 180.0f;

    auto cornerLateral = [&](float x, float y) {
        return x * kPerpCos + y * kPerpSin;
    };

    float minLateral = cornerLateral(0.0f, 0.0f);
    float maxLateral = minLateral;
    const float corners[4][2] = {
        {0.0f, 0.0f},
        {static_cast<float>(width), 0.0f},
        {0.0f, static_cast<float>(height)},
        {static_cast<float>(width), static_cast<float>(height)},
    };
    for (const auto& corner : corners) {
        const float lateral = cornerLateral(corner[0], corner[1]);
        if (lateral < minLateral) {
            minLateral = lateral;
        }
        if (lateral > maxLateral) {
            maxLateral = lateral;
        }
    }

    auto plot = [&](int x, int y, uint16_t color, uint8_t alpha) {
        if ((unsigned)x >= (unsigned)width || (unsigned)y >= (unsigned)height) {
            return;
        }
        uint16_t* pixel = framebuffer + y * width + x;
        const uint16_t dst = fbUnpack(*pixel);
        if (dst != 0) {
            return;
        }
        *pixel = fbPack(blendRgb565(0, color, alpha));
    };

    auto drawStreak = [&](float headX, float headY, float length, int thickness,
                          uint16_t color, uint8_t headAlpha) {
        const int steps = static_cast<int>(length);
        if (steps <= 0) {
            return;
        }

        for (int s = 0; s < steps; ++s) {
            const float t = static_cast<float>(s);
            const int x = static_cast<int>(headX - t * kDiagCos);
            const int y = static_cast<int>(headY - t * kDiagSin);
            const uint8_t alpha = static_cast<uint8_t>(
                static_cast<float>(headAlpha) * (1.0f - t / length));
            if (alpha < 8) {
                continue;
            }

            for (int dy = -thickness; dy <= thickness; ++dy) {
                for (int dx = -thickness; dx <= thickness; ++dx) {
                    if (dx * dx + dy * dy > thickness * thickness + thickness) {
                        continue;
                    }
                    plot(x + dx, y + dy, color, alpha);
                }
            }
        }
    };

    if (!s_menuStreaksReady) {
        for (int i = 0; i < kStreakCount; ++i) {
            rollMenuStreak(s_menuStreaks[i], minLateral, maxLateral, travelSpan, true);
        }
        s_menuStreaksReady = true;
    }

    const float alongStepScale = travelSpan * deltaTime;
    for (int i = 0; i < kStreakCount; ++i) {
        MenuStreak& streak = s_menuStreaks[i];
        streak.along += streak.speed * 0.032f * alongStepScale;
        if ((streak.along + 90.0f) >= travelSpan) {
            rollMenuStreak(streak, minLateral, maxLateral, travelSpan, false);
        }

        const float headX = streak.along * kDiagCos + streak.lateral * kPerpCos;
        const float headY = streak.along * kDiagSin + streak.lateral * kPerpSin;

        drawStreak(headX, headY, streak.length, streak.thickness,
                   streak.color, streak.alpha);
    }
}

namespace {

constexpr float kMenuDoorCloseSec = 0.5f;
constexpr float kMenuDoorPauseSec = 0.5f;
constexpr float kMenuDoorOpenSec = 0.5f;

constexpr uint16_t kDoorGrey = Colors::rgb(58, 61, 66);
constexpr uint16_t kDoorSeam = Colors::rgb(16, 18, 20);
constexpr uint16_t kDoorRivet = Colors::rgb(38, 40, 44);
constexpr uint16_t kDoorRivetHi = Colors::rgb(98, 102, 110);
constexpr uint16_t kDoorStreakLarge = Colors::rgb(88, 92, 98);
constexpr uint16_t kDoorStreakSmall = Colors::rgb(74, 78, 84);

void putPixel(uint16_t* framebuffer, int width, int height,
              int x, int y, uint16_t color) {
    if ((unsigned)x >= (unsigned)width || (unsigned)y >= (unsigned)height) {
        return;
    }
    framebuffer[y * width + x] = fbPack(color);
}

void fillDoorRect(uint16_t* framebuffer, int width, int height,
                  int x0, int x1, int y0, int y1, uint16_t color) {
    if (x0 > x1) {
        const int tmp = x0;
        x0 = x1;
        x1 = tmp;
    }
    if (y0 > y1) {
        const int tmp = y0;
        y0 = y1;
        y1 = tmp;
    }
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

void fillOval(uint16_t* framebuffer, int width, int height,
              int cx, int cy, int rx, int ry, uint16_t color) {
    for (int dy = -ry; dy <= ry; ++dy) {
        for (int dx = -rx; dx <= rx; ++dx) {
            const float nx = static_cast<float>(dx) / static_cast<float>(rx);
            const float ny = static_cast<float>(dy) / static_cast<float>(ry);
            if (nx * nx + ny * ny <= 1.0f) {
                putPixel(framebuffer, width, height, cx + dx, cy + dy, color);
            }
        }
    }
}

void drawRivet(uint16_t* framebuffer, int width, int height, int x, int y) {
    fillDoorRect(framebuffer, width, height, x, x + 8, y, y + 8, kDoorRivet);
    fillOval(framebuffer, width, height, x + 2, y + 2, 3, 2, kDoorRivetHi);
}

void drawDoorDiagonalStreak(uint16_t* framebuffer, int width, int height,
                            int x0, int x1, int yStart, int thickness,
                            uint16_t color) {
    const int panelW = x1 - x0;
    if (panelW <= 2) {
        return;
    }

    const int halfT = thickness / 2;
    for (int i = 0; i <= panelW; ++i) {
        const int cx = x0 + i;
        const int cy = yStart + i;
        for (int t = -halfT; t <= halfT; ++t) {
            const int px = cx + t;
            const int py = cy - t;
            if (px >= x0 && px < x1 && py >= 0 && py < height) {
                putPixel(framebuffer, width, height, px, py, color);
            }
        }
    }
}

void drawDoorPanel(uint16_t* framebuffer, int width, int height,
                   int x0, int x1, bool isLeftDoor) {
    if (x1 <= x0) {
        return;
    }

    fillDoorRect(framebuffer, width, height, x0, x1, 0, height, kDoorGrey);

    drawDoorDiagonalStreak(framebuffer, width, height, x0, x1, 18, 7,
                           kDoorStreakLarge);
    drawDoorDiagonalStreak(framebuffer, width, height, x0, x1, 72, 3,
                           kDoorStreakSmall);

    const int seamX0 = isLeftDoor ? x1 - 3 : x0;
    const int seamX1 = isLeftDoor ? x1 : x0 + 3;
    fillDoorRect(framebuffer, width, height, seamX0, seamX1, 0, height, kDoorSeam);

    constexpr int kRivetH = 8;
    constexpr int kRivetSpacing = 32;
    const int rivetX = isLeftDoor ? x1 - 14 : x0 + 10;
    for (int y = 0; y + kRivetH <= height; y += kRivetSpacing) {
        drawRivet(framebuffer, width, height, rivetX, y);
    }
}

} // namespace

float menuDoorTransitionDuration() {
    return kMenuDoorCloseSec + kMenuDoorPauseSec + kMenuDoorOpenSec;
}

float menuDoorGameplayStartTime() {
    return kMenuDoorCloseSec + kMenuDoorPauseSec;
}

MenuDoorAnim menuDoorAnimForTime(float elapsedSec) {
    MenuDoorAnim anim;
    const float closeEnd = kMenuDoorCloseSec;
    const float pauseEnd = closeEnd + kMenuDoorPauseSec;
    const float total = menuDoorTransitionDuration();

    if (elapsedSec >= total) {
        anim.showGameplay = true;
        return anim;
    }

    if (elapsedSec < closeEnd) {
        anim.cover = smoothStep(elapsedSec / closeEnd);
    } else if (elapsedSec < pauseEnd) {
        anim.cover = 1.0f;
    } else {
        const float openT = smoothStep((elapsedSec - pauseEnd) / kMenuDoorOpenSec);
        anim.cover = 1.0f - openT;
        anim.showGameplay = true;
    }

    return anim;
}

void drawMenuDoors(uint16_t* framebuffer, int width, int height,
                   const MenuDoorAnim& anim) {
    if (anim.cover <= 0.001f) {
        return;
    }

    const int cx = width / 2;
    const int halfCover =
        static_cast<int>(lroundf(static_cast<float>(cx) * anim.cover));
    if (halfCover <= 0) {
        return;
    }

    drawDoorPanel(framebuffer, width, height, 0, halfCover, true);
    drawDoorPanel(framebuffer, width, height, width - halfCover, width, false);
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
