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
            tier.enemyHpScale(), tier.enemyFireRateScale(),
            tier.enemyEngageRange(), tier.enemyAirStrikeRange(), tier.index);
    }
    if (m_portalBoss) {
        m_portalBoss->setWorldScaling(
            tier.enemySpeedScale(), tier.enemyShieldUseChance(),
            tier.enemyHpScale(), tier.enemyFireRateScale(),
            tier.enemyEngageRange(), tier.enemyAirStrikeRange(), tier.index);
    }
}

void EnemyManager::reset(float playerX, float playerZ, float playerAngle,
                         const ObstacleField* obstacles) {
    m_spawnIndex = 0;
    m_levelSpawnsEnabled = true;
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        m_enemies[static_cast<size_t>(i)]->deactivate();
    }
    dismissPortalBoss();
    m_aliveCount = 0;
    spawnOne(playerX, playerZ, playerAngle, obstacles);
    m_spawnTimer = m_initialSpawnDelay;
    m_lastAliveCount = m_aliveCount;
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
    m_aliveCount = 0;
}

void EnemyManager::setSpawnPaused(bool paused) {
    m_spawnPaused = paused;
}

void EnemyManager::onPortalBossDefeated() {
    m_levelSpawnsEnabled = false;
}

void EnemyManager::prepareSpawnAfterTransition(float playerX, float playerZ,
                                               float playerAngle) {
    (void)playerX;
    (void)playerZ;
    (void)playerAngle;
    m_levelSpawnsEnabled = true;
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

bool EnemyManager::spawnOne(float playerX, float playerZ, float playerAngle,
                            const ObstacleField* obstacles) {
    const int slot = findFreeSlot();
    if (slot < 0) {
        return false;
    }
    m_enemies[static_cast<size_t>(slot)]->reset(
        playerX, playerZ, playerAngle, obstacles, m_spawnIndex++);
    ++m_aliveCount;
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
                            float playerAngle, const ObstacleField* obstacles,
                            int currentAliveCount) {
    if (m_spawnPaused || !m_levelSpawnsEnabled) {
        return;
    }

    m_spawnTimer -= deltaTime;

    if (currentAliveCount >= m_maxEnemies || findFreeSlot() < 0) {
        return;
    }

    if (m_spawnTimer > 0.0f) {
        return;
    }

    if (spawnOne(playerX, playerZ, playerAngle, obstacles)) {
        m_spawnTimer = m_spawnIntervalMin +
            Rng::nextFloat01() * m_spawnIntervalJitter;
    }
}

void EnemyManager::update(float deltaTime, float playerX, float playerZ,
                          float playerAngle, float playerAimY,
                          const ObstacleField* obstacles) {
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        m_enemies[static_cast<size_t>(i)]->update(
            deltaTime, playerX, playerZ, playerAngle, playerAimY, obstacles);
    }
    if (m_portalBoss && m_portalBoss->isAlive()) {
        m_portalBoss->update(deltaTime, playerX, playerZ, playerAngle, playerAimY,
                             obstacles);
    }

    // Compute alive count once per frame; keep incremental counter in sync.
    const int alive = aliveCount();
    m_aliveCount = alive;
    if (m_levelSpawnsEnabled && alive < m_lastAliveCount && alive < m_maxEnemies) {
        if (m_spawnTimer > m_refillSpawnDelay) {
            m_spawnTimer = m_refillSpawnDelay;
        }
    }
    m_lastAliveCount = alive;

    trySpawn(deltaTime, playerX, playerZ, playerAngle, obstacles, alive);
}

namespace {

bool isEnemyThreatening(const Enemy& enemy, float playerX, float playerZ) {
    if (!enemy.isAlive()) {
        return false;
    }

    const float dx = enemy.getX() - playerX;
    const float dz = enemy.getZ() - playerZ;
    const float range = enemy.engageRange();
    return (dx * dx + dz * dz) <= range * range;
}

bool isEnemyBehind(float enemyX, float enemyZ, float playerX, float playerZ,
                   float playerAngle) {
    const float radians = playerAngle * static_cast<float>(M_PI) / 180.0f;
    const float forwardX = sinf(radians);
    const float forwardZ = cosf(radians);
    const float dx = enemyX - playerX;
    const float dz = enemyZ - playerZ;
    return (dx * forwardX + dz * forwardZ) < 0.0f;
}

} // namespace

