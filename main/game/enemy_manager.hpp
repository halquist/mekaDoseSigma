#pragma once

#include "enemy.hpp"
#include "world_tier.hpp"
#include "map_config.hpp"
#include <array>
#include <cstdint>

namespace Game {

class ObstacleField;

class EnemyManager {
public:
    static constexpr int MAX_ENEMIES = 5;

    EnemyManager(Renderer::Scene& scene, ProjectileSystem& projectiles,
                 const MapConfig& mapConfig);
    ~EnemyManager();

    void reset(float playerX, float playerZ, float playerAngle,
               const ObstacleField* obstacles);
    void update(float deltaTime, float playerX, float playerZ, float playerAngle,
                float playerAimY, const ObstacleField* obstacles);
    void applyWorldTier(const WorldTier& tier);

    void spawnPortalBoss(float portalX, float portalZ, float portalAngle,
                         float playerX, float playerZ);
    void dismissPortalBoss();
    bool isPortalBossAlive() const;
    Enemy* portalBoss();

    void clearAllEnemies();
    void setSpawnPaused(bool paused);
    void prepareSpawnAfterTransition(float playerX, float playerZ, float playerAngle);

    int aliveCount() const;
    bool isPlayerUnderAttack(float playerX, float playerZ, float playerAngle) const;
    bool findClosestEnemyBehind(float playerX, float playerZ, float playerAngle,
                                float& outX, float& outZ) const;

    Enemy* findClosestInArc(float fromX, float fromZ, float fromAngleDeg,
                              float maxRange, float aimConeDeg);
    bool findBestStrikeTarget(float fromX, float fromZ, float fromAngleDeg,
                              float maxRange, float aimConeDeg, float blastRadius,
                              float& outX, float& outZ, float& outAimY) const;

    Enemy& enemy(int index) { return *m_enemies[static_cast<size_t>(index)]; }
    const Enemy& enemy(int index) const { return *m_enemies[static_cast<size_t>(index)]; }

private:
    int findFreeSlot() const;
    bool spawnOne(float playerX, float playerZ, float playerAngle,
                  const ObstacleField* obstacles);
    void trySpawn(float deltaTime, float playerX, float playerZ, float playerAngle,
                  const ObstacleField* obstacles);

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
    bool m_spawnPaused = false;
};

} // namespace Game
