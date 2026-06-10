#include "enemy_manager.hpp"
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
}

EnemyManager::~EnemyManager() {
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        delete m_enemies[static_cast<size_t>(i)];
        m_enemies[static_cast<size_t>(i)] = nullptr;
    }
}

void EnemyManager::reset(float playerX, float playerZ, float playerAngle) {
    m_spawnIndex = 0;
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        m_enemies[static_cast<size_t>(i)]->deactivate();
    }
    spawnOne(playerX, playerZ, playerAngle);
    m_spawnTimer = INITIAL_SPAWN_DELAY;
    m_lastAliveCount = aliveCount();
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
    m_spawnTimer -= deltaTime;

    if (aliveCount() >= MAX_ENEMIES || findFreeSlot() < 0) {
        return;
    }

    if (m_spawnTimer > 0.0f) {
        return;
    }

    if (spawnOne(playerX, playerZ, playerAngle)) {
        m_spawnTimer = SPAWN_INTERVAL_MIN +
            Rng::nextFloat01() * SPAWN_INTERVAL_JITTER;
    }
}

void EnemyManager::update(float deltaTime, float playerX, float playerZ,
                          float playerAngle, float playerAimY) {
    for (int i = 0; i < MAX_ENEMIES; ++i) {
        m_enemies[static_cast<size_t>(i)]->update(
            deltaTime, playerX, playerZ, playerAngle, playerAimY);
    }

    const int alive = aliveCount();
    if (alive < m_lastAliveCount && alive < MAX_ENEMIES) {
        if (m_spawnTimer > REFILL_SPAWN_DELAY) {
            m_spawnTimer = REFILL_SPAWN_DELAY;
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
    return best;
}

} // namespace Game
