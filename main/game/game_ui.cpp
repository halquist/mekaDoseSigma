#include "game_ui.hpp"
#include "font.hpp"
#include "FramebufferIO.hpp"
#include "rng.hpp"
#include <cmath>
#include <cstdio>

namespace Game {
namespace GameUi {

namespace {

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

// ---------------------------------------------------------------------------
// Upgrade pick animation helpers
// ---------------------------------------------------------------------------

constexpr float kUpgradeSideAnimSec   = 0.40f;
constexpr float kUpgradeTabStartSec   = 0.30f;
constexpr float kUpgradeTabAnimSec    = 0.35f;
constexpr float kUpgradeInvertHoldSec = 0.50f;
constexpr float kUpgradeExitSec       = 0.50f;

constexpr int kUpgradeSkipTabH  = 48;
constexpr int kUpgradeScoreTabH = 48;
constexpr int kTabCutSize       = 18;
constexpr int kTabEdgeThick     = 2;

uint16_t colorDarken(uint16_t c, float f) {
    const float s = 1.0f - f;
    const uint32_t r = static_cast<uint32_t>(static_cast<float>((c >> 11) & 0x1Fu) * s);
    const uint32_t g = static_cast<uint32_t>(static_cast<float>((c >> 5) & 0x3Fu) * s);
    const uint32_t b = static_cast<uint32_t>(static_cast<float>(c & 0x1Fu) * s);
    return static_cast<uint16_t>((r << 11) | (g << 5) | b);
}

uint16_t colorLighten(uint16_t c, float f) {
    const uint32_t r = (c >> 11) & 0x1Fu;
    const uint32_t g = (c >> 5) & 0x3Fu;
    const uint32_t b = c & 0x1Fu;
    const uint32_t ro = static_cast<uint32_t>(
        static_cast<float>(r) + static_cast<float>(0x1Fu - r) * f);
    const uint32_t go = static_cast<uint32_t>(
        static_cast<float>(g) + static_cast<float>(0x3Fu - g) * f);
    const uint32_t bo = static_cast<uint32_t>(
        static_cast<float>(b) + static_cast<float>(0x1Fu - b) * f);
    return static_cast<uint16_t>((ro << 11) | (go << 5) | bo);
}

void drawEmbossText(uint16_t* framebuffer, int width, int height,
                    const char* text, int cx, int cy, int scale,
                    uint16_t baseColor) {
    const uint16_t mainColor = colorDarken(baseColor, 0.58f);
    const uint16_t hiColor   = colorLighten(baseColor, 0.45f);
    const uint16_t shColor   = colorDarken(baseColor, 0.82f);

    Font::drawTextCentered(framebuffer, width, height, text,
                           cx + 1, cy - 1, scale, shColor);
    Font::drawTextCentered(framebuffer, width, height, text,
                           cx - 1, cy + 1, scale, hiColor);
    Font::drawTextCentered(framebuffer, width, height, text,
                           cx, cy, scale, mainColor);
}

void drawEmbossNumber(uint16_t* framebuffer, int width, int height,
                      int number, int cx, int cy, int scale,
                      uint16_t baseColor) {
    const uint16_t mainColor = colorDarken(baseColor, 0.58f);
    const uint16_t hiColor   = colorLighten(baseColor, 0.45f);
    const uint16_t shColor   = colorDarken(baseColor, 0.82f);

    Font::drawNumber(framebuffer, width, height, number,
                     cx + 1, cy - 1, scale, shColor);
    Font::drawNumber(framebuffer, width, height, number,
                     cx - 1, cy + 1, scale, hiColor);
    Font::drawNumber(framebuffer, width, height, number,
                     cx, cy, scale, mainColor);
}

void drawDiagonalStreakClipped(uint16_t* framebuffer, int width, int height,
                               int x0, int x1, int y0, int y1,
                               int yStart, int thickness, uint16_t color) {
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
            if (px >= x0 && px < x1 && py >= y0 && py < y1) {
                putPixel(framebuffer, width, height, px, py, color);
            }
        }
    }
}

void drawUpgradePanel(uint16_t* framebuffer, int width, int height,
                      int x0, int x1, int y0, int y1,
                      const UpgradeOption& option, bool isLeft,
                      bool inverted) {
    if (x1 <= x0 || y1 <= y0) {
        return;
    }

    const uint16_t baseColor  = inverted
        ? colorDarken(option.color, 0.60f) : option.color;
    const uint16_t edgeColor  = inverted
        ? option.color : colorDarken(option.color, 0.45f);

    fillDoorRect(framebuffer, width, height, x0, x1, y0, y1, baseColor);

    if (!inverted) {
        drawDiagonalStreakClipped(framebuffer, width, height, x0, x1, y0, y1,
                                  y0 + 18, 7, colorLighten(option.color, 0.25f));
        drawDiagonalStreakClipped(framebuffer, width, height, x0, x1, y0, y1,
                                  y0 + 72, 3, colorLighten(option.color, 0.12f));
    }

    const int edgeX0 = isLeft ? x1 - 3 : x0;
    const int edgeX1 = isLeft ? x1     : x0 + 3;
    fillDoorRect(framebuffer, width, height, edgeX0, edgeX1, y0, y1, edgeColor);

    const int cx    = (x0 + x1) / 2;
    const int panCy = (y0 + y1) / 2;
    const uint16_t textBase = baseColor;

    drawEmbossText(framebuffer, width, height, option.title,
                   cx, panCy - 18, 2, textBase);
    if (option.subtitle[0] != '\0') {
        drawEmbossText(framebuffer, width, height, option.subtitle,
                       cx, panCy + 2, 2, textBase);
        if (option.tierLabel[0] != '\0') {
            drawEmbossText(framebuffer, width, height, option.tierLabel,
                           cx, panCy + 22, 2, textBase);
        }
    } else if (option.tierLabel[0] != '\0') {
        drawEmbossText(framebuffer, width, height, option.tierLabel,
                       cx, panCy + 6, 2, textBase);
    }
}

void circleRowBounds(int y, int width, int height, int& outMin, int& outMax) {
    const int cx = width / 2;
    const int cy = height / 2;
    const int radius = cx < cy ? cx : cy;
    const int dy = y - cy;
    const int dyAbs = dy < 0 ? -dy : dy;
    if (dyAbs >= radius) {
        outMin = cx;
        outMax = cx;
        return;
    }
    const int halfW = static_cast<int>(lroundf(sqrtf(
        static_cast<float>(radius * radius - dyAbs * dyAbs))));
    outMin = cx - halfW;
    outMax = cx + halfW;
}

void drawTabShape(uint16_t* framebuffer, int width, int height,
                  int tabY, int tabH, bool isTopTab,
                  uint16_t fillColor, uint16_t edgeColor) {
    const int y0 = tabY;
    const int y1 = tabY + tabH;

    // Chamfers anchor to the round-screen edge at the tab's free edge row.
    int freeVisMin = 0;
    int freeVisMax = width;
    const int freeY = isTopTab ? y1 - 1 : y0;
    if (freeY >= 0 && freeY < height) {
        circleRowBounds(freeY, width, height, freeVisMin, freeVisMax);
    }

    for (int y = y0; y < y1; ++y) {
        if (y < 0 || y >= height) {
            continue;
        }

        int xMin = 0;
        int xMax = width;

        if (isTopTab) {
            // SKIP: chamfer outside→inside, top of cut zone to free bottom edge
            const int d = (y1 - 1) - y;
            if (d >= 0 && d < kTabCutSize) {
                const int inset = kTabCutSize - 1 - d;
                xMin = freeVisMin + inset;
                xMax = freeVisMax - inset;
            }
        } else {
            // Score: chamfer outside→inside, bottom of cut zone to free top edge
            const int d = y - y0;
            if (d >= 0 && d < kTabCutSize) {
                const int inset = kTabCutSize - 1 - d;
                xMin = freeVisMin + inset;
                xMax = freeVisMax - inset;
            }
        }

        if (xMin < 0) {
            xMin = 0;
        }
        if (xMax > width) {
            xMax = width;
        }
        if (xMin >= xMax) {
            continue;
        }

        const bool isEdgeRow = isTopTab
            ? (y >= y1 - kTabEdgeThick)
            : (y < y0 + kTabEdgeThick);
        const uint16_t rowColor = isEdgeRow ? edgeColor : fillColor;
        const uint16_t packed   = fbPack(rowColor);
        uint16_t* row = framebuffer + y * width;
        for (int x = xMin; x < xMax; ++x) {
            row[x] = packed;
        }
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

UpgradePickAnim upgradePickAnimForTime(float entrySec, float confirmSec,
                                       int selectedChoice) {
    UpgradePickAnim anim;

    if (confirmSec <= 0.0f) {
        // Entry phase
        anim.sideCover = smoothStep(entrySec / kUpgradeSideAnimSec);
        const float tabProgress =
            (entrySec - kUpgradeTabStartSec) / kUpgradeTabAnimSec;
        anim.topCover = smoothStep(tabProgress);
        anim.botCover = anim.topCover;
        anim.invertChoice = -1;
    } else if (confirmSec > kUpgradeExitSec) {
        // Invert-hold phase
        anim.sideCover    = 1.0f;
        anim.topCover     = 1.0f;
        anim.botCover     = 1.0f;
        anim.invertChoice = selectedChoice;
    } else {
        // Exit phase
        const float exitT = smoothStep(1.0f - confirmSec / kUpgradeExitSec);
        anim.sideCover    = 1.0f - exitT;
        anim.topCover     = 1.0f - exitT;
        anim.botCover     = 1.0f - exitT;
        anim.invertChoice = selectedChoice;
    }

    return anim;
}

void drawUpgradePick(uint16_t* framebuffer, int width, int height,
                     const UpgradeOption& left, const UpgradeOption& right,
                     int selectedChoice, int score,
                     const UpgradePickAnim& anim) {
    (void)selectedChoice;

    const int cx        = width / 2;
    const int skipTabH  = kUpgradeSkipTabH;
    const int scoreTabH = kUpgradeScoreTabH;
    constexpr int panelY0 = 0;
    const int panelY1     = height;

    // Side panels — full height; top/bottom tabs draw on top
    if (anim.sideCover > 0.001f) {
        const int halfW = cx;
        const int slideOffset = static_cast<int>(lroundf(
            static_cast<float>(halfW) * (1.0f - anim.sideCover)));
        const int leftX0  = -slideOffset;
        const int leftX1  = halfW - slideOffset;
        const int rightX0 = width - halfW + slideOffset;
        const int rightX1 = width + slideOffset;

        if (leftX1 > 0) {
            drawUpgradePanel(framebuffer, width, height,
                             leftX0, leftX1, panelY0, panelY1,
                             left, true, anim.invertChoice == 0);
        }
        if (rightX0 < width) {
            drawUpgradePanel(framebuffer, width, height,
                             rightX0, rightX1, panelY0, panelY1,
                             right, false, anim.invertChoice == 1);
        }
    }

    // SKIP tab — slides from top
    if (anim.topCover > 0.001f) {
        const int tabY = static_cast<int>(lroundf(
            static_cast<float>(-skipTabH) * (1.0f - anim.topCover)));
        const bool skipInv = (anim.invertChoice == 2);
        const uint16_t skipFill = skipInv
            ? colorDarken(Colors::OBJECTIVE_ARROW, 0.5f)
            : Colors::OBJECTIVE_ARROW;
        const uint16_t skipEdge = colorDarken(Colors::OBJECTIVE_ARROW, 0.35f);
        drawTabShape(framebuffer, width, height,
                     tabY, skipTabH, true, skipFill, skipEdge);

        const int textCy = tabY + skipTabH / 2;
        drawEmbossText(framebuffer, width, height, "SKIP",
                       cx, textCy, 2, skipFill);
    }

    // Score tab — slides from bottom
    if (anim.botCover > 0.001f) {
        const int tabY = static_cast<int>(lroundf(
            static_cast<float>(height)
            - static_cast<float>(scoreTabH) * anim.botCover));
        constexpr uint16_t kScoreFill = Colors::rgb(50, 53, 58);
        constexpr uint16_t kScoreEdge = Colors::rgb(30, 32, 36);
        drawTabShape(framebuffer, width, height,
                     tabY, scoreTabH, false, kScoreFill, kScoreEdge);

        const int labelCy = tabY + 8;
        const int numCy   = tabY + 20;
        drawEmbossText(framebuffer, width, height, "SCORE",
                       cx, labelCy, 1, kScoreFill);
        drawEmbossNumber(framebuffer, width, height, score,
                         cx, numCy, 2, kScoreFill);
    }
}

int upgradePickFromTouch(int touchX, int touchY, int width, int height) {
    // Touch Y is flipped vs framebuffer (see input.cpp: y = 239 - raw).
    const int y = height - 1 - touchY;

    if (y < kUpgradeSkipTabH) {
        return 2;
    }
    if (y >= height - kUpgradeScoreTabH) {
        return -1;
    }

    return touchX < width / 2 ? 0 : 1;
}

} // namespace GameUi
} // namespace Game
