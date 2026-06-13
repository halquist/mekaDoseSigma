#include "air_strike.hpp"
#include "portal.hpp"
#include "run_upgrades.hpp"
#include "scene_util.hpp"
#include "terrain.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Game {

namespace {

float smoothStep(float t) {
    if (t <= 0.0f) {
        return 0.0f;
    }
    if (t >= 1.0f) {
        return 1.0f;
    }
    return t * t * (3.0f - 2.0f * t);
}

} // namespace

AirStrikeSystem::AirStrikeSystem(Renderer::Scene& scene)
    : m_scene(scene)
    , m_aimMat(Colors::DAMAGE_RED)
    , m_missileMat(Colors::MISSILE_GREY)
    , m_explosionMat(Colors::SPARK_ORANGE, EXPLOSION_PEAK_ALPHA)
{
    m_aimMat.shadingMode = Renderer::ShadingMode::UNLIT;
    m_aimMat.emissive = true;
    m_missileMat.shadingMode = Renderer::ShadingMode::UNLIT;
    m_explosionMat.shadingMode = Renderer::ShadingMode::UNLIT;
    m_explosionMat.emissive = true;

    m_aimLaser = Primitives::createCube(AIM_LASER_W, AIM_LASER_H, AIM_LASER_LEN, &m_aimMat);
    m_aimLaser->cullingMode = Renderer::CullingMode::NO_CULLING;
    m_scene.addObject(m_aimLaser);

    m_missile = Primitives::createCube(MISSILE_VIS_W, MISSILE_VIS_H, MISSILE_VIS_LEN,
                                       &m_missileMat);
    m_missile->cullingMode = Renderer::CullingMode::NO_CULLING;
    m_scene.addObject(m_missile);

    m_explosion = Primitives::createSphere(EXPLOSION_BASE_RADIUS, EXPLOSION_SPHERE_SEGMENTS,
                                           &m_explosionMat);
    m_explosion->cullingMode = Renderer::CullingMode::NO_CULLING;
    m_explosion->ignoreZBuffer = true;
    m_explosionBaseVerts.reserve(m_explosion->vertices.size());
    for (const auto& v : m_explosion->vertices) {
        m_explosionBaseVerts.push_back({v.position.x, v.position.y, v.position.z});
    }
    m_scene.addObject(m_explosion);

    hideVisuals();
}

void AirStrikeSystem::hideVisuals() {
    stashSceneObject(m_aimLaser);
    stashSceneObject(m_missile);
    stashSceneObject(m_explosion);
}

void AirStrikeSystem::reset() {
    m_state = State::Idle;
    m_cooldown = 0.0f;
    m_phaseTimer = 0.0f;
    m_activeTier = 0;
    hideVisuals();
}

void AirStrikeSystem::onTierEquipped(uint8_t tier, uint8_t previousTier) {
    m_activeTier = tier;
    if (previousTier == 0) {
        m_cooldown = airStrikeCooldownForTier(tier);
        m_state = State::Idle;
        m_phaseTimer = 0.0f;
        hideVisuals();
    }
}

void AirStrikeSystem::showAimLaser(float strikeX, float strikeZ, float skyY, float groundY) {
    const float midY = (skyY + groundY) * 0.5f;
    m_aimLaser->setRotation(-90, 0, 0);
    showSceneObject(m_aimLaser,
                    static_cast<int32_t>(lroundf(strikeX)),
                    static_cast<int32_t>(lroundf(midY)),
                    static_cast<int32_t>(lroundf(strikeZ)));
}

void AirStrikeSystem::showMissile(float y) {
    m_missile->setRotation(-90, 0, 0);
    showSceneObject(m_missile,
                    static_cast<int32_t>(lroundf(m_strikeX)),
                    static_cast<int32_t>(lroundf(y)),
                    static_cast<int32_t>(lroundf(m_strikeZ)));
}

void AirStrikeSystem::applyExplosionScale(float scale) {
    if (!m_explosion || m_explosionBaseVerts.empty()) {
        return;
    }

    const size_t count = m_explosionBaseVerts.size();
    for (size_t i = 0; i < count; ++i) {
        auto& pos = m_explosion->vertices[i].position;
        pos.x = static_cast<int32_t>(lroundf(static_cast<float>(m_explosionBaseVerts[i].x) * scale));
        pos.y = static_cast<int32_t>(lroundf(static_cast<float>(m_explosionBaseVerts[i].y) * scale));
        pos.z = static_cast<int32_t>(lroundf(static_cast<float>(m_explosionBaseVerts[i].z) * scale));
    }
    m_explosion->calculateBoundingBox();
}

bool AirStrikeSystem::refreshStrikeTarget(uint8_t tier, float mechX, float mechZ,
                                          float mechAngle, float skyBaseY,
                                          EnemyManager& enemies) {
    const float range = airStrikeRangeForTier(tier);
    const float aimCone = airStrikeAimConeDeg();
    const float radius = airStrikeRadiusForTier(tier);
    float targetX = 0.0f;
    float targetZ = 0.0f;
    float targetAimY = 0.0f;
    if (!enemies.findBestStrikeTarget(mechX, mechZ, mechAngle, range, aimCone,
                                      radius, targetX, targetZ, targetAimY)) {
        return false;
    }

    m_strikeX = targetX;
    m_strikeZ = targetZ;
    m_strikeAimY = targetAimY;
    const float groundY = Terrain::hoverHeightFast(m_strikeX, m_strikeZ);
    m_skyY = skyBaseY;
    if (m_skyY < groundY + 120.0f) {
        m_skyY = groundY + 120.0f;
    }
    return true;
}

