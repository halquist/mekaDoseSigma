#pragma once

#include "Jet.hpp"
#include "enemy_manager.hpp"
#include "particles.hpp"
#include "score.hpp"
#include "types.hpp"
#include <cstdint>
#include <vector>

namespace Game {

class WorldPortal;

class AirStrikeSystem {
public:
    explicit AirStrikeSystem(Renderer::Scene& scene);

    void reset();
    void onTierEquipped(uint8_t tier, uint8_t previousTier);
    void update(float deltaTime, uint8_t tier, float mechX, float mechZ, float mechAngle,
                float skyBaseY, EnemyManager& enemies, ParticleSystem& particles,
                RunScore& score, WorldPortal* portal);

private:
    enum class State : uint8_t {
        Idle,
        Aiming,
        Incoming,
        Exploding,
    };

    struct BaseVert {
        int32_t x = 0;
        int32_t y = 0;
        int32_t z = 0;
    };

    void hideVisuals();
    void showAimLaser(float strikeX, float strikeZ, float skyY, float groundY);
    void showMissile(float y);
    void applyExplosionScale(float scale);
    bool refreshStrikeTarget(uint8_t tier, float mechX, float mechZ, float mechAngle,
                             float skyBaseY, EnemyManager& enemies);
    void applySplash(float strikeX, float strikeZ, float strikeAimY, int damage,
                     float radius, EnemyManager& enemies, ParticleSystem& particles,
                     RunScore& score, WorldPortal* portal);

    Renderer::Scene& m_scene;
    Renderer::Object* m_aimLaser = nullptr;
    Renderer::Object* m_missile = nullptr;
    Renderer::Object* m_explosion = nullptr;
    Renderer::Material m_aimMat;
    Renderer::Material m_missileMat;
    Renderer::Material m_explosionMat;
    std::vector<BaseVert> m_explosionBaseVerts;

    State m_state = State::Idle;
    float m_cooldown = 0.0f;
    float m_phaseTimer = 0.0f;
    float m_strikeX = 0.0f;
    float m_strikeZ = 0.0f;
    float m_strikeAimY = 0.0f;
    float m_skyY = 0.0f;
    float m_missileY = 0.0f;
    float m_missileVy = 0.0f;
    float m_explodeRadius = 0.0f;
    int m_explodeDamage = 0;
    uint8_t m_activeTier = 0;

    static constexpr float AIM_DURATION_SEC = 1.0f;
    static constexpr float EXPLODE_DURATION_SEC = 0.45f;
    static constexpr float MISSILE_GRAVITY = 1320.0f;
    static constexpr int32_t AIM_LASER_W = 2;
    static constexpr int32_t AIM_LASER_H = 2;
    static constexpr int32_t AIM_LASER_LEN = 260;
    static constexpr int32_t MISSILE_VIS_W = 4;
    static constexpr int32_t MISSILE_VIS_H = 4;
    static constexpr int32_t MISSILE_VIS_LEN = 24;
    static constexpr int32_t EXPLOSION_BASE_RADIUS = 6;
    static constexpr int32_t EXPLOSION_SPHERE_SEGMENTS = 5;
    static constexpr uint8_t EXPLOSION_PEAK_ALPHA = 170;
};

} // namespace Game
