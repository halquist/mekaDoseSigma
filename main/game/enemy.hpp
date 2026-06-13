#pragma once

#include "Jet.hpp"
#include "types.hpp"
#include "map_config.hpp"
#include "projectile.hpp"
#include "mech_loadout.hpp"
#include "mech_rig.hpp"
#include "mech_catalog.hpp"
#include "ability.hpp"

namespace Game {

class ObstacleField;

class Enemy {
public:
    Enemy(Renderer::Scene& scene, ProjectileSystem& projectiles,
            const MapConfig& mapConfig);

    void update(float deltaTime, float playerX, float playerZ, float playerAngle,
                float playerAimY, const ObstacleField* obstacles);
    void reset(float playerX, float playerZ, float playerAngle,
               const ObstacleField* obstacles, uint32_t spawnIndex = 0);
    void spawnAsPortalBoss(float portalX, float portalZ, float portalAngle,
                           float playerX, float playerZ);
    void deactivate();
    void setWorldScaling(float speedScale, float shieldUseChance,
                         float hpScale, float fireRateScale,
                         float engageRange, float airStrikeRange, int worldIndex);

    bool isActive() const { return m_active; }
    bool isPortalBoss() const { return m_kind == EnemyKind::BossMech; }
    bool isAirborne() const;
    float getX() const { return m_x; }
    float getZ() const { return m_z; }
    float getWidth() const;
    float getAimY() const;
    float getMissileAimY() const;
    void getHitVerticalRange(float& minY, float& maxY) const;

    int takeDamage(int damage = 1, bool* outHitShield = nullptr);
    bool isAlive() const { return m_active && m_health > 0; }
    float engageRange() const { return m_engageRange; }

private:
    enum class AIState { PATROL, CHASE, ATTACK };
    enum class EnemyKind { Tank, Mech, AirJet, Blimp, BossMech };
    enum class AirPhase { Reposition, Approach, Strike, Exit };

    void spawnInRing(float playerX, float playerZ, float playerAngle, uint32_t spawnIndex,
                     AIState state, const ObstacleField* obstacles);
    void hide();
    void hideTankParts();
    void hideAirJet();
    void hideBlimp();
    bool isBehindPlayer(float playerX, float playerZ, float playerAngle) const;
    void updateAI(float deltaTime, float playerX, float playerZ, float playerAimY,
                  const ObstacleField* obstacles);
    void updateAirAI(float deltaTime, float playerX, float playerZ, float playerAngle,
                     float playerAimY);
    void setupAirRun(float playerX, float playerZ, float playerAngle);
    void setAirRepositionWaypoint(float playerX, float playerZ, float playerAngle);
    void updateVisual(float deltaTime);
    void startIdle(float duration);
    void getMechMuzzleWorld(float& wx, float& wy, float& wz) const;
    void getLaserMuzzleWorld(float& wx, float& wy, float& wz) const;
    void getMissileMuzzleWorld(float& wx, float& wy, float& wz) const;
    void getTankMuzzleWorld(float& wx, float& wy, float& wz) const;
    void getBlimpMuzzleWorld(float& wx, float& wy, float& wz) const;
    void syncRenderPivot();
    float hoverBaseY() const;
    float groundBaseY() const;
    static EnemyKind pickKind();
    void configureForKind();
    void updateBossShield(float deltaTime);

    Renderer::Scene& m_scene;
    ProjectileSystem& m_projectiles;
    const MapConfig& m_mapConfig;
    MechLoadout m_loadout;
    MechRig m_rig;
    MechAbility m_ability;

    Renderer::Object* m_tankBody = nullptr;
    Renderer::Object* m_tankTurret = nullptr;
    Renderer::Object* m_tankBarrel = nullptr;
    Renderer::Material m_tankBodyMat;
    Renderer::Material m_tankTurretMat;
    Renderer::Material m_tankBarrelMat;

    Renderer::Object* m_airJet = nullptr;
    Renderer::Material m_airJetMat;

    Renderer::Object* m_blimp = nullptr;
    Renderer::Material m_blimpBodyMat;
    Renderer::Material m_blimpCockpitMat;

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

