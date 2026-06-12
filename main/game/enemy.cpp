#include "enemy.hpp"
#include "obstacles.hpp"
#include "run_upgrades.hpp"
#include "scene_util.hpp"
#include "terrain.hpp"
#include "worldgen.hpp"
#include "rng.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Game {

namespace {

Renderer::Object* createStealthBomber(Renderer::Material* mat) {
    auto* jet = new Renderer::Object();
    jet->cullingMode = Renderer::CullingMode::NO_CULLING;

    auto addV = [&](int32_t x, int32_t y, int32_t z) -> uint16_t {
        Renderer::Object::Vertex v;
        v.position = {x, y, z};
        jet->addVertex(v);
        return static_cast<uint16_t>(jet->vertices.size() - 1);
    };

    // Flat flying wing in X/Z (+Z nose, +Y up), stealth-bomber arrow silhouette.
    const int32_t yTop = 2;
    const int32_t yBot = 0;

    const uint16_t nT = addV(0, yTop, 26);
    const uint16_t lT = addV(-32, yTop, -18);
    const uint16_t rT = addV(32, yTop, -18);
    const uint16_t tT = addV(0, yTop, -22);

    const uint16_t nB = addV(0, yBot, 26);
    const uint16_t lB = addV(-32, yBot, -18);
    const uint16_t rB = addV(32, yBot, -18);
    const uint16_t tB = addV(0, yBot, -22);

    jet->addTriangle(nT, lT, rT, mat);
    jet->addTriangle(lT, tT, rT, mat);
    jet->addTriangle(nB, rB, lB, mat);
    jet->addTriangle(lB, tB, rB, mat);

    jet->addTriangle(nT, nB, lB, mat);
    jet->addTriangle(nT, lB, lT, mat);
    jet->addTriangle(nT, rT, rB, mat);
    jet->addTriangle(nT, rB, nB, mat);
    jet->addTriangle(lT, lB, tB, mat);
    jet->addTriangle(lT, tB, tT, mat);
    jet->addTriangle(rT, tT, tB, mat);
    jet->addTriangle(rT, tB, rB, mat);

    jet->calculateBoundingBox();
    const int32_t cx = jet->centreVolume.x;
    const int32_t cy = jet->centreVolume.y;
    const int32_t cz = jet->centreVolume.z;
    for (auto& v : jet->vertices) {
        v.position.x -= cx;
        v.position.y -= cy;
        v.position.z -= cz;
    }
    jet->calculateBoundingBox();
    return jet;
}

static float wrapAngleDeg(float angle) {
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

} // namespace

Enemy::EnemyKind Enemy::pickKind() {
    // ~17% mech, ~17% air jet, ~66% tank
    const uint32_t roll = Rng::nextRange(100);
    if (roll < 17) return EnemyKind::Mech;
    if (roll < 34) return EnemyKind::AirJet;
    return EnemyKind::Tank;
}

Enemy::Enemy(Renderer::Scene& scene, ProjectileSystem& projectiles,
             const MapConfig& mapConfig)
    : m_scene(scene)
    , m_projectiles(projectiles)
    , m_mapConfig(mapConfig)
    , m_rig(scene)
    , m_ability(scene)
    , m_tankBodyMat(Colors::TANK_BODY)
    , m_tankTurretMat(Colors::TANK_TURRET)
    , m_tankBarrelMat(Colors::TANK_BARREL)
    , m_airJetMat(Colors::AIR_JET)
{
    m_loadout.applyPreset(MechCatalog::LOADOUT_STRIKER_HOVER);

    m_tankBodyMat.shadingMode = Renderer::ShadingMode::GOURAUD;
    m_tankTurretMat.shadingMode = Renderer::ShadingMode::GOURAUD;
    m_tankBarrelMat.shadingMode = Renderer::ShadingMode::GOURAUD;
    m_airJetMat.shadingMode = Renderer::ShadingMode::GOURAUD;

    m_tankBody = Primitives::createCube(36, 14, 28, &m_tankBodyMat);
    m_tankTurret = Primitives::createCube(18, 10, 18, &m_tankTurretMat);
    m_tankBarrel = Primitives::createCube(5, 5, 18, &m_tankBarrelMat);
    m_airJet = createStealthBomber(&m_airJetMat);

    m_scene.addObject(m_tankBody);
    m_scene.addObject(m_tankTurret);
    m_scene.addObject(m_tankBarrel);
    m_scene.addObject(m_airJet);

    hide();
}

void Enemy::hideTankParts() {
    stashSceneObject(m_tankBody);
    stashSceneObject(m_tankTurret);
    stashSceneObject(m_tankBarrel);
}

void Enemy::hideAirJet() {
    stashSceneObject(m_airJet);
}

void Enemy::hide() {
    m_rig.setHidden(true);
    hideTankParts();
    hideAirJet();
    m_ability.reset();
}

void Enemy::configureForKind() {
    hideTankParts();
    hideAirJet();
    m_rig.setHidden(true);
    m_willUseShield = false;
    m_shieldTriggerHealth = 0;
    m_shieldActivated = false;
    m_shieldActivationsLeft = 0;
    m_shieldWasActive = false;
    m_mechUsesLaser = false;
    m_damageScale = 1.0f;

    const auto scaleHp = [this](int baseHp) {
        const int hp = static_cast<int>(
            lroundf(static_cast<float>(baseHp) * m_hpScale));
        return hp < 1 ? 1 : hp;
    };
    const auto scaleFireInterval = [this](float baseInterval) {
        float interval = baseInterval / m_fireRateScale;
        if (interval < 0.32f) {
            interval = 0.32f;
        }
        return interval;
    };

    switch (m_kind) {
        case EnemyKind::Tank:
            m_health = scaleHp(TANK_MAX_HEALTH);
            m_speed = 110.0f * m_speedScale;
            m_fireInterval = scaleFireInterval(1.4f);
            m_tankBodyMat.color = Colors::TANK_BODY;
            if (m_worldIndex <= EARLY_BOSS_MAX_WORLD_INDEX) {
                m_damageScale = EARLY_TANK_DAMAGE_SCALE;
            }
            break;
        case EnemyKind::Mech:
            m_health = scaleHp(MECH_MAX_HEALTH);
            m_speed = 160.0f * m_speedScale;
            m_rig.ensureBuilt(m_loadout, MechPalette::EnemyRed);
            m_rig.setHidden(false);
            if (m_worldIndex < MECH_TIER2_WORLD_INDEX) {
                m_mechUsesLaser = true;
                m_fireInterval = scaleFireInterval(MECH_LASER_FIRE_INTERVAL);
            } else {
                m_mechUsesLaser = false;
                m_fireInterval = scaleFireInterval(missileCooldownForTier(1));
                m_ability.equip(AbilityCatalog::SHIELD_ENEMY);
                m_ability.setSingleUse(true);
                m_ability.setShieldColor(Colors::SHIELD_ENEMY);
                if (Rng::nextFloat01() < m_shieldUseChance) {
                    m_willUseShield = true;
                    m_shieldTriggerHealth =
                        (2 + static_cast<int>(Rng::nextRange(3))) * 6;
                }
            }
            break;
        case EnemyKind::BossMech: {
            m_health = scaleHp(BOSS_MECH_MAX_HEALTH);
            if (m_worldIndex <= EARLY_BOSS_MAX_WORLD_INDEX) {
                m_health = static_cast<int>(lroundf(
                    static_cast<float>(m_health) * EARLY_BOSS_HP_SCALE));
                if (m_health < 1) {
                    m_health = 1;
                }
            }
            m_speed = 155.0f * m_speedScale;
            m_fireInterval = scaleFireInterval(1.25f);
            m_damageScale = BOSS_DAMAGE_SCALE;
            m_rig.ensureBuilt(m_loadout, MechPalette::BossBlue);
            m_rig.setHidden(false);
            m_ability.equip(AbilityCatalog::SHIELD_ENEMY);
            m_ability.setSingleUse(true);
            m_ability.setShieldColor(Colors::SHIELD_ENEMY);
            if (m_worldIndex <= EARLY_BOSS_MAX_WORLD_INDEX) {
                m_shieldActivationsLeft = 1;
            } else {
                m_shieldActivationsLeft = 1 + static_cast<int>(Rng::nextRange(2));
            }
            m_willUseShield = true;
            m_shieldTriggerHealth =
                (8 + static_cast<int>(Rng::nextRange(5))) * 6;
            break;
        }
        case EnemyKind::AirJet:
            m_health = scaleHp(AIR_MAX_HEALTH);
            m_speed = AIR_RUN_SPEED * m_speedScale;
            m_fireInterval = scaleFireInterval(1.8f);
            break;
    }
}

void Enemy::setAirRepositionWaypoint(float playerX, float playerZ, float playerAngle) {
    const float perpRad =
        (playerAngle + m_passSide * 90.0f) * static_cast<float>(M_PI) / 180.0f;
    m_waypointX = playerX + sinf(perpRad) * 400.0f;
    m_waypointZ = playerZ + cosf(perpRad) * 400.0f;
}

void Enemy::setupAirRun(float playerX, float playerZ, float playerAngle) {
    m_passSide = Rng::coinFlip() ? 1.0f : -1.0f;
    m_airPhase = AirPhase::Reposition;
    m_bombsThisRun = 0;
    m_bombDropCooldown = 0.0f;
    m_bankRoll = m_passSide * 18.0f;
    setAirRepositionWaypoint(playerX, playerZ, playerAngle);

    const float dx = m_waypointX - m_x;
    const float dz = m_waypointZ - m_z;
    m_angle = atan2f(dx, dz) * 180.0f / static_cast<float>(M_PI);
    m_runAngle = m_angle;
}

float Enemy::hoverBaseY() const {
    return Terrain::hoverHeightFast(m_x, m_z) + HOVER_OFFSET;
}

float Enemy::groundBaseY() const {
    return Terrain::groundHeightFast(m_x, m_z);
}

float Enemy::getWidth() const {
    switch (m_kind) {
        case EnemyKind::Tank: return TANK_WIDTH;
        case EnemyKind::AirJet: return AIR_JET_WIDTH;
        default: return m_loadout.hitWidth();
    }
}

float Enemy::getAimY() const {
    if (m_health <= 0) {
        if (m_kind == EnemyKind::AirJet) return FLIGHT_ALTITUDE;
        if (m_kind == EnemyKind::Tank) return groundBaseY();
        return hoverBaseY() + MECH_BODY_CENTER_Y;
    }
    if (m_kind == EnemyKind::Tank) {
        return groundBaseY() + TANK_TURRET_Y - 6.0f;
    }
    if (m_kind == EnemyKind::AirJet) {
        return m_baseY;
    }
    return m_baseY + MECH_BODY_CENTER_Y;
}

float Enemy::getMissileAimY() const {
    if (m_kind == EnemyKind::Mech || m_kind == EnemyKind::BossMech) {
        const float span = MECH_AIM_Y_MAX - MECH_AIM_Y_MIN;
        const float offset = MECH_AIM_Y_MIN + Rng::nextFloat01() * span;
        return m_baseY + offset;
    }
    return getAimY();
}

void Enemy::getHitVerticalRange(float& minY, float& maxY) const {
    if (m_kind == EnemyKind::Mech || m_kind == EnemyKind::BossMech) {
        const float base = (m_health > 0) ? m_baseY : hoverBaseY();
        minY = base + MECH_HIT_Y_MIN;
        maxY = base + MECH_HIT_Y_MAX;
        return;
    }

    const float aim = getAimY();
    if (m_kind == EnemyKind::AirJet) {
        minY = aim - 14.0f;
        maxY = aim + 14.0f;
        return;
    }

    minY = aim - 10.0f;
    maxY = aim + 10.0f;
}

void Enemy::syncRenderPivot() {
    m_renderX = static_cast<int32_t>(lroundf(m_x));
    m_renderZ = static_cast<int32_t>(lroundf(m_z));
    m_renderAngle = static_cast<int32_t>(lroundf(m_angle));
}

void Enemy::getLaserMuzzleWorld(float& wx, float& wy, float& wz) const {
    float lx = MECH_LASER_MUZZLE_X;
    float ly = MECH_LASER_MUZZLE_Y;
    float lz = MECH_LASER_MUZZLE_Z;
    m_rig.transformLocalToWorld(static_cast<float>(m_renderX),
                                static_cast<float>(m_renderZ),
                                m_baseY,
                                static_cast<float>(m_renderAngle),
                                m_loadout.visualPitch(), lx, ly, lz, wx, wy, wz);
}

void Enemy::getMissileMuzzleWorld(float& wx, float& wy, float& wz) const {
    float lx = MECH_MISSILE_MUZZLE_X;
    float ly = MECH_MISSILE_MUZZLE_Y;
    float lz = MECH_MISSILE_MUZZLE_Z;
    m_rig.transformLocalToWorld(static_cast<float>(m_renderX),
                                static_cast<float>(m_renderZ),
                                m_baseY,
                                static_cast<float>(m_renderAngle),
                                m_loadout.visualPitch(), lx, ly, lz, wx, wy, wz);
}

void Enemy::getMechMuzzleWorld(float& wx, float& wy, float& wz) const {
    if (m_mechUsesLaser) {
        getLaserMuzzleWorld(wx, wy, wz);
    } else {
        getMissileMuzzleWorld(wx, wy, wz);
    }
}

void Enemy::getTankMuzzleWorld(float& wx, float& wy, float& wz) const {
    const float radians = m_angle * M_PI / 180.0f;
    wx = m_x + sinf(radians) * TANK_MUZZLE_OFFSET;
    wz = m_z + cosf(radians) * TANK_MUZZLE_OFFSET;
    wy = groundBaseY() + TANK_TURRET_Y;
}

void Enemy::spawnInRing(float playerX, float playerZ, float playerAngle, uint32_t spawnIndex,
                        AIState state, const ObstacleField* obstacles) {
    const uint32_t spawnSalt = Rng::nextU32();
    WorldGen::sampleEnemySpawn(playerX, playerZ, playerAngle, spawnIndex, spawnSalt,
                               m_mapConfig, obstacles, m_x, m_z);

    m_active = true;
    m_kind = pickKind();

    float dx = playerX - m_x;
    float dz = playerZ - m_z;
    m_angle = atan2f(dx, dz) * 180.0f / M_PI;
    m_patrolAngle = m_angle;
    m_state = state;
    m_stateTimer = 0;
    m_isIdle = false;
    m_idleTimer = 0;
    m_fireTimer = 0;
    m_behindDespawnGrace = SPAWN_BEHIND_DESPAWN_GRACE;

    configureForKind();

    if (m_kind == EnemyKind::Mech) {
        m_smoothedHoverY = hoverBaseY();
        syncRenderPivot();
        m_baseY = m_smoothedHoverY;
    } else if (m_kind == EnemyKind::AirJet) {
        m_baseY = FLIGHT_ALTITUDE;
        syncRenderPivot();
        setupAirRun(playerX, playerZ, playerAngle);
    } else {
        m_baseY = groundBaseY();
        syncRenderPivot();
    }

    updateVisual(0.0f);
}

void Enemy::spawnAsPortalBoss(float portalX, float portalZ, float portalAngle,
                              float playerX, float playerZ) {
    hide();

    const float side = Rng::coinFlip() ? 1.0f : -1.0f;
    const float perpRad =
        (portalAngle + side * 75.0f) * static_cast<float>(M_PI) / 180.0f;
    const float offsetDist = 72.0f + Rng::nextFloat01() * 28.0f;
    m_x = portalX + sinf(perpRad) * offsetDist;
    m_z = portalZ + cosf(perpRad) * offsetDist;

    m_active = true;
    m_kind = EnemyKind::BossMech;
    m_spawnIndex = 0;

    const float dx = playerX - m_x;
    const float dz = playerZ - m_z;
    m_angle = atan2f(dx, dz) * 180.0f / static_cast<float>(M_PI);
    m_patrolAngle = m_angle;
    m_state = AIState::ATTACK;
    m_stateTimer = 0;
    m_isIdle = false;
    m_idleTimer = 0;
    m_fireTimer = m_fireInterval * 0.35f;

    configureForKind();

    m_smoothedHoverY = hoverBaseY();
    syncRenderPivot();
    m_baseY = m_smoothedHoverY;
    updateVisual(0.0f);
}

bool Enemy::isBehindPlayer(float playerX, float playerZ, float playerAngle) const {
    float radians = playerAngle * M_PI / 180.0f;
    float forwardX = sinf(radians);
    float forwardZ = cosf(radians);
    float dx = m_x - playerX;
    float dz = m_z - playerZ;
    return (dx * forwardX + dz * forwardZ) < -DESPAWN_BEHIND_DIST;
}

void Enemy::setWorldScaling(float speedScale, float shieldUseChance,
                            float hpScale, float fireRateScale,
                            float engageRange, float airStrikeRange, int worldIndex) {
    m_speedScale = speedScale;
    m_shieldUseChance = shieldUseChance;
    m_hpScale = hpScale;
    m_fireRateScale = fireRateScale;
    m_engageRange = engageRange;
    m_airStrikeRange = airStrikeRange;
    m_worldIndex = worldIndex;
}

void Enemy::deactivate() {
    m_health = 0;
    m_active = false;
    hide();
}

void Enemy::reset(float playerX, float playerZ, float playerAngle,
                  const ObstacleField* obstacles, uint32_t spawnIndex) {
    m_spawnIndex = spawnIndex;
    spawnInRing(playerX, playerZ, playerAngle, m_spawnIndex, AIState::CHASE, obstacles);
}

void Enemy::startIdle(float duration) {
    m_isIdle = true;
    m_idleTimer = duration;
}

void Enemy::update(float deltaTime, float playerX, float playerZ, float playerAngle,
                   float playerAimY, const ObstacleField* obstacles) {
    if (!m_active || m_health <= 0) return;

    if (m_kind != EnemyKind::BossMech) {
        if (m_behindDespawnGrace > 0.0f) {
            m_behindDespawnGrace -= deltaTime;
        } else if (isBehindPlayer(playerX, playerZ, playerAngle)) {
            deactivate();
            return;
        }

        const float relX = m_x - playerX;
        const float relZ = m_z - playerZ;
        if (relX * relX + relZ * relZ > DESPAWN_FAR_DIST * DESPAWN_FAR_DIST) {
            deactivate();
            return;
        }
    }

    if (m_kind == EnemyKind::AirJet) {
        updateAirAI(deltaTime, playerX, playerZ, playerAngle, playerAimY);
    } else {
        updateAI(deltaTime, playerX, playerZ, playerAimY, obstacles);
    }
    updateVisual(deltaTime);
}

void Enemy::updateAirAI(float deltaTime, float playerX, float playerZ, float playerAngle,
                        float playerAimY) {
    (void)playerAimY;

    const float dx = playerX - m_x;
    const float dz = playerZ - m_z;
    const float distToPlayer = sqrtf(dx * dx + dz * dz);
    const float angleToPlayer =
        atan2f(dx, dz) * 180.0f / static_cast<float>(M_PI);

    const float wdx = m_waypointX - m_x;
    const float wdz = m_waypointZ - m_z;
    const float distToWaypoint = sqrtf(wdx * wdx + wdz * wdz);
    const float angleToWaypoint =
        atan2f(wdx, wdz) * 180.0f / static_cast<float>(M_PI);

    m_bombDropCooldown -= deltaTime;

    float targetAngle = m_angle;
    float moveSpeed = AIR_TURN_SPEED;

    switch (m_airPhase) {
        case AirPhase::Reposition:
            targetAngle = angleToWaypoint;
            if (distToWaypoint < 120.0f ||
                (distToPlayer > 260.0f && fabsf(wrapAngleDeg(angleToPlayer - m_angle)) < 35.0f)) {
                m_airPhase = AirPhase::Approach;
                m_runAngle = angleToPlayer;
                m_bombsThisRun = 0;
                m_bombDropCooldown = 0.0f;
                targetAngle = m_runAngle;
                moveSpeed = AIR_RUN_SPEED;
            }
            break;

        case AirPhase::Approach:
            targetAngle = m_runAngle;
            moveSpeed = AIR_RUN_SPEED;
            if (distToPlayer < m_airStrikeRange) {
                m_airPhase = AirPhase::Strike;
            }
            break;

        case AirPhase::Strike: {
            targetAngle = m_runAngle;
            moveSpeed = AIR_RUN_SPEED;

            if (distToPlayer < 150.0f && m_bombDropCooldown <= 0.0f &&
                m_bombsThisRun < AIR_BOMBS_PER_RUN) {
                m_projectiles.fireEnemyBomb(
                    m_x, m_baseY - 6.0f, m_z, playerX, playerZ);
                m_bombsThisRun++;
                m_bombDropCooldown = AIR_BOMB_INTERVAL / m_fireRateScale;
                if (m_bombDropCooldown < 0.22f) {
                    m_bombDropCooldown = 0.22f;
                }
            }

            const float fwdX = sinf(playerAngle * static_cast<float>(M_PI) / 180.0f);
            const float fwdZ = cosf(playerAngle * static_cast<float>(M_PI) / 180.0f);
            const float relX = m_x - playerX;
            const float relZ = m_z - playerZ;
            const float along = relX * fwdX + relZ * fwdZ;
            if (along > 70.0f && distToPlayer < 260.0f) {
                m_airPhase = AirPhase::Exit;
            }
            break;
        }

        case AirPhase::Exit:
            targetAngle = m_runAngle;
            moveSpeed = AIR_RUN_SPEED;
            if (distToPlayer > 420.0f) {
                m_passSide *= -1.0f;
                setAirRepositionWaypoint(playerX, playerZ, playerAngle);
                m_airPhase = AirPhase::Reposition;
                targetAngle = angleToWaypoint;
                moveSpeed = AIR_TURN_SPEED;
            }
            break;
    }

    float angleDiff = wrapAngleDeg(targetAngle - m_angle);

    if (m_airPhase == AirPhase::Approach ||
        m_airPhase == AirPhase::Strike ||
        m_airPhase == AirPhase::Exit) {
        m_angle = m_runAngle;
        m_bankRoll = m_passSide * 16.0f;
    } else {
        const float turnRate = 6.5f;
        m_angle += angleDiff * turnRate * deltaTime;
        angleDiff = wrapAngleDeg(targetAngle - m_angle);
        m_bankRoll = angleDiff * 0.55f;
        if (m_bankRoll > 38.0f) m_bankRoll = 38.0f;
        else if (m_bankRoll < -38.0f) m_bankRoll = -38.0f;
    }

    const float radians = m_angle * static_cast<float>(M_PI) / 180.0f;
    m_x += sinf(radians) * moveSpeed * deltaTime;
    m_z += cosf(radians) * moveSpeed * deltaTime;
}

void Enemy::updateAI(float deltaTime, float playerX, float playerZ, float playerAimY,
                     const ObstacleField* obstacles) {
    float dx = playerX - m_x;
    float dz = playerZ - m_z;
    float distToPlayer = sqrtf(dx * dx + dz * dz);
    float angleToPlayer = atan2f(dx, dz) * 180.0f / M_PI;

    m_stateTimer += deltaTime;
    m_fireTimer += deltaTime;

    if (m_isIdle) {
        m_idleTimer -= deltaTime;
        if (m_idleTimer <= 0) {
            m_isIdle = false;
        }
    }

    if (distToPlayer < m_engageRange) {
        m_state = AIState::ATTACK;
    }

    float targetAngle = m_angle;
    float moveSpeed = 0;

    switch (m_state) {
        case AIState::PATROL:
            targetAngle = m_patrolAngle;
            moveSpeed = m_speed * 0.5f;
            break;

        case AIState::CHASE:
            targetAngle = angleToPlayer;
            if (distToPlayer > PREFERRED_DIST_MAX) {
                moveSpeed = m_speed * 0.55f;
            } else {
                m_state = AIState::ATTACK;
            }
            break;

        case AIState::ATTACK:
            targetAngle = angleToPlayer;
            if (distToPlayer < PREFERRED_DIST_MIN) {
                targetAngle = angleToPlayer + 180.0f;
                moveSpeed = m_speed * 0.4f;
            } else if (distToPlayer > PREFERRED_DIST_MAX) {
                moveSpeed = m_speed * 0.45f;
            } else {
                moveSpeed = 0;
                if (m_stateTimer > 1.0f && Rng::nextRange(100) < 45) {
                    startIdle(0.8f + Rng::nextRange(60) / 100.0f);
                    m_stateTimer = 0;
                }
            }

            if (m_fireTimer >= m_fireInterval && distToPlayer < m_engageRange) {
                m_fireTimer = 0;
                if (m_kind == EnemyKind::Mech || m_kind == EnemyKind::BossMech) {
                    float mx, my, mz;
                    if (m_kind == EnemyKind::Mech && m_mechUsesLaser) {
                        getLaserMuzzleWorld(mx, my, mz);
                        m_projectiles.fireEnemyLaserAtTarget(
                            mx, my, mz, playerX, playerZ, playerAimY, m_damageScale);
                    } else {
                        getMissileMuzzleWorld(mx, my, mz);
                        m_projectiles.fireEnemyHomingAtTarget(
                            mx, my, mz, playerX, playerZ, playerAimY, m_damageScale);
                    }
                } else {
                    float mx, my, mz;
                    getTankMuzzleWorld(mx, my, mz);
                    (void)my;
                    m_projectiles.fireEnemyAtTarget(mx, mz, playerX, playerZ, m_damageScale);
                }
            }
            break;
    }

    float angleDiff = targetAngle - m_angle;
    while (angleDiff > 180) angleDiff -= 360;
    while (angleDiff < -180) angleDiff += 360;

    if (moveSpeed > 0 && !m_isIdle) {
        const float radians = targetAngle * M_PI / 180.0f;
        const float newX = m_x + sinf(radians) * moveSpeed * deltaTime;
        const float newZ = m_z + cosf(radians) * moveSpeed * deltaTime;
        if (obstacles) {
            obstacles->resolveMovement(m_x, m_z, newX, newZ, ENEMY_COLLISION_RADIUS);
        } else {
            m_x = newX;
            m_z = newZ;
        }
        m_angle = targetAngle;
    } else {
        m_angle += angleDiff * 8.0f * deltaTime;
    }
}

void Enemy::updateBossShield(float deltaTime) {
    (void)deltaTime;
    if (m_kind != EnemyKind::BossMech) {
        return;
    }

    if (m_ability.isActive()) {
        m_shieldWasActive = true;
        return;
    }

    if (!m_shieldWasActive || m_ability.isBreakAnimating()) {
        return;
    }

    m_shieldWasActive = false;
    m_shieldActivated = false;
    m_shieldActivationsLeft--;
    if (m_shieldActivationsLeft > 0) {
        m_ability.rearmAfterShieldBreak();
        m_willUseShield = true;
        m_shieldTriggerHealth =
            (1 + static_cast<int>(Rng::nextRange(3))) * 6;
    } else {
        m_willUseShield = false;
    }
}

void Enemy::updateVisual(float deltaTime) {
    if (m_health <= 0) return;

    syncRenderPivot();

    if (m_kind == EnemyKind::Mech || m_kind == EnemyKind::BossMech) {
        const float targetY = hoverBaseY();
        if (deltaTime > 0.0f) {
            const float blend = fminf(1.0f, 0.75f * deltaTime);
            m_smoothedHoverY += (targetY - m_smoothedHoverY) * blend;
        } else {
            m_smoothedHoverY = targetY;
        }
        m_baseY = m_smoothedHoverY;
        hideTankParts();
        hideAirJet();
        m_rig.updatePose(m_renderX, m_renderZ, m_baseY, m_renderAngle,
                         m_loadout.visualPitch(), m_loadout);
        m_ability.update(deltaTime, m_rig, m_renderX, m_renderZ, m_baseY,
                         m_renderAngle, m_loadout.visualPitch());
        updateBossShield(deltaTime);
        return;
    }

    if (m_kind == EnemyKind::AirJet) {
        m_baseY = FLIGHT_ALTITUDE;
        m_rig.setHidden(true);
        hideTankParts();
        showSceneObject(m_airJet, m_renderX,
                        static_cast<int32_t>(lroundf(m_baseY)), m_renderZ);
        m_airJet->setRotation(AIR_JET_NOSE_PITCH, m_renderAngle,
                              static_cast<int16_t>(lroundf(m_bankRoll)));
        return;
    }

    const float baseY = groundBaseY();
    m_baseY = baseY;
    m_rig.setHidden(true);
    hideAirJet();

    const int16_t ix = m_renderX;
    const int16_t iz = m_renderZ;
    const int16_t iy = static_cast<int16_t>(baseY);
    const int16_t angle = static_cast<int16_t>(m_angle);

    const int16_t turretY = static_cast<int16_t>(baseY + TANK_TURRET_Y);
    const float radians = m_angle * M_PI / 180.0f;
    const float barrelX = m_x + sinf(radians) * TANK_BARREL_OFFSET;
    const float barrelZ = m_z + cosf(radians) * TANK_BARREL_OFFSET;

    showSceneObject(m_tankBody, ix, iy, iz);
    m_tankBody->setRotation(0, angle, 0);
    showSceneObject(m_tankTurret, ix, turretY, iz);
    m_tankTurret->setRotation(0, angle, 0);
    showSceneObject(m_tankBarrel,
                    static_cast<int32_t>(lroundf(barrelX)), turretY,
                    static_cast<int32_t>(lroundf(barrelZ)));
    m_tankBarrel->setRotation(0, angle, 0);
}

int Enemy::takeDamage(int damage, bool* outHitShield) {
    if (outHitShield) {
        *outHitShield = false;
    }
    if (!m_active || m_health <= 0 || damage <= 0) {
        return 0;
    }

    if (m_kind == EnemyKind::Mech || m_kind == EnemyKind::BossMech) {
        if (m_willUseShield && !m_shieldActivated && m_ability.isReady() &&
            m_health - damage <= m_shieldTriggerHealth) {
            if (m_ability.tryActivate()) {
                m_shieldActivated = true;
            }
        }
        const ShieldDamageResult absorbed = m_ability.absorbDamage(damage);
        if (outHitShield) {
            *outHitShield = absorbed.hitShield;
        }
        damage = absorbed.healthDamage;
    }

    if (damage <= 0) {
        return 0;
    }

    m_health -= damage;
    if (m_health <= 0) {
        deactivate();
    }
    return damage;
}

} // namespace Game
