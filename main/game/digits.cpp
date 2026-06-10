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
constexpr float kShieldArcGap = 4.0f;
constexpr float kShieldArcThickness = 5.0f;
constexpr float kAbilityIconGap = 7.0f;
/// Shield ring is thin — skip the inner outline inset to save per-pixel work.
constexpr float kShieldFillOutlinePx = 0.0f;
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

struct Annulus2 {
    float innerR = 0.0f;
    float outerR = 0.0f;
    float innerR2 = 0.0f;
    float outerR2 = 0.0f;
};

Annulus2 makeAnnulus(float outerR, float thickness) {
    const float outerRBg = outerR + kOuterBleed;
    const float innerRBg = outerRBg - thickness;
    Annulus2 ring;
    ring.outerR = outerRBg;
    ring.innerR = innerRBg;
    ring.innerR2 = (innerRBg - 0.5f) * (innerRBg - 0.5f);
    ring.outerR2 = (outerRBg + 0.5f) * (outerRBg + 0.5f);
    return ring;
}

Annulus2 makeFillAnnulus(const Annulus2& bgRing, float fillOutlinePx) {
    Annulus2 ring;
    ring.outerR = bgRing.outerR - fillOutlinePx;
    ring.innerR = bgRing.innerR + fillOutlinePx;
    ring.innerR2 = (ring.innerR - 0.5f) * (ring.innerR - 0.5f);
    ring.outerR2 = (ring.outerR + 0.5f) * (ring.outerR + 0.5f);
    return ring;
}

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

bool inAnnulus(float dist2, const Annulus2& ring) {
    return dist2 >= ring.innerR2 && dist2 <= ring.outerR2;
}

const ArcBoundary* makeFillBoundary(float fillSpanDeg, float fillOutlinePx, float midR,
                                    ArcBoundary& storage) {
    if (fillSpanDeg <= 0.0f) {
        return nullptr;
    }

    const float capDeg =
        fillOutlinePx * (180.0f / static_cast<float>(M_PI)) / midR;
    const float fillStart = kArcStartDeg + capDeg;
    const float fillEnd = kArcStartDeg + fillSpanDeg - capDeg;
    const float shrunkSpan = fillEnd - fillStart;
    if (shrunkSpan <= 0.25f) {
        return nullptr;
    }

    storage = makeArcBoundary(fillStart, shrunkSpan);
    return &storage;
}

void drawHealthAndShieldArcsImpl(uint16_t* framebuffer, int screenWidth, int screenHeight,
                                 float cx, float cy, float displayR,
                                 float healthFillSpanDeg,
                                 float shieldFillSpanDeg,
                                 bool drawHealthLayer, bool showShield,
                                 uint16_t healthFillColor, uint16_t shieldFillColor,
                                 uint16_t bgColor) {
    const Annulus2 healthBg = makeAnnulus(displayR, kArcThickness);
    const Annulus2 healthFillRing = makeFillAnnulus(healthBg, kFillOutlinePx);
    const float healthMidR = (healthBg.innerR + healthBg.outerR) * 0.5f;

    Annulus2 shieldBg{};
    Annulus2 shieldFillRing{};
    float shieldMidR = 0.0f;
    if (showShield) {
        const float shieldOuterR = displayR - kArcThickness - kShieldArcGap;
        shieldBg = makeAnnulus(shieldOuterR, kShieldArcThickness);
        shieldFillRing = makeFillAnnulus(shieldBg, kShieldFillOutlinePx);
        shieldMidR = (shieldBg.innerR + shieldBg.outerR) * 0.5f;
    }

    const ArcBoundary fullBoundary = makeArcBoundary(kArcStartDeg, kArcSpanDeg);
    ArcBoundary healthFillBoundaryStorage;
    ArcBoundary shieldFillBoundaryStorage;
    const ArcBoundary* healthFillBoundary = nullptr;
    if (drawHealthLayer) {
        healthFillBoundary =
            makeFillBoundary(healthFillSpanDeg, kFillOutlinePx, healthMidR,
                             healthFillBoundaryStorage);
    }
    const ArcBoundary* shieldFillBoundary = nullptr;
    if (showShield) {
        shieldFillBoundary =
            makeFillBoundary(shieldFillSpanDeg, kShieldFillOutlinePx, shieldMidR,
                             shieldFillBoundaryStorage);
    }

    const uint16_t packedHealthFill = fbPack(healthFillColor);
    const uint16_t packedShieldFill = fbPack(shieldFillColor);
    const uint16_t packedBg = fbPack(bgColor);

    const float loopOuterR = healthBg.outerR;
    const int xMin = static_cast<int>(floorf(cx - loopOuterR - 1.0f));
    const int xMax = static_cast<int>(ceilf(cx + loopOuterR + 1.0f));
    const int yMin = static_cast<int>(floorf(cy - loopOuterR - 1.0f));
    const int yMax = static_cast<int>(ceilf(cy));

    for (int py = yMin; py <= yMax; ++py) {
        if (py < 0 || py >= screenHeight) {
            continue;
        }

        const int row = py * screenWidth;
        for (int px = xMin; px <= xMax; ++px) {
            if (px < 0 || px >= screenWidth) {
                continue;
            }

            const float sx = static_cast<float>(px) + 0.5f;
            const float sy = static_cast<float>(py) + 0.5f;
            const float dx = sx - cx;
            const float dy = sy - cy;
            const float dist2 = dx * dx + dy * dy;

            const bool inHealthBg = drawHealthLayer && inAnnulus(dist2, healthBg);
            const bool inShieldBg = showShield && inAnnulus(dist2, shieldBg);
            if (!inHealthBg && !inShieldBg) {
                continue;
            }
            if (!inArcSweep(dx, dy, fullBoundary)) {
                continue;
            }

            if (inHealthBg) {
                const bool inFill = healthFillBoundary != nullptr
                    && healthFillRing.innerR < healthFillRing.outerR
                    && inAnnulus(dist2, healthFillRing)
                    && inArcSweep(dx, dy, *healthFillBoundary);
                framebuffer[row + px] = inFill ? packedHealthFill : packedBg;
                continue;
            }

            const bool inFill = shieldFillBoundary != nullptr
                && shieldFillRing.innerR < shieldFillRing.outerR
                && inAnnulus(dist2, shieldFillRing)
                && inArcSweep(dx, dy, *shieldFillBoundary);
            framebuffer[row + px] = inFill ? packedShieldFill : packedBg;
        }
    }
}

