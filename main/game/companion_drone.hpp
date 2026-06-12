#pragma once

#include "Jet.hpp"
#include "types.hpp"
#include <cstdint>

namespace Game {

class ProjectileSystem;

class CompanionDrone {
public:
    explicit CompanionDrone(Renderer::Scene& scene);

    void reset();
    void update(float deltaTime, float mechX, float mechZ, float mechBaseY, bool active);
    bool tryFire(ProjectileSystem& projectiles, float targetX, float targetZ,
                 float targetAimY, uint8_t droneTier);

private:
    void hide();
    void syncVisual();

    Renderer::Scene& m_scene;
    Renderer::Object* m_body = nullptr;
    Renderer::Material m_bodyMat;

    float m_orbitAngle = 0.0f;
    float m_fireCooldown = 0.0f;
    float m_x = 0.0f;
    float m_z = 0.0f;
    float m_y = 0.0f;
    bool m_active = false;

    static constexpr float ORBIT_RADIUS = 48.0f;
    static constexpr float ORBIT_HEIGHT = 38.0f;
    static constexpr float ORBIT_SPEED = 1.8f;
};

} // namespace Game