    bool m_willUseShield = false;
    int m_shieldTriggerHealth = 0;
    bool m_shieldActivated = false;
    int m_shieldActivationsLeft = 0;
    bool m_shieldWasActive = false;
    float m_damageScale = 1.0f;
    float m_speedScale = 1.0f;
    float m_hpScale = 1.0f;
    float m_fireRateScale = 1.0f;
    float m_shieldUseChance = ENEMY_SHIELD_USE_CHANCE;
    int m_worldIndex = 0;
    bool m_mechUsesLaser = false;
    float m_behindDespawnGrace = 0.0f;
    float m_engageRange = 420.0f;
    float m_airStrikeRange = 190.0f;

    static constexpr float PREFERRED_DIST_MIN = 180.0f;
    static constexpr float PREFERRED_DIST_MAX = 280.0f;
    static constexpr float DESPAWN_BEHIND_DIST = 700.0f;
    static constexpr float DESPAWN_FAR_DIST = 820.0f;
    static constexpr float SPAWN_BEHIND_DESPAWN_GRACE = 4.0f;
    static constexpr float MECH_LASER_MUZZLE_X = 0.0f;
    static constexpr float MECH_LASER_MUZZLE_Y = 30.0f;
    static constexpr float MECH_LASER_MUZZLE_Z = 10.0f;
    static constexpr float MECH_MISSILE_MUZZLE_X = 0.0f;
    static constexpr float MECH_MISSILE_MUZZLE_Y = 29.0f;
    static constexpr float MECH_MISSILE_MUZZLE_Z = -7.0f;
    static constexpr int MECH_TIER2_WORLD_INDEX = 3;
    static constexpr float MECH_LASER_FIRE_INTERVAL = 1.15f;
    static constexpr float HOVER_OFFSET = 10.0f;
    static constexpr float FLIGHT_ALTITUDE = 92.0f;
    static constexpr float BLIMP_ALTITUDE = 148.0f;
    static constexpr float TANK_WIDTH = 28.0f;
    static constexpr float AIR_JET_WIDTH = 26.0f;
    static constexpr float BLIMP_WIDTH = 76.0f;
    static constexpr float TANK_TURRET_Y = 12.0f;
    static constexpr float TANK_BARREL_OFFSET = 16.0f;
    static constexpr float TANK_MUZZLE_OFFSET = 28.0f;
    static constexpr int TANK_MAX_HEALTH = 18;
    static constexpr int MECH_MAX_HEALTH = 36;
    static constexpr int BOSS_MECH_MAX_HEALTH = MECH_MAX_HEALTH * 3;
    static constexpr float BOSS_DAMAGE_SCALE = 1.75f;
    static constexpr int EARLY_BOSS_MAX_WORLD_INDEX = 1;
    static constexpr float EARLY_BOSS_HP_SCALE = 0.7f;
    static constexpr float EARLY_TANK_DAMAGE_SCALE = 0.7f;
    static constexpr int AIR_MAX_HEALTH = 24;
    static constexpr int BLIMP_MAX_HEALTH = 90;
    static constexpr float BLIMP_SPEED = 95.0f;
    static constexpr float BLIMP_LASER_FIRE_INTERVAL = 1.15f;
    static constexpr float BLIMP_MUZZLE_FORWARD = 38.0f;
    static constexpr float BLIMP_MUZZLE_DROP = 12.0f;
    static constexpr float ENEMY_SHIELD_USE_CHANCE = 0.45f;
    static constexpr int AIR_BOMBS_PER_RUN = 2;
    static constexpr float AIR_RUN_SPEED = 310.0f;
    static constexpr float AIR_TURN_SPEED = 230.0f;
    static constexpr float AIR_BOMB_INTERVAL = 0.38f;
    static constexpr int AIR_JET_NOSE_PITCH = -12;
    static constexpr float MECH_BODY_CENTER_Y = 32.0f;
    static constexpr float MECH_AIM_Y_MIN = 12.0f;
    static constexpr float MECH_AIM_Y_MAX = 42.0f;
    static constexpr float MECH_HIT_Y_MIN = 8.0f;
    static constexpr float MECH_HIT_Y_MAX = 46.0f;
    static constexpr float ENEMY_COLLISION_RADIUS = 14.0f;
};

} // namespace Game
