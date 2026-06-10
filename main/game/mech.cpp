#include "mech.hpp"
#include "terrain.hpp"
#include "obstacles.hpp"
#include "projectile.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Game {

static constexpr float MECH_HOVER_OFFSET = 10.0f;
static constexpr float HOVER_SMOOTH_RATE = 0.75f;
static constexpr float HOVER_SAMPLE_RADIUS = 32.0f;

static float smoothToward(float current, float target, float rate, float deltaTime) {
    const float blend = fminf(1.0f, rate * deltaTime);
    return current + (target - current) * blend;
}

static float averagedHoverHeight(float worldX, float worldZ) {
    const float center = Terrain::hoverHeight(worldX, worldZ);
    const float r = HOVER_SAMPLE_RADIUS;
    const float left = Terrain::hoverHeight(worldX - r, worldZ);
    const float right = Terrain::hoverHeight(worldX + r, worldZ);
    const float front = Terrain::hoverHeight(worldX, worldZ + r);
    const float back = Terrain::hoverHeight(worldX, worldZ - r);
    return (center + left + right + front + back) * 0.2f;
}

Mech::Mech(Renderer::Scene& scene, const MechLoadoutPreset& preset)
    : m_scene(scene)
    , m_rig(scene)
    , m_ability(scene)
{
    m_loadout.applyPreset(preset);
    m_ability.equip(AbilityCatalog::SHIELD);
    rebuildVisual();
}

void Mech::refreshShieldCapacity() {
    AbilityDef def = AbilityCatalog::SHIELD;
    def.shieldCapacity =
        AbilityCatalog::SHIELD.shieldCapacity + m_loadout.bonuses().shieldCapacity;
    m_ability.equip(def);
}

void Mech::rebuildVisual() {
    m_rig.rebuild(m_loadout);
    snapHoverHeight();
    syncRenderPivot();
    m_baseY = m_smoothedHoverY;
    m_rig.updatePose(m_renderX, m_renderZ, m_baseY, m_renderAngle,
                     m_loadout.visualPitch(), m_loadout);
}

void Mech::snapHoverHeight() {
    m_smoothedHoverY = averagedHoverHeight(m_x, m_z) + MECH_HOVER_OFFSET;
}

void Mech::syncRenderPivot() {
    m_renderX = static_cast<int32_t>(lroundf(m_x));
    m_renderZ = static_cast<int32_t>(lroundf(m_z));
    m_renderAngle = static_cast<int32_t>(lroundf(m_angle));
}

void Mech::getMuzzleWorld(float& wx, float& wy, float& wz) const {
    const WeaponDef* weapon = m_loadout.weapon(m_loadout.activeWeaponSlot());
    if (!weapon) {
        wx = static_cast<float>(m_renderX);
        wy = m_baseY;
        wz = static_cast<float>(m_renderZ);
        return;
    }
    float lx, ly, lz;
    m_rig.weaponMuzzleLocal(*weapon, lx, ly, lz);
    m_rig.transformLocalToWorld(static_cast<float>(m_renderX),
                                static_cast<float>(m_renderZ),
                                m_baseY,
                                static_cast<float>(m_renderAngle),
                                m_loadout.visualPitch(), lx, ly, lz, wx, wy, wz);
}

void Mech::updateVisual(float deltaTime) {
    const float targetY = averagedHoverHeight(m_x, m_z) + MECH_HOVER_OFFSET;

    if (deltaTime > 0.0f) {
        m_smoothedHoverY = smoothToward(m_smoothedHoverY, targetY, HOVER_SMOOTH_RATE, deltaTime);
    } else {
        snapHoverHeight();
    }

    syncRenderPivot();
    m_baseY = m_smoothedHoverY;
    m_rig.updatePose(m_renderX, m_renderZ, m_baseY, m_renderAngle,
                     m_loadout.visualPitch(), m_loadout);
}

void Mech::reset() {
    m_loadout.applyPreset(MechCatalog::LOADOUT_STRIKER_HOVER);
    refreshShieldCapacity();
    m_x = 0;
    m_z = 0;
    m_angle = 0;
    m_turnActivity = 0.0f;
    m_fireCooldown = 0;
    m_dodgeRemaining = 0;
    m_dodgeCooldown = 0;
    m_dodgeDir = 0;
    m_touchActive = false;
    m_dodgeTriggeredThisTouch = false;
    m_angleAtTouchStart = 0.0f;
    m_touchClock = 0.0f;
    m_alive = true;
    m_ability.reset();
    m_rig.setHidden(false);
    snapHoverHeight();
    syncRenderPivot();
    m_baseY = m_smoothedHoverY;
    m_rig.updatePose(m_renderX, m_renderZ, m_baseY, m_renderAngle,
                     m_loadout.visualPitch(), m_loadout);
}

void Mech::explode() {
    m_alive = false;
    m_ability.reset();
    m_rig.setHidden(true);
}

