#include "digits.hpp"
#include "FramebufferIO.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Game {

namespace {

constexpr float kDisplayRadius = 120.0f;
constexpr float kArcRadius = 110.0f;
constexpr float kArcThickness = 8.0f;
/// Upper semicircle: 9 o'clock → 12 → 3 o'clock (clockwise from 12 o'clock).
constexpr float kArcStartDeg = 270.0f;
constexpr float kArcSpanDeg = 180.0f;

float screenCenterX(int screenWidth) {
    return static_cast<float>(screenWidth) * 0.5f;
}

float screenCenterY(int screenHeight) {
    return static_cast<float>(screenHeight) * 0.5f;
}

bool insideRoundDisplay(float x, float y, float cx, float cy) {
    const float dx = x - cx;
    const float dy = y - cy;
    return dx * dx + dy * dy <= kDisplayRadius * kDisplayRadius;
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

void drawHealthArcSinglePass(uint16_t* framebuffer, int screenWidth, int screenHeight,
                             float cx, float cy,
                             const ArcBoundary& fullBoundary,
                             const ArcBoundary* fillBoundary,
                             uint16_t fillColor, uint16_t bgColor) {
    const float innerR = kArcRadius - kArcThickness;
    const float outerR = kArcRadius;
    const float innerR2 = innerR * innerR;
    const float outerR2 = outerR * outerR;
    const uint16_t packedFill = fbPack(fillColor);
    const uint16_t packedBg = fbPack(bgColor);

    const int xMin = static_cast<int>(floorf(cx - outerR));
    const int xMax = static_cast<int>(ceilf(cx + outerR));
    const int yMin = static_cast<int>(floorf(cy - outerR));
    const int yMax = static_cast<int>(ceilf(cy));

    for (int py = yMin; py <= yMax; ++py) {
        if (py < 0 || py >= screenHeight) continue;
        for (int px = xMin; px <= xMax; ++px) {
            if (px < 0 || px >= screenWidth) continue;

            const float sx = static_cast<float>(px) + 0.5f;
            const float sy = static_cast<float>(py) + 0.5f;
            if (!insideRoundDisplay(sx, sy, cx, cy)) continue;

            const float dx = sx - cx;
            const float dy = sy - cy;
            const float dist2 = dx * dx + dy * dy;
            if (dist2 < innerR2 || dist2 > outerR2) continue;
            if (!inArcSweep(dx, dy, fullBoundary)) continue;

            const bool inFill =
                fillBoundary != nullptr && inArcSweep(dx, dy, *fillBoundary);
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
    const float fillSpan =
        kArcSpanDeg * (static_cast<float>(health) / static_cast<float>(maxHealth));

    const ArcBoundary fullBoundary = makeArcBoundary(kArcStartDeg, kArcSpanDeg);
    if (fillSpan > 0.0f) {
        const ArcBoundary fillBoundary = makeArcBoundary(kArcStartDeg, fillSpan);
        drawHealthArcSinglePass(framebuffer, screenWidth, screenHeight,
                                cx, cy, fullBoundary, &fillBoundary,
                                fillColor, bgColor);
    } else {
        drawHealthArcSinglePass(framebuffer, screenWidth, screenHeight,
                                cx, cy, fullBoundary, nullptr,
                                fillColor, bgColor);
    }
}

} // namespace Game