int clampRatioValue(int value, int maxValue) {
    if (maxValue < 1) {
        maxValue = 1;
    }
    if (value < 0) {
        return 0;
    }
    if (value > maxValue) {
        return maxValue;
    }
    return value;
}

} // namespace

void Digits::drawHealthAndShieldArcs(uint16_t* framebuffer, int screenWidth, int screenHeight,
                                     int health, int maxHealth,
                                     uint16_t healthFillColor, uint16_t bgColor,
                                     bool showShield, int shieldHp, int maxShieldHp,
                                     uint16_t shieldFillColor) {
    health = clampRatioValue(health, maxHealth);
    shieldHp = clampRatioValue(shieldHp, maxShieldHp);

    const float cx = screenCenterX(screenWidth);
    const float cy = screenCenterY(screenHeight);
    const float displayR = displayRadius(screenWidth, screenHeight);
    const float healthFillSpan =
        kArcSpanDeg * (static_cast<float>(health) / static_cast<float>(maxHealth));
    const float shieldFillSpan = showShield
        ? kArcSpanDeg * (static_cast<float>(shieldHp) / static_cast<float>(maxShieldHp))
        : 0.0f;

    drawHealthAndShieldArcsImpl(framebuffer, screenWidth, screenHeight,
                                cx, cy, displayR,
                                healthFillSpan, shieldFillSpan,
                                true, showShield,
                                healthFillColor, shieldFillColor, bgColor);
}

void Digits::drawHealthArc(uint16_t* framebuffer, int screenWidth, int screenHeight,
                           int health, int maxHealth,
                           uint16_t fillColor, uint16_t bgColor) {
    drawHealthAndShieldArcs(framebuffer, screenWidth, screenHeight,
                            health, maxHealth, fillColor, bgColor,
                            false, 0, 1, fillColor);
}

void Digits::drawShieldArc(uint16_t* framebuffer, int screenWidth, int screenHeight,
                           int shieldHp, int maxShieldHp,
                           uint16_t fillColor, uint16_t bgColor) {
    shieldHp = clampRatioValue(shieldHp, maxShieldHp);

    const float cx = screenCenterX(screenWidth);
    const float cy = screenCenterY(screenHeight);
    const float displayR = displayRadius(screenWidth, screenHeight);
    const float shieldFillSpan =
        kArcSpanDeg * (static_cast<float>(shieldHp) / static_cast<float>(maxShieldHp));

    drawHealthAndShieldArcsImpl(framebuffer, screenWidth, screenHeight,
                                cx, cy, displayR,
                                0.0f, shieldFillSpan,
                                false, true,
                                fillColor, fillColor, bgColor);
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
