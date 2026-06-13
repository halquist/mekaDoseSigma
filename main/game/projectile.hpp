#pragma once

#include "Jet.hpp"
#include "types.hpp"
#include <array>
#include <cstdint>

namespace Game {

enum class ProjectileVisualKind : uint8_t {
    None,
    PlayerMissile,
    Bolt,
    Bomb,
    Laser,
    PlayerLaser,
};

class ProjectileSystem {
public:
    explicit ProjectileSystem(Renderer::Scene& scene);

    void firePlayerAtTarget(float x, float y, float z, float targetX, float targetZ,
                            float targetAimY, int damage);
    void firePlayerStraight(float x, float y, float z, float targetX, float targetZ,
                            float targetAimY, int damage);
    void fireDroneLaser(float x, float y, float z, float targetX, float targetZ,
                        float targetAimY, int damage);
    void firePlayerLaser(float x, float y, float z, float targetX, float targetZ,
                         float targetAimY, int damage);
    void fireEnemyHomingAtTarget(float x, float y, float z, float targetX, float targetZ,
                                 float targetAimY, float damageScale = 1.0f);
    void fireEnemyLaserAtTarget(float x, float y, float z, float targetX, float targetZ,
                                float targetAimY, float damageScale = 1.0f);
    void fireEnemyAtTarget(float x, float z, float targetX, float targetZ,
                           float damageScale = 1.0f);
    void fireEnemyBomb(float x, float y, float z, float targetX, float targetZ);

    void update(float deltaTime);
    bool checkMissileTargetImpact(float* outX = nullptr, float* outZ = nullptr,
                                  float* outY = nullptr);
    void reset();
    void setEnemyDamageScale(float scale);

    int checkPlayerHit(float playerX, float playerZ, float playerAimY, float playerWidth,
                       float* outHitX = nullptr, float* outHitZ = nullptr,
                       float* outHitY = nullptr);
    bool checkEnemyHit(float enemyX, float enemyZ, float enemyMinY, float enemyMaxY,
                       float enemyWidth, int* outDamage = nullptr);

private:
    struct Projectile {
        Renderer::Object* obj = nullptr;
        Renderer::Object* missileObj = nullptr;
        Renderer::Object* boltObj = nullptr;
        Renderer::Object* bombObj = nullptr;
        Renderer::Object* laserObj = nullptr;
        ProjectileVisualKind visualKind = ProjectileVisualKind::None;
        float x = 0;
        float z = 0;
        float prevX = 0;
        float prevZ = 0;
        float prevY = 0;
        float vx = 0;
        float vz = 0;
        float y = 0;
        float vy = 0;
        float targetX = 0;
        float targetZ = 0;
        float targetAimY = 0;
        float launchWorldY = 0;
        float targetHoverY = 0;
        float targetYOffset = 0;
        float launchDist = 0;
        float arcPathLen = 0;
        float pathTraveled = 0;
        float peakArcY = 0;
        float life = 0;
        float trailTimer = 0;
        bool active = false;
        bool isPlayerProjectile = false;
        bool isHomingMissile = false;
        bool isFallingBomb = false;
        float damageScale = 1.0f;
        int playerHitDamage = 0;
    };

    struct TrailPuff {
        Renderer::Object* obj = nullptr;
        float life = 0;
        float maxLife = 0;
        bool active = false;
    };

    void destroyProjectile(Projectile& p);
    void detonateMissile(Projectile& p);
    void applyProjectileVisual(Projectile& p, ProjectileVisualKind kind);
    void updateHomingMissile(Projectile& p, float deltaTime);
    void updateStraightProjectile(Projectile& p, float deltaTime);
    void updateFallingBomb(Projectile& p, float deltaTime);
    void updateMissileVisual(Projectile& p);
    void spawnTrailPuff(float x, float y, float z, bool isPlayerProjectile);
    float missileWorldY(const Projectile& p) const;
    float computeMissileDesiredYOffset(const Projectile& p, float progress) const;

    static constexpr int MAX_PROJECTILES = 16;
    static constexpr int TRAIL_PUFFS = 14;

    Renderer::Scene& m_scene;
    Renderer::Material m_missileMat;
    Renderer::Material m_enemyProjMat;
    Renderer::Material m_bombMat;
    Renderer::Material m_laserMat;
    Renderer::Material m_playerLaserMat;

    std::array<Projectile, MAX_PROJECTILES> m_projectiles;
    std::array<TrailPuff, TRAIL_PUFFS> m_trailPuffs;
    std::array<Renderer::Material, TRAIL_PUFFS> m_trailMats;
    float m_enemyDamageScale = 1.0f;

    static constexpr float PROJECTILE_SPEED = 650.0f;
    static constexpr float PROJECTILE_LIFE = 2.8f;
    static constexpr float ENEMY_HIT_RADIUS = 36.0f;
    static constexpr float ENEMY_HIT_Y_TOLERANCE = 10.0f;

    static constexpr int PLAYER_DAMAGE_TANK_BOLT = 9;
    static constexpr int PLAYER_DAMAGE_ENEMY_MISSILE = 6;
    static constexpr int PLAYER_DAMAGE_ENEMY_LASER = 3;
    static constexpr int PLAYER_DAMAGE_BOMB = 14;

    static constexpr float MISSILE_PEAK_ARC_Y = 24.0f;

    static constexpr float MISSILE_LAUNCH_SPEED = 240.0f;
    static constexpr float MISSILE_CRUISE_SPEED = 520.0f;
    static constexpr float MISSILE_SWOOP_DEG = 52.0f;
    static constexpr float MISSILE_CLIMB_VY = 140.0f;
    static constexpr float MISSILE_GRAVITY = 110.0f;
    static constexpr float MISSILE_TURN_RATE = 200.0f;
    static constexpr float MISSILE_TRAIL_INTERVAL = 0.065f;
    static constexpr float MISSILE_TRAIL_LIFE = 0.44f;
    static constexpr uint8_t MISSILE_TRAIL_BASE_ALPHA = 150;

    /// Player homing missile mesh
    static constexpr int32_t MISSILE_VIS_W = 3;
    static constexpr int32_t MISSILE_VIS_H = 3;
    static constexpr int32_t MISSILE_VIS_LEN = 10;
    static constexpr float BOMB_GRAVITY = 220.0f;
    static constexpr float BOMB_SPLASH_RADIUS = 42.0f;
    static constexpr int32_t BOMB_VIS_SIZE = 7;
    static constexpr int32_t MISSILE_TRAIL_PUFF_SIZE = 2;

    static constexpr float LASER_SPEED = 1000.0f;
    static constexpr float LASER_LIFE = 0.35f;
    static constexpr int32_t LASER_VIS_W = 2;
    static constexpr int32_t LASER_VIS_H = 2;
    static constexpr int32_t LASER_VIS_LEN = 48;
};

} // namespace Game
