#pragma once

#include "Jet.hpp"
#include "types.hpp"
#include "map_config.hpp"
#include "projectile.hpp"

namespace Game {

class Enemy {
public:
    Enemy(Renderer::Scene& scene, ProjectileSystem& projectiles,
            const MapConfig& mapConfig);

    void update(float deltaTime, float playerX, float playerZ, float playerAngle);
    void reset(float playerX, float playerZ, float playerAngle, uint32_t spawnIndex = 0);

    float getX() const { return m_x; }
    float getZ() const { return m_z; }
    float getWidth() const { return 28.0f; }
    float getAimY() const;

    void takeDamage();
    bool isAlive() const { return m_health > 0; }

private:
    enum class AIState { PATROL, CHASE, ATTACK };
    enum class Locomotion { HOVER, GROUND };

    void spawnInRing(float playerX, float playerZ, uint32_t spawnIndex, AIState state);
    void hide();
    void hideAllParts();
    bool isBehindPlayer(float playerX, float playerZ, float playerAngle) const;
    void updateAI(float deltaTime, float playerX, float playerZ);
    void updateVisual();
    void startIdle(float duration);
    float visualBaseY() const;

    Renderer::Scene& m_scene;
    ProjectileSystem& m_projectiles;
    const MapConfig& m_mapConfig;
    Renderer::Object* m_hoverBody = nullptr;
    Renderer::Object* m_tankBody = nullptr;
    Renderer::Object* m_tankTurret = nullptr;
    Renderer::Material m_hoverMat;
    Renderer::Material m_tankBodyMat;
    Renderer::Material m_tankTurretMat;

    Locomotion m_locomotion = Locomotion::HOVER;

    float m_x = 0;
    float m_z = 0;
    float m_angle = 0;
    float m_speed = 160.0f;

    AIState m_state = AIState::PATROL;
    float m_stateTimer = 0;
    float m_patrolAngle = 0;
    bool m_isIdle = false;
    float m_idleTimer = 0;

    int m_health = 3;
    float m_fireTimer = 0;
    float m_fireInterval = 1.4f;
    uint32_t m_spawnIndex = 0;

    static constexpr float ENGAGE_RANGE = 420.0f;
    static constexpr float PREFERRED_DIST_MIN = 180.0f;
    static constexpr float PREFERRED_DIST_MAX = 280.0f;
    static constexpr float DISENGAGE_RANGE = 550.0f;
    static constexpr float DESPAWN_BEHIND_DIST = 350.0f;

    static constexpr float SPAWN_DIST_MIN = 520.0f;
    static constexpr float SPAWN_DIST_MAX = 720.0f;
    static constexpr float SPAWN_LATERAL_SPAN = 0.0f;

    static constexpr int VISUAL_PITCH = -90;
    static constexpr int MAX_HEALTH = 3;
};

} // namespace Game
