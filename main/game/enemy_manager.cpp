#include "enemy_manager.hpp"
#include "world_tier.hpp"
#include "rng.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Game {

EnemyManager::EnemyManager(Renderer::Scene& scene, ProjectileSystem& projectiles,
                           const MapConfig& mapConfig) {
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        m_enemies[static_cast<size_t>(i)] =
            new Enemy(scene, projectiles, mapConfig);
    }
    m_portalBoss = new Enemy(scene, projectiles, mapConfig);
}

EnemyManager::~EnemyManager() {
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        delete m_enemies[static_cast<size_t>(i)];
        m_enemies[static_cast<size_t>(i)] = nullptr;
    }
    delete m_portalBoss;
    m_portalBoss = nullptr;
}

void EnemyManager::applyWorldTier(const WorldTier& tier) {
    m_spawnIntervalMin = tier.spawnIntervalMin();
    m_spawnIntervalJitter = tier.spawnIntervalJitter();
    m_initialSpawnDelay = tier.initialSpawnDelay();
    m_refillSpawnDelay = tier.refillSpawnDelay();
    m_maxEnemies = tier.maxEnemies();

    for (int i = 0; i < MAX_ENEMIES; ++i) {
        m_enemies[static_cast<size_t>(i)]->setWorldScaling(
            tier.enemySpeedScale(), tier.enemyShieldUseChance(),
            tier.enemyHpScale(), tier.enemyFireRateScale());
    }
    if (m_portalBoss) {
        m_portalBoss->setWorldScaling(
            tier.enemySpeedScale(), tier.enemyShieldUseChance(),
            tier.enemyHpScale(), tier.enemyFireRateScale());
    }
}

void EnemyManager::reset(float playerX, float playerZ, float playerAngle) {
    m_spawnIndex = 0;
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        m_enemies[static_cast<size_t>(i)]->deactivate();
    }
    dismissPortalBoss();
    spawnOne(playerX, playerZ, playerAngle);
    m_spawnTimer = m_initialSpawnDelay;
    m_lastAliveCount = aliveCount();
}

void EnemyManager::spawnPortalBoss(float portalX, float portalZ, float portalAngle,
                                   float playerX, float playerZ) {
    if (!m_portalBoss) {
        return;
    }
    m_portalBoss->spawnAsPortalBoss(portalX, portalZ, portalAngle, playerX, playerZ);
}

void EnemyManager::dismissPortalBoss() {
    if (m_portalBoss) {
        m_portalBoss->deactivate();
    }
}

void EnemyManager::clearAllEnemies() {
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        m_enemies[static_cast<size_t>(i)]->deactivate();
    }
    dismissPortalBoss();
    m_lastAliveCount = 0;
}

void EnemyManager::setSpawnPaused(bool paused) {
    m_spawnPaused = paused;
}

void EnemyManager::prepareSpawnAfterTransition(float playerX, float playerZ,
                                               float playerAngle) {
    (void)playerX;
    (void)playerZ;
    (void)playerAngle;
    m_spawnTimer = m_initialSpawnDelay;
    m_lastAliveCount = 0;
}

bool EnemyManager::isPortalBossAlive() const {
    return m_portalBoss && m_portalBoss->isAlive();
}

Enemy* EnemyManager::portalBoss() {
    return m_portalBoss;
}

int EnemyManager::findFreeSlot() const {
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        if (!m_enemies[static_cast<size_t>(i)]->isActive()) {
            return i;
        }
    }
    return -1;
}

bool EnemyManager::spawnOne(float playerX, float playerZ, float playerAngle) {
    const int slot = findFreeSlot();
    if (slot < 0) {
        return false;
    }
    m_enemies[static_cast<size_t>(slot)]->reset(
        playerX, playerZ, playerAngle, m_spawnIndex++);
    return true;
}

int EnemyManager::aliveCount() const {
    int count = 0;
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        if (m_enemies[static_cast<size_t>(i)]->isAlive()) {
            count++;
        }
    }
    return count;
}

void EnemyManager::trySpawn(float deltaTime, float playerX, float playerZ,
                            float playerAngle) {
    if (m_spawnPaused) {
        return;
    }

    m_spawnTimer -= deltaTime;

    if (aliveCount() >= m_maxEnemies || findFreeSlot() < 0) {
        return;
    }

    if (m_spawnTimer > 0.0f) {
        return;
    }

    if (spawnOne(playerX, playerZ, playerAngle)) {
        m_spawnTimer = m_spawnIntervalMin +
            Rng::nextFloat01() * m_spawnIntervalJitter;
    }
}

void EnemyManager::update(float deltaTime, float playerX, float playerZ,
                          float playerAngle, float playerAimY) {
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        m_enemies[static_cast<size_t>(i)]->update(
            deltaTime, playerX, playerZ, playerAngle, playerAimY);
    }
    if (m_portalBoss && m_portalBoss->isAlive()) {
        m_portalBoss->update(deltaTime, playerX, playerZ, playerAngle, playerAimY);
    }

    const int alive = aliveCount();
    if (alive < m_lastAliveCount && alive < m_maxEnemies) {
        if (m_spawnTimer > m_refillSpawnDelay) {
            m_spawnTimer = m_refillSpawnDelay;
        }
    }
    m_lastAliveCount = alive;

    trySpawn(deltaTime, playerX, playerZ, playerAngle);
}

Enemy* EnemyManager::findClosestInArc(float fromX, float fromZ, float fromAngleDeg,
                                      float maxRange, float aimConeDeg) {
    Enemy* best = nullptr;
    float bestDistSq = maxRange * maxRange;

    for (int i = 0; i < MAX_ENEMIES; ++i) {
        Enemy& e = *m_enemies[static_cast<size_t>(i)];
        if (!e.isAlive()) continue;

        const float dx = e.getX() - fromX;
        const float dz = e.getZ() - fromZ;
        const float distSq = dx * dx + dz * dz;
        if (distSq >= bestDistSq) continue;

        float angleToTarget = atan2f(dx, dz) * 180.0f / static_cast<float>(M_PI);
        float angleDiff = angleToTarget - fromAngleDeg;
        while (angleDiff > 180.0f) angleDiff -= 360.0f;
        while (angleDiff < -180.0f) angleDiff += 360.0f;
        if (fabsf(angleDiff) > aimConeDeg) continue;

        bestDistSq = distSq;
        best = &e;
    }

    if (m_portalBoss && m_portalBoss->isAlive()) {
        const float dx = m_portalBoss->getX() - fromX;
        const float dz = m_portalBoss->getZ() - fromZ;
        const float distSq = dx * dx + dz * dz;
        if (distSq < bestDistSq) {
            float angleToTarget = atan2f(dx, dz) * 180.0f / static_cast<float>(M_PI);
            float angleDiff = angleToTarget - fromAngleDeg;
            while (angleDiff > 180.0f) angleDiff -= 360.0f;
            while (angleDiff < -180.0f) angleDiff += 360.0f;
            if (fabsf(angleDiff) <= aimConeDeg) {
                best = m_portalBoss;
            }
        }
    }
    return best;
}

} // namespace Game
