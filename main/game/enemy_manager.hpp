#pragma once

#include "enemy.hpp"
#include "world_tier.hpp"
#include "map_config.hpp"
#include <array>
#include <cstdint>

namespace Game {

class EnemyManager {
public:
    static constexpr int MAX_ENEMIES = 3;

    EnemyManager(Renderer::Scene& scene, ProjectileSystem& projectiles,
                 const MapConfig& mapConfig);
    ~EnemyManager();

    void reset(float playerX, float playerZ, float playerAngle);
    void update(float deltaTime, float playerX, float playerZ, float playerAngle,
                float playerAimY);
    void applyWorldTier(const WorldTier& tier);

    void spawnPortalBoss(float portalX, float portalZ, float portalAngle,
                         float playerX, float playerZ);
    void dismissPortalBoss();
    bool isPortalBossAlive() const;
    Enemy* portalBoss();

    int aliveCount() const;
    Enemy* findClosestInArc(float fromX, float fromZ, float fromAngleDeg,
                              float maxRange, float aimConeDeg);

    Enemy& enemy(int index) { return *m_enemies[static_cast<size_t>(index)]; }
    const Enemy& enemy(int index) const { return *m_enemies[static_cast<size_t>(index)]; }

private:
    int findFreeSlot() const;
    bool spawnOne(float playerX, float playerZ, float playerAngle);
    void trySpawn(float deltaTime, float playerX, float playerZ, float playerAngle);

    std::array<Enemy*, MAX_ENEMIES> m_enemies = {};
    Enemy* m_portalBoss = nullptr;
    uint32_t m_spawnIndex = 0;
    float m_spawnTimer = 0.0f;
    int m_lastAliveCount = 0;

    static constexpr float SPAWN_INTERVAL_MIN = 4.5f;
    static constexpr float SPAWN_INTERVAL_JITTER = 4.0f;
    static constexpr float INITIAL_SPAWN_DELAY = 3.5f;
    static constexpr float REFILL_SPAWN_DELAY = 4.0f;

    float m_spawnIntervalMin = SPAWN_INTERVAL_MIN;
    float m_spawnIntervalJitter = SPAWN_INTERVAL_JITTER;
    float m_initialSpawnDelay = INITIAL_SPAWN_DELAY;
    float m_refillSpawnDelay = REFILL_SPAWN_DELAY;
    int m_maxEnemies = MAX_ENEMIES;
};

} // namespace Game
