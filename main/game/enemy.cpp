#include "enemy.hpp"
#include "terrain.hpp"
#include "worldgen.hpp"
#include <cmath>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Game {

Enemy::Enemy(Renderer::Scene& scene, ProjectileSystem& projectiles,
             const MapConfig& mapConfig)
    : m_scene(scene)
    , m_projectiles(projectiles)
    , m_mapConfig(mapConfig)
    , m_hoverMat(Colors::ENEMY_BODY)
    , m_tankBodyMat(Colors::TANK_BODY)
    , m_tankTurretMat(Colors::TANK_TURRET)
{
    m_hoverMat.shadingMode = Renderer::ShadingMode::GOURAUD;
    m_tankBodyMat.shadingMode = Renderer::ShadingMode::GOURAUD;
    m_tankTurretMat.shadingMode = Renderer::ShadingMode::GOURAUD;

    m_hoverBody = Primitives::createPyramid(26, 44, &m_hoverMat);
    m_tankBody = Primitives::createCube(36, 14, 28, &m_tankBodyMat);
    m_tankTurret = Primitives::createCube(18, 10, 18, &m_tankTurretMat);

    m_scene.addObject(m_hoverBody);
    m_scene.addObject(m_tankBody);
    m_scene.addObject(m_tankTurret);
    hideAllParts();
}

void Enemy::hideAllParts() {
    if (m_hoverBody) m_hoverBody->setPosition(0, -1000, 0);
    if (m_tankBody) m_tankBody->setPosition(0, -1000, 0);
    if (m_tankTurret) m_tankTurret->setPosition(0, -1000, 0);
}

void Enemy::hide() {
    hideAllParts();
}

float Enemy::visualBaseY() const {
    return (m_locomotion == Locomotion::HOVER)
        ? Terrain::hoverHeight(m_x, m_z)
        : Terrain::groundHeight(m_x, m_z);
}

float Enemy::getAimY() const {
    if (m_health <= 0) return visualBaseY();
    const float baseY = visualBaseY();
    return (m_locomotion == Locomotion::HOVER) ? baseY : (baseY + 6.0f);
}

void Enemy::spawnInRing(float playerX, float playerZ, uint32_t spawnIndex, AIState state) {
    WorldGen::sampleEnemySpawn(playerX, playerZ, spawnIndex, m_mapConfig, m_x, m_z);

    float dx = playerX - m_x;
    float dz = playerZ - m_z;
    m_angle = atan2f(dx, dz) * 180.0f / M_PI;
    m_patrolAngle = m_angle;
    m_health = MAX_HEALTH;
    m_state = state;
    m_stateTimer = 0;
    m_isIdle = false;
    m_idleTimer = 0;
    m_fireTimer = 0;

    m_locomotion = (rand() % 2) ? Locomotion::HOVER : Locomotion::GROUND;
    m_speed = (m_locomotion == Locomotion::GROUND) ? 110.0f : 160.0f;

    updateVisual();
}

bool Enemy::isBehindPlayer(float playerX, float playerZ, float playerAngle) const {
    float radians = playerAngle * M_PI / 180.0f;
    float forwardX = sinf(radians);
    float forwardZ = cosf(radians);
    float dx = m_x - playerX;
    float dz = m_z - playerZ;
    return (dx * forwardX + dz * forwardZ) < -DESPAWN_BEHIND_DIST;
}

void Enemy::reset(float playerX, float playerZ, float playerAngle, uint32_t spawnIndex) {
    (void)playerAngle;
    m_spawnIndex = spawnIndex;
    spawnInRing(playerX, playerZ, m_spawnIndex, AIState::CHASE);
}

void Enemy::startIdle(float duration) {
    m_isIdle = true;
    m_idleTimer = duration;
}

void Enemy::update(float deltaTime, float playerX, float playerZ, float playerAngle) {
    if (m_health <= 0) return;

    if (isBehindPlayer(playerX, playerZ, playerAngle)) {
        m_spawnIndex++;
        spawnInRing(playerX, playerZ, m_spawnIndex, AIState::CHASE);
        return;
    }

    updateAI(deltaTime, playerX, playerZ);
    updateVisual();
}

void Enemy::updateAI(float deltaTime, float playerX, float playerZ) {
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

    if (distToPlayer < ENGAGE_RANGE) {
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
                if (m_stateTimer > 1.0f && (rand() % 100) < 45) {
                    startIdle(0.8f + (rand() % 60) / 100.0f);
                    m_stateTimer = 0;
                }
            }

            if (m_fireTimer >= m_fireInterval && distToPlayer < ENGAGE_RANGE) {
                m_fireTimer = 0;
                m_projectiles.fireEnemyAtTarget(m_x, m_z, playerX, playerZ);
            }
            break;
    }

    float angleDiff = targetAngle - m_angle;
    while (angleDiff > 180) angleDiff -= 360;
    while (angleDiff < -180) angleDiff += 360;

    if (moveSpeed > 0 && !m_isIdle) {
        float radians = targetAngle * M_PI / 180.0f;
        m_x += sinf(radians) * moveSpeed * deltaTime;
        m_z += cosf(radians) * moveSpeed * deltaTime;
        m_angle = targetAngle;
    } else {
        m_angle += angleDiff * 8.0f * deltaTime;
    }
}

void Enemy::updateVisual() {
    if (m_health <= 0) return;

    const float baseY = visualBaseY();
    const int16_t ix = (int16_t)m_x;
    const int16_t iz = (int16_t)m_z;
    const int16_t iy = (int16_t)baseY;

    if (m_locomotion == Locomotion::HOVER) {
        m_hoverBody->setPosition(ix, iy, iz);
        m_hoverBody->setRotation(VISUAL_PITCH, (int16_t)m_angle, 0);
        m_tankBody->setPosition(0, -1000, 0);
        m_tankTurret->setPosition(0, -1000, 0);
    } else {
        m_tankBody->setPosition(ix, iy, iz);
        m_tankTurret->setPosition(ix, (int16_t)(baseY + 12.0f), iz);
        m_tankBody->setRotation(0, (int16_t)m_angle, 0);
        m_tankTurret->setRotation(0, (int16_t)m_angle, 0);
        m_hoverBody->setPosition(0, -1000, 0);
    }
}

void Enemy::takeDamage() {
    if (m_health <= 0) return;
    m_health--;
    if (m_health <= 0) {
        hide();
    }
}

} // namespace Game
