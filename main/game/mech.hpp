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

class Mech {
public:
    Mech(Renderer::Scene& scene,
         const MechLoadoutPreset& preset = MechCatalog::LOADOUT_STRIKER_HOVER);

    void update(const TouchInput& input, float deltaTime, int screenWidth, int screenHeight,
                ObstacleField* obstacles = nullptr);
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

    MechAbility& ability() { return m_ability; }
    const MechAbility& ability() const { return m_ability; }
    void equipAbility(const AbilityDef& def) { m_ability.equip(def); }
    ShieldDamageResult absorbDamage(int damage) { return m_ability.absorbDamage(damage); }

    bool isAlive() const { return m_alive; }
    void explode();

private:
    void updateVisual(float deltaTime);
    void getMuzzleWorld(float& wx, float& wy, float& wz) const;
    void snapHoverHeight();
    void syncRenderPivot();
    bool tryStartDodge(int8_t dir);
    void applyDodge(float deltaTime, ObstacleField* obstacles);

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
    float m_fireCooldown = 0;
    float m_dodgeRemaining = 0.0f;
    float m_dodgeCooldown = 0.0f;
    int8_t m_dodgeDir = 0;
    bool m_alive = true;
    bool m_touchActive = false;
    bool m_dodgeTriggeredThisTouch = false;
    float m_angleAtTouchStart = 0.0f;
    float m_touchClock = 0.0f;

    static constexpr float DODGE_DURATION = 0.22f;
    static constexpr float DODGE_SPEED = 420.0f;
    static constexpr float DODGE_COOLDOWN = 0.48f;
};

} // namespace Game
