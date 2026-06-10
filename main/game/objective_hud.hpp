#pragma once

#include "Camera.hpp"
#include <cstdint>

namespace Game {

class ObjectiveHud {
public:
    static void drawArrow(uint16_t* framebuffer, int screenWidth, int screenHeight,
                          const Renderer::Camera& camera,
                          float playerX, float playerZ, float playerAngle,
                          float targetX, float targetY, float targetZ,
                          uint16_t color);
};

} // namespace Game
