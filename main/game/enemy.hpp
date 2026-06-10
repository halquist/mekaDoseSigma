#pragma once

#include "Jet.hpp"
#include "types.hpp"
#include "map_config.hpp"
#include "projectile.hpp"
#include "mech_loadout.hpp"
#include "mech_rig.hpp"
#include "mech_catalog.hpp"

namespace Game {

class Enemy {
public:
    Enemy(Renderer::Scene& scene, ProjectileSystem& projectiles,
            const MapConfig& mapConfig);

    void update(float deltaTime, float playerX, float playerZ, float playerAngle,
                float playerAimY);
    void reset(float playerX, float playerZ, float playerAngle, uint32_t spawnIndex = 0);
    void deactivate();

    bool isActive() const { return m_active; }
    float getX() const { return m_x; }
    float getZ() const { return m_z; }
    float getWidth() const;
    float getAimY() const;
    float getMissileAimY() const;
    void getHitVerticalRange(float& minY, float& maxY) const;

    void takeDamage();
    bool isAlive() const { return m_active && m_health > 0; }

private:
    enum class AIState { PATROL, CHASE, ATTACK };
    enum class EnemyKind { Tank, Mech, AirJet };
    enum class AirPhase { Reposition, Approach, Strike, Exit };

    void spawnInRing(float playerX, float playerZ, float playerAngle, uint32_t spawnIndex,
                     AIState state);
    void hide();
    void hideTankParts();
    void hideAirJet();
    bool isBehindPlayer(float playerX, float playerZ, float playerAngle) const;
    void updateAI(float deltaTime, float playerX, float playerZ, float playerAimY);
    void updateAirAI(float deltaTime, float playerX, float playerZ, float playerAngle,
                     float playerAimY);
    void setupAirRun(float playerX, float playerZ, float playerAngle);
    void setAirRepositionWaypoint(float playerX, float playerZ, float playerAngle);
    void updateVisual(float deltaTime);
    void startIdle(float duration);
    void getMechMuzzleWorld(float& wx, float& wy, float& wz) const;
    void getTankMuzzleWorld(float& wx, float& wy, float& wz) const;
    void syncRenderPivot();
    float hoverBaseY() const;
    float groundBaseY() const;
    static EnemyKind pickKind();
    void configureForKind();

    Renderer::Scene& m_scene;
    ProjectileSystem& m_projectiles;
    const MapConfig& m_mapConfig;
    MechLoadout m_loadout;
    MechRig m_rig;

    Renderer::Object* m_tankBody = nullptr;
    Renderer::Object* m_tankTurret = nullptr;
    Renderer::Object* m_tankBarrel = nullptr;
    Renderer::Material m_tankBodyMat;
    Renderer::Material m_tankTurretMat;
    Renderer::Material m_tankBarrelMat;

    Renderer::Object* m_airJet = nullptr;
    Renderer::Material m_airJetMat;

    EnemyKind m_kind = EnemyKind::Tank;

    float m_x = 0;
    float m_z = 0;
    float m_angle = 0;
    float m_baseY = 0;
    float m_smoothedHoverY = 0;
    int32_t m_renderX = 0;
    int32_t m_renderZ = 0;
    int32_t m_renderAngle = 0;
    float m_speed = 110.0f;
    float m_passSide = 1.0f;
    float m_runAngle = 0.0f;
    float m_bankRoll = 0.0f;
    float m_waypointX = 0.0f;
    float m_waypointZ = 0.0f;
    float m_bombDropCooldown = 0.0f;
    AirPhase m_airPhase = AirPhase::Reposition;
    int m_bombsThisRun = 0;

    AIState m_state = AIState::PATROL;
    float m_stateTimer = 0;
    float m_patrolAngle = 0;
    bool m_isIdle = false;
    float m_idleTimer = 0;

    int m_health = 3;
    float m_fireTimer = 0;
    float m_fireInterval = 1.4f;
    uint32_t m_spawnIndex = 0;
    bool m_active = false;

    static constexpr float ENGAGE_RANGE = 420.0f;
    static constexpr float PREFERRED_DIST_MIN = 180.0f;
    static constexpr float PREFERRED_DIST_MAX = 280.0f;
    static constexpr float DESPAWN_BEHIND_DIST = 350.0f;
    static constexpr float DESPAWN_FAR_DIST = 820.0f;
    static constexpr float HOVER_OFFSET = 10.0f;
    static constexpr float FLIGHT_ALTITUDE = 92.0f;
    static constexpr float TANK_WIDTH = 28.0f;
    static constexpr float AIR_JET_WIDTH = 26.0f;
    static constexpr float TANK_TURRET_Y = 12.0f;
    static constexpr float TANK_BARREL_OFFSET = 16.0f;
    static constexpr float TANK_MUZZLE_OFFSET = 28.0f;
    static constexpr int TANK_MAX_HEALTH = 3;
    static constexpr int MECH_MAX_HEALTH = 6;
    static constexpr int AIR_MAX_HEALTH = 4;
    static constexpr int AIR_BOMBS_PER_RUN = 2;
    static constexpr float AIR_RUN_SPEED = 310.0f;
    static constexpr float AIR_TURN_SPEED = 230.0f;
    static constexpr float AIR_STRIKE_RANGE = 190.0f;
    static constexpr float AIR_BOMB_INTERVAL = 0.38f;
    static constexpr int AIR_JET_NOSE_PITCH = -12;
    static constexpr float MECH_BODY_CENTER_Y = 32.0f;
    static constexpr float MECH_AIM_Y_MIN = 12.0f;
    static constexpr float MECH_AIM_Y_MAX = 42.0f;
    static constexpr float MECH_HIT_Y_MIN = 8.0f;
    static constexpr float MECH_HIT_Y_MAX = 46.0f;
};

} // namespace Game
