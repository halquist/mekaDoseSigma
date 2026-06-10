#include "objective_hud.hpp"
#include "Camera.hpp"
#include "FramebufferIO.hpp"
#include "Shader.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Game {

namespace {

constexpr float kHideDistance = 400.0f;
constexpr float kHideFacingCos = 0.766f; // cos(40°)
constexpr float kEdgeInset = 22.0f;      // px from round bezel to arrow anchor
constexpr float kDisplayRadius = 120.0f; // 240px round panel
constexpr float kArrowTipLen = 8.0f;
constexpr float kArrowBaseLen = 5.0f;
constexpr float kArrowHalfWidth = 4.0f;

float screenCenterX(int screenWidth) {
    return static_cast<float>(screenWidth) * 0.5f;
}

float screenCenterY(int screenHeight) {
    return static_cast<float>(screenHeight) * 0.5f;
}

float ringRadius() {
    return kDisplayRadius - kEdgeInset;
}

bool insideRoundDisplay(float x, float y, float cx, float cy) {
    const float dx = x - cx;
    const float dy = y - cy;
    return dx * dx + dy * dy <= kDisplayRadius * kDisplayRadius;
}

void setPixel(uint16_t* framebuffer, int screenWidth, int screenHeight,
              int x, int y, uint16_t color, float cx, float cy) {
    if (x < 0 || x >= screenWidth || y < 0 || y >= screenHeight) return;
    if (!insideRoundDisplay(static_cast<float>(x) + 0.5f,
                            static_cast<float>(y) + 0.5f, cx, cy)) {
        return;
    }
    framebuffer[y * screenWidth + x] = fbPack(color);
}

void fillTriangle(uint16_t* framebuffer, int screenWidth, int screenHeight,
                  float x0, float y0, float x1, float y1, float x2, float y2,
                  uint16_t color, float cx, float cy) {
    if (y0 > y1) { float t; t=x0; x0=x1; x1=t; t=y0; y0=y1; y1=t; }
    if (y0 > y2) { float t; t=x0; x0=x2; x2=t; t=y0; y0=y2; y2=t; }
    if (y1 > y2) { float t; t=x1; x1=x2; x2=t; t=y1; y1=y2; y2=t; }

    const int yStart = static_cast<int>(y0);
    const int yEnd = static_cast<int>(y2);
    if (yEnd < 0 || yStart >= screenHeight) return;

    for (int y = yStart; y <= yEnd; ++y) {
        if (y < 0) continue;
        float t = (y2 - y0) > 0.01f ? (static_cast<float>(y) - y0) / (y2 - y0) : 0.0f;
        float lx = x0 + (x2 - x0) * t;
        float rx = (y1 - y0) > 0.01f
            ? x0 + (x1 - x0) * (static_cast<float>(y) - y0) / (y1 - y0)
            : x1;
        if (y > y1) {
            t = (y2 - y1) > 0.01f ? (static_cast<float>(y) - y1) / (y2 - y1) : 0.0f;
            rx = x1 + (x2 - x1) * t;
        }
        if (lx > rx) { float s = lx; lx = rx; rx = s; }
        const int xMin = static_cast<int>(lx);
        const int xMax = static_cast<int>(rx);
        for (int x = xMin; x <= xMax; ++x) {
            setPixel(framebuffer, screenWidth, screenHeight, x, y, color, cx, cy);
        }
    }
}

Vector3 horizontalViewOffset(const Renderer::Camera& camera,
                             float worldDx, float worldDz) {
    return camera.transformDirection(
        {static_cast<int32_t>(worldDx),
         0,
         static_cast<int32_t>(worldDz)});
}

float cameraBearingToTarget(const Renderer::Camera& camera,
                            float targetX, float targetZ) {
    const float dx = targetX - static_cast<float>(camera.position.x);
    const float dz = targetZ - static_cast<float>(camera.position.z);
    const Vector3 view = horizontalViewOffset(camera, dx, dz);
    // Camera view +Z is forward; map bearing to screen where 0 = up.
    return atan2f(static_cast<float>(view.x), static_cast<float>(view.z));
}

void bearingToRingPoint(float bearing, float cx, float cy,
                        float& outX, float& outY) {
    const float r = ringRadius();
    outX = cx + sinf(bearing) * r;
    outY = cy - cosf(bearing) * r;
}

void bearingToDirection(float bearing, float& dirX, float& dirY) {
    dirX = sinf(bearing);
    dirY = -cosf(bearing);
}

Vector3 worldOffsetToView(const Renderer::Camera& camera,
                          float dx, float dy, float dz) {
    return camera.transformDirection(
        {static_cast<int32_t>(dx),
         static_cast<int32_t>(dy),
         static_cast<int32_t>(dz)});
}

void projectToScreen(const Renderer::Camera& camera,
                     int screenWidth, int screenHeight,
                     float dx, float dy, float dz,
                     float& outScreenX, float& outScreenY, bool& outInFront) {
    Vector3 viewDir = worldOffsetToView(camera, dx, dy, dz);
    outInFront = viewDir.z > 0;
    if (!outInFront) {
        return;
    }

    const float invZ = camera.fovFactor / static_cast<float>(viewDir.z);
    outScreenX = screenWidth * 0.5f + static_cast<float>(viewDir.x) * invZ;
    outScreenY = screenHeight * 0.5f - static_cast<float>(viewDir.y) * invZ;
}

bool isInsidePlayCircle(float sx, float sy, float cx, float cy) {
    const float dx = sx - cx;
    const float dy = sy - cy;
    const float innerR = ringRadius() - 10.0f;
    return dx * dx + dy * dy <= innerR * innerR;
}

bool shouldHideArrow(float playerX, float playerZ, float playerAngle,
                     float targetX, float targetZ,
                     const Renderer::Camera& camera,
                     int screenWidth, int screenHeight,
                     float targetY) {
    const float relX = targetX - playerX;
    const float relZ = targetZ - playerZ;
    const float dist = sqrtf(relX * relX + relZ * relZ);
    if (dist >= kHideDistance) {
        return false;
    }

    const float radians = playerAngle * static_cast<float>(M_PI) / 180.0f;
    const float fwdX = sinf(radians);
    const float fwdZ = cosf(radians);
    const float facingDot = (relX * fwdX + relZ * fwdZ) / fmaxf(dist, 0.001f);
    if (facingDot < kHideFacingCos) {
        return false;
    }

    const float dx = targetX - static_cast<float>(camera.position.x);
    const float dy = targetY - static_cast<float>(camera.position.y);
    const float dz = targetZ - static_cast<float>(camera.position.z);

    float sx = 0.0f;
    float sy = 0.0f;
    bool inFront = false;
    projectToScreen(camera, screenWidth, screenHeight, dx, dy, dz, sx, sy, inFront);
    if (!inFront) {
        return false;
    }

    const float cx = screenCenterX(screenWidth);
    const float cy = screenCenterY(screenHeight);
    return isInsidePlayCircle(sx, sy, cx, cy);
}

} // namespace