bool EnemyManager::isPlayerUnderAttack(float playerX, float playerZ,
                                       float playerAngle) const {
    (void)playerAngle;

    for (int i = 0; i < MAX_ENEMIES; ++i) {
        if (isEnemyThreatening(*m_enemies[static_cast<size_t>(i)], playerX, playerZ)) {
            return true;
        }
    }

    if (m_portalBoss && isEnemyThreatening(*m_portalBoss, playerX, playerZ)) {
        return true;
    }

    return false;
}

bool EnemyManager::findClosestEnemyBehind(float playerX, float playerZ,
                                          float playerAngle,
                                          float& outX, float& outZ) const {
    float bestDistSq = 0.0f;
    bool found = false;

    const auto consider = [&](const Enemy& enemy) {
        if (!isEnemyThreatening(enemy, playerX, playerZ)) {
            return;
        }
        if (!isEnemyBehind(enemy.getX(), enemy.getZ(), playerX, playerZ, playerAngle)) {
            return;
        }

        const float dx = enemy.getX() - playerX;
        const float dz = enemy.getZ() - playerZ;
        const float distSq = dx * dx + dz * dz;
        if (!found || distSq < bestDistSq) {
            bestDistSq = distSq;
            outX = enemy.getX();
            outZ = enemy.getZ();
            found = true;
        }
    };

    for (int i = 0; i < MAX_ENEMIES; ++i) {
        consider(*m_enemies[static_cast<size_t>(i)]);
    }
    if (m_portalBoss) {
        consider(*m_portalBoss);
    }

    return found;
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

bool EnemyManager::findBestStrikeTarget(float fromX, float fromZ, float fromAngleDeg,
                                        float maxRange, float aimConeDeg,
                                        float blastRadius,
                                        float& outX, float& outZ,
                                        float& outAimY) const {
    struct Candidate {
        float x = 0.0f;
        float z = 0.0f;
        float aimY = 0.0f;
        float mechDistSq = 0.0f;
    };

    Candidate candidates[MAX_ENEMIES + 1];
    int candidateCount = 0;

    const auto tryAdd = [&](const Enemy& e) {
        if (!e.isAlive()) {
            return;
        }
        const float dx = e.getX() - fromX;
        const float dz = e.getZ() - fromZ;
        const float distSq = dx * dx + dz * dz;
        if (distSq > maxRange * maxRange) {
            return;
        }

        float angleToTarget = atan2f(dx, dz) * 180.0f / static_cast<float>(M_PI);
        float angleDiff = angleToTarget - fromAngleDeg;
        while (angleDiff > 180.0f) angleDiff -= 360.0f;
        while (angleDiff < -180.0f) angleDiff += 360.0f;
        if (fabsf(angleDiff) > aimConeDeg) {
            return;
        }

        Candidate& c = candidates[candidateCount++];
        c.x = e.getX();
        c.z = e.getZ();
        c.aimY = e.getAimY();
        c.mechDistSq = distSq;
    };

    for (int i = 0; i < MAX_ENEMIES; ++i) {
        tryAdd(*m_enemies[static_cast<size_t>(i)]);
    }
    if (m_portalBoss) {
        tryAdd(*m_portalBoss);
    }

    if (candidateCount == 0) {
        return false;
    }

    const float radiusSq = blastRadius * blastRadius;

    const auto countInBlast = [&](float cx, float cz) -> int {
        int count = 0;
        for (int i = 0; i < MAX_ENEMIES; ++i) {
            const Enemy& e = *m_enemies[static_cast<size_t>(i)];
            if (!e.isAlive()) {
                continue;
            }
            const float dx = e.getX() - cx;
            const float dz = e.getZ() - cz;
            if (dx * dx + dz * dz <= radiusSq) {
                ++count;
            }
        }
        if (m_portalBoss && m_portalBoss->isAlive()) {
            const float dx = m_portalBoss->getX() - cx;
            const float dz = m_portalBoss->getZ() - cz;
            if (dx * dx + dz * dz <= radiusSq) {
                ++count;
            }
        }
        return count;
    };

    int bestCount = -1;
    float bestMechDistSq = 0.0f;
    int bestIdx = -1;

    for (int i = 0; i < candidateCount; ++i) {
        const int hitCount = countInBlast(candidates[i].x, candidates[i].z);
        if (hitCount > bestCount ||
            (hitCount == bestCount && candidates[i].mechDistSq < bestMechDistSq)) {
            bestCount = hitCount;
            bestMechDistSq = candidates[i].mechDistSq;
            bestIdx = i;
        }
    }

    if (bestIdx < 0) {
        return false;
    }

    outX = candidates[bestIdx].x;
    outZ = candidates[bestIdx].z;
    outAimY = candidates[bestIdx].aimY;
    return true;
}

} // namespace Game
