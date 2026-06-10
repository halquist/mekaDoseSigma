#include "digits.hpp"
#include "FramebufferIO.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Game {

namespace {

constexpr float kArcThickness = 8.0f;
constexpr float kFillOutlinePx = 2.0f;
/// Slight outward bleed so the band fully covers the bezel (avoids sky gaps).
constexpr float kOuterBleed = 1.5f;
/// 160° arc centered on 12 o'clock (280° → 80°).
constexpr float kArcSpanDeg = 160.0f;
constexpr float kArcStartDeg = 360.0f - kArcSpanDeg * 0.5f;

float screenCenterX(int screenWidth) {
    return static_cast<float>(screenWidth) * 0.5f;
}

float screenCenterY(int screenHeight) {
    return static_cast<float>(screenHeight) * 0.5f;
}

float displayRadius(int screenWidth, int screenHeight) {
    const float rw = static_cast<float>(screenWidth) * 0.5f;
    const float rh = static_cast<float>(screenHeight) * 0.5f;
    return (rw < rh) ? rw : rh;
}

struct ArcBoundary {
    float startX;
    float startY;
    float endX;
    float endY;
};

ArcBoundary makeArcBoundary(float startDeg, float spanDeg) {
    const float startRad = startDeg * static_cast<float>(M_PI) / 180.0f;
    const float endRad = (startDeg + spanDeg) * static_cast<float>(M_PI) / 180.0f;
    return {
        sinf(startRad), -cosf(startRad),
        sinf(endRad), -cosf(endRad),
    };
}

bool inArcSweep(float dx, float dy, const ArcBoundary& boundary) {
    const float crossStart = boundary.startX * dy - boundary.startY * dx;
    if (crossStart < 0.0f) {
        return false;
    }
    const float crossEnd = dx * boundary.endY - dy * boundary.endX;
    return crossEnd >= 0.0f;
}

bool inAnnulus(float dist2, float innerR, float outerR) {
    const float innerR2 = (innerR - 0.5f) * (innerR - 0.5f);
    const float outerR2 = (outerR + 0.5f) * (outerR + 0.5f);
    return dist2 >= innerR2 && dist2 <= outerR2;
}

void drawHealthArcSinglePass(uint16_t* framebuffer, int screenWidth, int screenHeight,
                             float cx, float cy, float outerR, float thickness,
                             const ArcBoundary& fullBoundary,
                             float fillSpanDeg,
                             uint16_t fillColor, uint16_t bgColor) {
    const float outerRBg = outerR + kOuterBleed;
    const float innerRBg = outerRBg - thickness;
    const float outerRFill = outerRBg - kFillOutlinePx;
    const float innerRFill = innerRBg + kFillOutlinePx;
    const float midR = (innerRBg + outerRBg) * 0.5f;
    const float capDeg =
        kFillOutlinePx * (180.0f / static_cast<float>(M_PI)) / midR;
    const uint16_t packedFill = fbPack(fillColor);
    const uint16_t packedBg = fbPack(bgColor);

    ArcBoundary fillBoundaryStorage;
    const ArcBoundary* fillBoundary = nullptr;
    if (fillSpanDeg > 0.0f) {
        const float fillStart = kArcStartDeg + capDeg;
        const float fillEnd = kArcStartDeg + fillSpanDeg - capDeg;
        const float shrunkSpan = fillEnd - fillStart;
        if (shrunkSpan > 0.25f) {
            fillBoundaryStorage = makeArcBoundary(fillStart, shrunkSpan);
            fillBoundary = &fillBoundaryStorage;
        }
    }

    const int xMin = static_cast<int>(floorf(cx - outerRBg - 1.0f));
    const int xMax = static_cast<int>(ceilf(cx + outerRBg + 1.0f));
    const int yMin = static_cast<int>(floorf(cy - outerRBg - 1.0f));
    const int yMax = static_cast<int>(ceilf(cy));

    for (int py = yMin; py <= yMax; ++py) {
        if (py < 0 || py >= screenHeight) continue;
        for (int px = xMin; px <= xMax; ++px) {
            if (px < 0 || px >= screenWidth) continue;

            const float sx = static_cast<float>(px) + 0.5f;
            const float sy = static_cast<float>(py) + 0.5f;
            const float dx = sx - cx;
            const float dy = sy - cy;
            const float dist2 = dx * dx + dy * dy;

            if (!inAnnulus(dist2, innerRBg, outerRBg)) continue;
            if (!inArcSweep(dx, dy, fullBoundary)) continue;

            const bool inFill = fillBoundary != nullptr
                && innerRFill < outerRFill
                && inAnnulus(dist2, innerRFill, outerRFill)
                && inArcSweep(dx, dy, *fillBoundary);
            framebuffer[py * screenWidth + px] = inFill ? packedFill : packedBg;
        }
    }
}

} // namespace

