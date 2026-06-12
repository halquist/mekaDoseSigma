#include "mech.hpp"
#include "enemy_manager.hpp"
#include "run_upgrades.hpp"
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
    m_ability.configurePlayerShield(0);
    rebuildVisual();
}

void Mech::refreshShieldCapacity() {
    const int capacity = shieldCapacityForTier(m_loadout.bonuses().shieldTier);
    m_ability.configurePlayerShield(capacity);
}

void Mech::queueShieldDeploy() {
    m_ability.queueAutoDeploy();
}

void Mech::deployPendingShield() {
    m_ability.deployPendingAutoShield();
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

void Mech::getLaserMuzzleWorld(float& wx, float& wy, float& wz) const {
    float lx = PLAYER_LASER_MUZZLE_X;
    float ly = PLAYER_LASER_MUZZLE_Y;
    float lz = PLAYER_LASER_MUZZLE_Z;
    m_rig.transformLocalToWorld(static_cast<float>(m_renderX),
                                static_cast<float>(m_renderZ),
                                m_baseY,
                                static_cast<float>(m_renderAngle),
                                m_loadout.visualPitch(), lx, ly, lz, wx, wy, wz);
}

void Mech::getMissileMuzzleWorld(float& wx, float& wy, float& wz) const {
    float lx = PLAYER_MISSILE_MUZZLE_X;
    float ly = PLAYER_MISSILE_MUZZLE_Y;
    float lz = PLAYER_MISSILE_MUZZLE_Z;
    m_rig.transformLocalToWorld(static_cast<float>(m_renderX),
                                static_cast<float>(m_renderZ),
                                m_baseY,
                                static_cast<float>(m_renderAngle),
                                m_loadout.visualPitch(), lx, ly, lz, wx, wy, wz);
}

bool Mech::targetInWeaponArc(float targetX, float targetZ, float range,
                             float aimConeDeg) const {
    const float dx = targetX - m_x;
    const float dz = targetZ - m_z;
    const float dist = sqrtf(dx * dx + dz * dz);
    if (dist > range) {
        return false;
    }

    const float angleToTarget = atan2f(dx, dz) * 180.0f / static_cast<float>(M_PI);
    float angleDiff = angleToTarget - m_angle;
    while (angleDiff > 180.0f) angleDiff -= 360.0f;
    while (angleDiff < -180.0f) angleDiff += 360.0f;
    return fabsf(angleDiff) <= aimConeDeg;
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
    m_laserCooldown = 0;
    m_missileCooldown = 0;
    m_dodgeRemaining = 0;
    m_dodgeCooldown = 0;
    m_dodgeDir = 0;
    m_fingerDown = false;
    m_centerTouchSession = false;
    m_dodgeTriggeredThisTouch = false;
    m_steerLatch = false;
    m_lastSteerSign = 0.0f;
    m_touchClock = 0.0f;
    m_touchDownTime = 0.0f;
    m_touchUpDebounce = 0.0f;
    m_lastCenterTapTime = -100.0f;
    m_flipRemaining = 0.0f;
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
    if (!targetAlive) {
        return false;
    }

    const RunBonuses& bonuses = m_loadout.bonuses();
    bool fired = false;

    if (bonuses.laserTier >= 1 && m_laserCooldown <= 0.0f &&
        targetInWeaponArc(targetX, targetZ,
                          laserRangeForTier(bonuses.laserTier),
                          laserAimConeDeg())) {
        float mx = 0.0f;
        float my = 0.0f;
        float mz = 0.0f;
        getLaserMuzzleWorld(mx, my, mz);
        projectiles.firePlayerLaser(mx, my, mz, targetX, targetZ, targetAimY,
                                    laserDamageForTier(bonuses.laserTier));
        m_laserCooldown = laserCooldownForTier(bonuses.laserTier);
        fired = true;
    }

    if (bonuses.missileTier >= 1 && m_missileCooldown <= 0.0f &&
        targetInWeaponArc(targetX, targetZ,
                          missileRangeForTier(bonuses.missileTier),
                          missileAimConeDeg())) {
        float mx = 0.0f;
        float my = 0.0f;
        float mz = 0.0f;
        getMissileMuzzleWorld(mx, my, mz);
        projectiles.firePlayerAtTarget(mx, my, mz, targetX, targetZ, targetAimY,
                                       missileDamageForTier(bonuses.missileTier));
        m_missileCooldown = missileCooldownForTier(bonuses.missileTier);
        fired = true;
    }

    return fired;
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

bool Mech::tryStartFlip180(const EnemyManager* enemies) {
    if (m_flipRemaining > 0.0f) {
        return false;
    }

    m_flipStartAngle = m_angle;
    float targetAngle = m_angle + 180.0f;

    if (enemies != nullptr && enemies->isPlayerUnderAttack(m_x, m_z, m_angle)) {
        float threatX = 0.0f;
        float threatZ = 0.0f;
        if (enemies->findClosestEnemyBehind(m_x, m_z, m_angle, threatX, threatZ)) {
            targetAngle = atan2f(threatX - m_x, threatZ - m_z) *
                180.0f / static_cast<float>(M_PI);
        }
    }

    while (targetAngle > 180.0f) {
        targetAngle -= 360.0f;
    }
    while (targetAngle < -180.0f) {
        targetAngle += 360.0f;
    }

    float angleDiff = targetAngle - m_flipStartAngle;
    while (angleDiff > 180.0f) {
        angleDiff -= 360.0f;
    }
    while (angleDiff < -180.0f) {
        angleDiff += 360.0f;
    }
    m_flipTargetAngle = m_flipStartAngle + angleDiff;
    m_flipRemaining = FLIP_180_DURATION;
    return true;
}

void Mech::applyFlip180(float deltaTime) {
    if (m_flipRemaining <= 0.0f) {
        return;
    }

    m_flipRemaining -= deltaTime;
    const float t = 1.0f - fmaxf(0.0f, m_flipRemaining) / FLIP_180_DURATION;
    const float eased = t * t * (3.0f - 2.0f * t);
    m_angle = m_flipStartAngle + (m_flipTargetAngle - m_flipStartAngle) * eased;
    m_turnActivity = 1.0f;

    if (m_flipRemaining <= 0.0f) {
        m_flipRemaining = 0.0f;
        m_angle = m_flipTargetAngle;
    }
}

void Mech::handleCenterTapRelease(const EnemyManager* enemies) {
    const bool shieldActivated = m_ability.onCenterTap(m_touchClock);
    if (shieldActivated) {
        m_lastCenterTapTime = -100.0f;
        return;
    }

    if (m_lastCenterTapTime >= 0.0f &&
        (m_touchClock - m_lastCenterTapTime) <= CENTER_DOUBLE_TAP_SEC) {
        tryStartFlip180(enemies);
        m_lastCenterTapTime = -100.0f;
    } else {
        m_lastCenterTapTime = m_touchClock;
    }
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

void Mech::update(const TouchInput& input, float deltaTime, int screenWidth, int screenHeight,
                  ObstacleField* obstacles, const EnemyManager* enemies) {
    if (!m_alive) return;

    m_touchClock += deltaTime;
    m_turnActivity = 0.0f;

    if (m_laserCooldown > 0.0f) {
        m_laserCooldown -= deltaTime;
    }
    if (m_missileCooldown > 0.0f) {
        m_missileCooldown -= deltaTime;
    }
    if (m_dodgeCooldown > 0.0f) {
        m_dodgeCooldown -= deltaTime;
    }

    float turnInput = 0.0f;
    bool moveForward = false;
    bool moveReverse = false;

    if (input.touched) {
        m_lastTouchX = input.x;
        m_lastTouchY = input.y;
        m_touchUpDebounce = 0.0f;

        if (!m_fingerDown) {
            m_fingerDown = true;
            m_touchDownTime = m_touchClock;
            m_dodgeTriggeredThisTouch = false;
            m_centerTouchSession = false;
        }

        if (TouchZones::isForward(input.x, screenWidth)
            && !TouchZones::isReverse(input.y, screenHeight)) {
            m_centerTouchSession = true;
        }
    } else if (m_fingerDown) {
        m_touchUpDebounce += deltaTime;
        if (m_touchUpDebounce >= TOUCH_UP_DEBOUNCE_SEC) {
            if (m_centerTouchSession) {
                const float holdDuration = m_touchClock - m_touchDownTime;
                if (holdDuration <= MAX_CENTER_TAP_HOLD_SEC) {
                    handleCenterTapRelease(enemies);
                }
            }
            m_fingerDown = false;
            m_centerTouchSession = false;
            m_dodgeTriggeredThisTouch = false;
            m_steerLatch = false;
        }
    }

    if (m_fingerDown) {
        const int touchX = input.touched ? input.x : m_lastTouchX;
        const int touchY = input.touched ? input.y : m_lastTouchY;

        const bool dodgeLeft = TouchZones::isDodgeLeft(touchX, screenWidth);
        const bool dodgeRight = TouchZones::isDodgeRight(touchX, screenWidth);

        if (dodgeLeft || dodgeRight) {
            if (!m_dodgeTriggeredThisTouch) {
                const int8_t dir = dodgeLeft ? static_cast<int8_t>(-1) : static_cast<int8_t>(1);
                if (tryStartDodge(dir)) {
                    m_dodgeTriggeredThisTouch = true;
                }
            }
        } else if (m_dodgeRemaining <= 0.0f) {
            const float rawSteer = TouchZones::steerInput(touchX, screenWidth);
            const int distFromCenter = TouchZones::distFromCenterX(touchX, screenWidth);

            if (rawSteer != 0.0f) {
                m_steerLatch = true;
            }
            if (distFromCenter <
                TouchZones::FORWARD_HALF_WIDTH - TouchZones::STEER_LATCH_RELEASE_PX) {
                m_steerLatch = false;
            }

            if (m_steerLatch || rawSteer != 0.0f) {
                turnInput = (rawSteer != 0.0f) ? rawSteer : m_lastSteerSign;
            }

            if (turnInput == 0.0f && !m_steerLatch) {
                if (TouchZones::isReverse(touchY, screenHeight)) {
                    moveReverse = true;
                } else if (TouchZones::isForward(touchX, screenWidth)) {
                    moveForward = true;
                }
            }
        }
    }

    if (turnInput != 0.0f) {
        m_lastSteerSign = turnInput > 0.0f ? 1.0f : -1.0f;
    }

    if (m_dodgeRemaining > 0.0f || m_flipRemaining > 0.0f) {
        turnInput = 0.0f;
    }

    if (m_flipRemaining > 0.0f) {
        applyFlip180(deltaTime);
    } else {
        const float turnRate = m_loadout.turnRate();
        m_angle += turnInput * turnRate * deltaTime;
        m_turnActivity = fabsf(turnInput);
    }

    const float speed = m_loadout.moveSpeed();
    const float radians = m_angle * M_PI / 180.0f;

    if (moveForward) {
        const float newX = m_x + sinf(radians) * speed * deltaTime;
        const float newZ = m_z + cosf(radians) * speed * deltaTime;

        if (obstacles) {
            obstacles->resolveMovement(
                m_x, m_z, newX, newZ, ObstacleField::PLAYER_RADIUS);
        } else {
            m_x = newX;
            m_z = newZ;
        }
    } else if (moveReverse) {
        const float newX = m_x - sinf(radians) * speed * deltaTime;
        const float newZ = m_z - cosf(radians) * speed * deltaTime;

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