void AirStrikeSystem::applySplash(float strikeX, float strikeZ, float strikeAimY, int damage,
                                  float radius, EnemyManager& enemies,
                                  ParticleSystem& particles, RunScore& score,
                                  WorldPortal* portal) {
    const float radiusSq = radius * radius;

    const auto tryHit = [&](Enemy& enemy) {
        if (!enemy.isAlive() || enemy.isAirborne()) {
            return;
        }
        const float dx = enemy.getX() - strikeX;
        const float dz = enemy.getZ() - strikeZ;
        if (dx * dx + dz * dz > radiusSq) {
            return;
        }

        const bool wasAlive = enemy.isAlive();
        bool hitEnemyShield = false;
        enemy.takeDamage(damage, &hitEnemyShield);
        particles.spawnHitEffect(
            enemy.getX(), enemy.getAimY(), enemy.getZ(),
            hitEnemyShield ? Colors::SHIELD_ENEMY : Colors::SPARK_ORANGE);
        if (wasAlive && !enemy.isAlive()) {
            score.kills++;
            particles.spawnDeathEffect(enemy.getX(), enemy.getAimY(), enemy.getZ());
            if (enemy.isPortalBoss()) {
                if (portal && portal->isActive() && portal->isLocked()) {
                    portal->setLocked(false);
                }
                enemies.onPortalBossDefeated();
            }
        }
    };

    for (int i = 0; i < EnemyManager::MAX_ENEMIES; ++i) {
        tryHit(enemies.enemy(i));
    }

    Enemy* portalBoss = enemies.portalBoss();
    if (portalBoss) {
        tryHit(*portalBoss);
    }
}

void AirStrikeSystem::update(float deltaTime, uint8_t tier, float mechX, float mechZ,
                             float mechAngle, float skyBaseY, EnemyManager& enemies,
                             ParticleSystem& particles, RunScore& score,
                             WorldPortal* portal) {
    if (tier == 0) {
        return;
    }

    m_activeTier = tier;

    switch (m_state) {
    case State::Idle:
        if (m_cooldown > 0.0f) {
            m_cooldown -= deltaTime;
            if (m_cooldown < 0.0f) {
                m_cooldown = 0.0f;
            }
        }

        if (m_cooldown > 0.0f) {
            break;
        }

        if (!refreshStrikeTarget(tier, mechX, mechZ, mechAngle, skyBaseY, enemies)) {
            break;
        }

        {
            const float groundY = Terrain::hoverHeightFast(m_strikeX, m_strikeZ);
            showAimLaser(m_strikeX, m_strikeZ, m_skyY, groundY);
            m_phaseTimer = AIM_DURATION_SEC;
            m_state = State::Aiming;
        }
        break;

    case State::Aiming:
        if (!refreshStrikeTarget(tier, mechX, mechZ, mechAngle, skyBaseY, enemies)) {
            stashSceneObject(m_aimLaser);
            m_state = State::Idle;
            break;
        }

        {
            const float groundY = Terrain::hoverHeightFast(m_strikeX, m_strikeZ);
            showAimLaser(m_strikeX, m_strikeZ, m_skyY, groundY);
        }

        m_phaseTimer -= deltaTime;
        if (m_phaseTimer > 0.0f) {
            break;
        }

        stashSceneObject(m_aimLaser);
        if (!refreshStrikeTarget(tier, mechX, mechZ, mechAngle, skyBaseY, enemies)) {
            m_state = State::Idle;
            break;
        }

        m_missileY = m_skyY;
        m_missileVy = 0.0f;
        showMissile(m_missileY);
        m_state = State::Incoming;
        break;

    case State::Incoming:
        m_missileVy -= MISSILE_GRAVITY * deltaTime;
        m_missileY += m_missileVy * deltaTime;

        {
            const float groundY = Terrain::hoverHeightFast(m_strikeX, m_strikeZ);
            if (m_missileY <= groundY + 3.0f) {
                stashSceneObject(m_missile);
                m_explodeRadius = airStrikeRadiusForTier(m_activeTier);
                m_explodeDamage = airStrikeDamageForTier(m_activeTier);
                applySplash(m_strikeX, m_strikeZ, m_strikeAimY,
                            m_explodeDamage, m_explodeRadius,
                            enemies, particles, score, portal);
                m_phaseTimer = 0.0f;
                m_explosionMat.alpha = EXPLOSION_PEAK_ALPHA;
                applyExplosionScale(0.01f);
                showSceneObject(m_explosion,
                                static_cast<int32_t>(lroundf(m_strikeX)),
                                static_cast<int32_t>(lroundf(groundY + 2.0f)),
                                static_cast<int32_t>(lroundf(m_strikeZ)));
                m_state = State::Exploding;
                break;
            }

            showMissile(m_missileY);
        }
        break;

    case State::Exploding:
        m_phaseTimer += deltaTime;
        {
            const float t = smoothStep(m_phaseTimer / EXPLODE_DURATION_SEC);
            const float targetScale =
                m_explodeRadius / static_cast<float>(EXPLOSION_BASE_RADIUS);
            applyExplosionScale(0.01f + t * targetScale);
            m_explosionMat.alpha =
                static_cast<uint8_t>(EXPLOSION_PEAK_ALPHA * (1.0f - t));

            const float groundY = Terrain::hoverHeightFast(m_strikeX, m_strikeZ);
            showSceneObject(m_explosion,
                            static_cast<int32_t>(lroundf(m_strikeX)),
                            static_cast<int32_t>(lroundf(groundY + 2.0f)),
                            static_cast<int32_t>(lroundf(m_strikeZ)));

            if (m_phaseTimer >= EXPLODE_DURATION_SEC) {
                stashSceneObject(m_explosion);
                m_cooldown = airStrikeCooldownForTier(m_activeTier);
                m_state = State::Idle;
            }
        }
        break;
    }
}

} // namespace Game