bool Mech::tryAutoFire(ProjectileSystem& projectiles,
                       float targetX, float targetZ, float targetAimY,
                       bool targetAlive) {
    if (m_fireCooldown > 0.0f || !targetAlive) return false;

    const WeaponDef* weapon = m_loadout.weapon(m_loadout.activeWeaponSlot());
    if (!weapon) return false;

    float dx = targetX - m_x;
    float dz = targetZ - m_z;
    float dist = sqrtf(dx * dx + dz * dz);
    if (dist > weapon->range) return false;

    float angleToTarget = atan2f(dx, dz) * 180.0f / M_PI;
    float angleDiff = angleToTarget - m_angle;
    while (angleDiff > 180.0f) angleDiff -= 360.0f;
    while (angleDiff < -180.0f) angleDiff += 360.0f;
    if (fabsf(angleDiff) > weapon->aimConeDeg) return false;

    float mx, my, mz;
    getMuzzleWorld(mx, my, mz);

    if (weapon->kind == WeaponKind::HomingMissile) {
        projectiles.firePlayerAtTarget(mx, my, mz, targetX, targetZ, targetAimY);
    } else {
        projectiles.firePlayerStraight(mx, my, mz, targetX, targetZ, targetAimY);
    }

    m_fireCooldown = weapon->cooldown;
    return true;
}

bool Mech::tryStartDodge(int8_t dir) {
    if (dir == 0 || m_dodgeRemaining > 0.0f || m_dodgeCooldown > 0.0f) {
        return false;
    }

    m_dodgeDir = dir;
    m_dodgeRemaining = DODGE_DURATION;
    m_dodgeCooldown = DODGE_COOLDOWN;
    return true;
}

void Mech::applyDodge(float deltaTime, ObstacleField* obstacles) {
    if (m_dodgeRemaining <= 0.0f) return;

    m_dodgeRemaining -= deltaTime;
    if (m_dodgeRemaining < 0.0f) {
        m_dodgeRemaining = 0.0f;
    }

    const float radians = m_angle * static_cast<float>(M_PI) / 180.0f;
    const float strafeX = cosf(radians) * static_cast<float>(m_dodgeDir);
    const float strafeZ = -sinf(radians) * static_cast<float>(m_dodgeDir);
    const float newX = m_x + strafeX * DODGE_SPEED * deltaTime;
    const float newZ = m_z + strafeZ * DODGE_SPEED * deltaTime;

    if (obstacles) {
        obstacles->resolveMovement(
            m_x, m_z, newX, newZ, ObstacleField::PLAYER_RADIUS);
    } else {
        m_x = newX;
        m_z = newZ;
    }
}

void Mech::update(const TouchInput& input, float deltaTime, int screenWidth,
                  ObstacleField* obstacles) {
    if (!m_alive) return;

    m_touchClock += deltaTime;
    m_turnActivity = 0.0f;

    if (m_fireCooldown > 0.0f) {
        m_fireCooldown -= deltaTime;
    }
    if (m_dodgeCooldown > 0.0f) {
        m_dodgeCooldown -= deltaTime;
    }

    float turnInput = 0.0f;
    bool moveForward = false;

    if (input.touched) {
        if (!m_touchActive) {
            m_touchActive = true;
            m_angleAtTouchStart = m_angle;
            m_dodgeTriggeredThisTouch = false;

            if (TouchZones::isForward(input.x, screenWidth)) {
                m_ability.onCenterTap(m_touchClock);
            }
        }

        const bool dodgeLeft = TouchZones::isDodgeLeft(input.x, screenWidth);
        const bool dodgeRight = TouchZones::isDodgeRight(input.x, screenWidth);

        if (dodgeLeft || dodgeRight) {
            if (!m_dodgeTriggeredThisTouch) {
                const int8_t dir = dodgeLeft ? static_cast<int8_t>(-1) : static_cast<int8_t>(1);
                if (tryStartDodge(dir)) {
                    m_angle = m_angleAtTouchStart;
                    m_dodgeTriggeredThisTouch = true;
                }
            }
        } else if (m_dodgeRemaining <= 0.0f) {
            if (TouchZones::isSteerLeft(input.x, screenWidth)) {
                turnInput = -1.0f;
            } else if (TouchZones::isSteerRight(input.x, screenWidth)) {
                turnInput = 1.0f;
            } else if (TouchZones::isForward(input.x, screenWidth)) {
                moveForward = true;
            }
        }
    } else {
        m_touchActive = false;
        m_dodgeTriggeredThisTouch = false;
    }

    if (m_dodgeRemaining > 0.0f) {
        turnInput = 0.0f;
    }

    const float turnRate = m_loadout.turnRate();
    m_angle += turnInput * turnRate * deltaTime;
    m_turnActivity = fabsf(turnInput);

    if (moveForward) {
        const float speed = m_loadout.moveSpeed();
        const float radians = m_angle * M_PI / 180.0f;
        const float newX = m_x + sinf(radians) * speed * deltaTime;
        const float newZ = m_z + cosf(radians) * speed * deltaTime;

        if (obstacles) {
            obstacles->resolveMovement(
                m_x, m_z, newX, newZ, ObstacleField::PLAYER_RADIUS);
        } else {
            m_x = newX;
            m_z = newZ;
        }
    }

    applyDodge(deltaTime, obstacles);

    updateVisual(deltaTime);
    m_ability.update(deltaTime, m_rig, m_renderX, m_renderZ, m_baseY,
                     m_renderAngle, m_loadout.visualPitch());
}

} // namespace Game