void ObjectiveHud::drawArrow(uint16_t* framebuffer, int screenWidth, int screenHeight,
                             const Renderer::Camera& camera,
                             float playerX, float playerZ, float playerAngle,
                             float targetX, float targetY, float targetZ,
                             uint16_t color) {
    if (shouldHideArrow(playerX, playerZ, playerAngle,
                        targetX, targetZ, camera,
                        screenWidth, screenHeight, targetY)) {
        return;
    }

    const float cx = screenCenterX(screenWidth);
    const float cy = screenCenterY(screenHeight);
    const float bearing = cameraBearingToTarget(camera, targetX, targetZ);

    float edgeX = 0.0f;
    float edgeY = 0.0f;
    bearingToRingPoint(bearing, cx, cy, edgeX, edgeY);

    float dirX = 0.0f;
    float dirY = 0.0f;
    bearingToDirection(bearing, dirX, dirY);

    const float tipX = edgeX + dirX * kArrowTipLen;
    const float tipY = edgeY + dirY * kArrowTipLen;
    const float baseX = edgeX - dirX * kArrowBaseLen;
    const float baseY = edgeY - dirY * kArrowBaseLen;
    const float perpX = -dirY * kArrowHalfWidth;
    const float perpY = dirX * kArrowHalfWidth;

    fillTriangle(framebuffer, screenWidth, screenHeight,
                 tipX, tipY,
                 baseX + perpX, baseY + perpY,
                 baseX - perpX, baseY - perpY,
                 color, cx, cy);
}

} // namespace Game