void Digits::drawHealthArc(uint16_t* framebuffer, int screenWidth, int screenHeight,
                           int health, int maxHealth,
                           uint16_t fillColor, uint16_t bgColor) {
    if (maxHealth < 1) {
        maxHealth = 1;
    }
    if (health < 0) {
        health = 0;
    } else if (health > maxHealth) {
        health = maxHealth;
    }

    const float cx = screenCenterX(screenWidth);
    const float cy = screenCenterY(screenHeight);
    const float displayR = displayRadius(screenWidth, screenHeight);
    const float fillSpan =
        kArcSpanDeg * (static_cast<float>(health) / static_cast<float>(maxHealth));

    const ArcBoundary fullBoundary = makeArcBoundary(kArcStartDeg, kArcSpanDeg);
    drawHealthArcSinglePass(framebuffer, screenWidth, screenHeight,
                            cx, cy, displayR, kArcThickness, fullBoundary, fillSpan,
                            fillColor, bgColor);
}

constexpr float kShieldArcGap = 4.0f;
constexpr float kShieldArcThickness = 5.0f;
constexpr float kAbilityIconGap = 7.0f;

void Digits::drawShieldArc(uint16_t* framebuffer, int screenWidth, int screenHeight,
                           int shieldHp, int maxShieldHp,
                           uint16_t fillColor, uint16_t bgColor) {
    if (maxShieldHp < 1) {
        maxShieldHp = 1;
    }
    if (shieldHp < 0) {
        shieldHp = 0;
    } else if (shieldHp > maxShieldHp) {
        shieldHp = maxShieldHp;
    }

    const float cx = screenCenterX(screenWidth);
    const float cy = screenCenterY(screenHeight);
    const float displayR = displayRadius(screenWidth, screenHeight);
    const float outerR = displayR - kArcThickness - kShieldArcGap;
    const float fillSpan =
        kArcSpanDeg * (static_cast<float>(shieldHp) / static_cast<float>(maxShieldHp));

    const ArcBoundary fullBoundary = makeArcBoundary(kArcStartDeg, kArcSpanDeg);
    drawHealthArcSinglePass(framebuffer, screenWidth, screenHeight,
                            cx, cy, outerR, kShieldArcThickness, fullBoundary, fillSpan,
                            fillColor, bgColor);
}

void Digits::drawAbilityReadyIcon(uint16_t* framebuffer, int screenWidth, int screenHeight,
                                  uint16_t color) {
    const float cx = screenCenterX(screenWidth);
    const float cy = screenCenterY(screenHeight);
    const float displayR = displayRadius(screenWidth, screenHeight);
    const float iconCenterY =
        cy - (displayR - kArcThickness - kShieldArcGap - kShieldArcThickness - kAbilityIconGap);
    const int icx = static_cast<int>(lroundf(cx));
    const int icy = static_cast<int>(lroundf(iconCenterY));
    const uint16_t packed = fbPack(color);

    constexpr int halfW = 5;
    constexpr int height = 8;

    for (int dy = -height / 2; dy <= height / 2; ++dy) {
        const int py = icy + dy;
        if (py < 0 || py >= screenHeight) {
            continue;
        }

        const int distFromBase = height / 2 - dy;
        const int rowHalfW = (distFromBase * halfW) / height;
        if (rowHalfW <= 0) {
            continue;
        }

        for (int dx = -rowHalfW; dx <= rowHalfW; ++dx) {
            const int px = icx + dx;
            if (px < 0 || px >= screenWidth) {
                continue;
            }
            framebuffer[py * screenWidth + px] = packed;
        }
    }
}

} // namespace Game
