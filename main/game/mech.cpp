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

Mech::Mech(Renderer::Scene& scene, const MechLoadoutPreset& preset)
    : m_scene(scene)
    , m_rig(scene)
{
    m_loadout.applyPreset(preset);
    rebuildVisual();
}

void Mech::rebuildVisual() {
    m_rig.rebuild(m_loadout);
    updateVisual();
}

void Mech::getMuzzleWorld(float& wx, float& wy, float& wz) const {
    const WeaponDef* weapon = m_loadout.weapon(m_loadout.activeWeaponSlot());
    if (!weapon) {
        wx = m_x;
        wy = m_baseY;
        wz = m_z;
        return;
    }
    float lx, ly, lz;
    m_rig.weaponMuzzleLocal(*weapon, lx, ly, lz);
    m_rig.transformLocalToWorld(m_x, m_z, m_baseY, m_angle,
                                m_loadout.visualPitch(), lx, ly, lz, wx, wy, wz);
}

void Mech::updateVisual() {
    m_baseY = Terrain::hoverHeight(m_x, m_z) + MECH_HOVER_OFFSET;
    m_rig.updatePose(m_x, m_z, m_baseY, m_angle, m_loadout.visualPitch(), m_loadout);
}

void Mech::reset() {
    m_x = 0;
    m_z = 0;
    m_angle = 0;
    m_fireCooldown = 0;
    m_alive = true;
    updateVisual();
}

void Mech::explode() {
    m_alive = false;
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

void Mech::update(const TouchInput& input, float deltaTime, int screenWidth,
                  ObstacleField* obstacles) {
    if (!m_alive) return;

    if (m_fireCooldown > 0.0f) {
        m_fireCooldown -= deltaTime;
    }

    float turnInput = 0;
    bool moveForward = false;

    if (input.touched) {
        if (TouchZones::isSteerLeft(input.x, screenWidth)) {
            turnInput = -1.0f;
        } else if (TouchZones::isSteerRight(input.x, screenWidth)) {
            turnInput = 1.0f;
        } else if (TouchZones::isForward(input.x, screenWidth)) {
            moveForward = true;
        }
    }

    const float turnRate = m_loadout.turnRate();
    m_angle += turnInput * turnRate * deltaTime;

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

    updateVisual();
}

} // namespace Game
