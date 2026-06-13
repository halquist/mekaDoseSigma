#pragma once

#include "Jet.hpp"
#include "types.hpp"
#include "input.hpp"
#include "mech_loadout.hpp"
#include "mech_rig.hpp"
#include "mech_catalog.hpp"
#include "ability.hpp"
#include <cstdint>

namespace Game {

class ObstacleField;
class ProjectileSystem;
class EnemyManager;

class Mech {
public:
    Mech(Renderer::Scene& scene,
         const MechLoadoutPreset& preset = MechCatalog::LOADOUT_STRIKER_HOVER);

    void update(const TouchInput& input, float deltaTime, int screenWidth, int screenHeight,
                ObstacleField* obstacles = nullptr,
                const EnemyManager* enemies = nullptr);
    void reset();

    MechLoadout& loadout() { return m_loadout; }
    const MechLoadout& loadout() const { return m_loadout; }

    void rebuildVisual();

    bool tryAutoFire(ProjectileSystem& projectiles,
                     float targetX, float targetZ, float targetAimY,
                     bool targetAlive);

    float getX() const { return m_x; }
    float getZ() const { return m_z; }
    float getBaseY() const { return m_baseY; }
    float getAngle() const { return m_angle; }
    float getTurnActivity() const { return m_turnActivity; }
    int32_t getRenderPivotX() const { return m_renderX; }
    int32_t getRenderPivotZ() const { return m_renderZ; }
    int32_t getRenderPivotAngle() const { return m_renderAngle; }
    float getWidth() const { return m_loadout.hitWidth(); }
    int getMaxHp() const { return m_loadout.maxHp(); }
    int getWeaponDamage() const { return m_loadout.weaponDamage(); }
    void refreshShieldCapacity();
    void queueShieldDeploy();
    void deployPendingShield();

    MechAbility& ability() { return m_ability; }
    const MechAbility& ability() const { return m_ability; }
    void equipAbility(const AbilityDef& def) { m_ability.equip(def); }
    ShieldDamageResult absorbDamage(int damage) { return m_ability.absorbDamage(damage); }

    bool isAlive() const { return m_alive; }
    void explode();

private:
    void updateVisual(float deltaTime);
    static constexpr float PLAYER_LASER_MUZZLE_X = 0.0f;
    static constexpr float PLAYER_LASER_MUZZLE_Y = 30.0f;
    static constexpr float PLAYER_LASER_MUZZLE_Z = 10.0f;
    static constexpr float PLAYER_MISSILE_MUZZLE_X = 0.0f;
    static constexpr float PLAYER_MISSILE_MUZZLE_Y = 29.0f;
    static constexpr float PLAYER_MISSILE_MUZZLE_Z = -7.0f;

    void getLaserMuzzleWorld(float& wx, float& wy, float& wz) const;
    void getMissileMuzzleWorld(float& wx, float& wy, float& wz) const;
    bool targetInWeaponArc(float targetX, float targetZ, float range, float aimConeDeg) const;
    void snapHoverHeight();
    void syncRenderPivot();
    bool tryStartDodge(int8_t dir);
    void applyDodge(float deltaTime, ObstacleField* obstacles);
    bool tryStartFlip180(const EnemyManager* enemies);
    void applyFlip180(float deltaTime);
    void registerShortCenterTap(const EnemyManager* enemies);
    bool tryFlipOnArmedCenterTap(const EnemyManager* enemies);
    void clearCenterTapChain();

    Renderer::Scene& m_scene;
    MechLoadout m_loadout;
    MechRig m_rig;
    MechAbility m_ability;

    float m_x = 0;
    float m_z = 0;
    float m_baseY = 0;
    float m_smoothedHoverY = 0;
    int32_t m_renderX = 0;
    int32_t m_renderZ = 0;
    int32_t m_renderAngle = 0;
    float m_angle = 0;
    float m_turnActivity = 0.0f;
    float m_laserCooldown = 0.0f;
    float m_missileCooldown = 0.0f;
    float m_dodgeRemaining = 0.0f;
    float m_dodgeCooldown = 0.0f;
    int8_t m_dodgeDir = 0;
    bool m_alive = true;
    bool m_fingerDown = false;
    bool m_centerTouchSession = false;
    bool m_dodgeTriggeredThisTouch = false;
    bool m_steerLatch = false;
    float m_lastSteerSign = 0.0f;
    float m_touchClock = 0.0f;
    float m_touchDownTime = 0.0f;
    float m_touchUpDebounce = 0.0f;
    int m_lastTouchX = 0;
    int m_lastTouchY = 0;
    float m_lastCenterTapTime = -100.0f;
    bool m_centerTapArmed = false;
    float m_flipRemaining = 0.0f;
    float m_flipStartAngle = 0.0f;
    float m_flipTargetAngle = 0.0f;

    static constexpr float CENTER_DOUBLE_TAP_SEC = 0.38f;
    static constexpr float FAST_DOUBLE_TAP_DOWN_SEC = 0.30f;
    static constexpr float MIN_CENTER_TAP_GAP_SEC = 0.03f;
    static constexpr float MAX_CENTER_TAP_HOLD_SEC = 0.28f;
    static constexpr float TOUCH_UP_DEBOUNCE_SEC = 0.06f;
    static constexpr float FLIP_180_DURATION = 0.2f;
    static constexpr float DODGE_DURATION = 0.22f;
    static constexpr float DODGE_SPEED = 420.0f;
    static constexpr float DODGE_COOLDOWN = 0.48f;
};

} // namespace Game
