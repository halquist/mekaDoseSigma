#include "worldgen.hpp"
#include "obstacles.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Game {
namespace WorldGen {

uint32_t hash(int cx, int cz, uint32_t seed) {
    uint32_t h = static_cast<uint32_t>(cx * 73856093) ^
                 static_cast<uint32_t>(cz * 19349663) ^
                 seed;
    h ^= h >> 16;
    h *= 0x45d9f3b;
    h ^= h >> 16;
    return h;
}

uint32_t hashPosition(float x, float z, uint32_t seed) {
    return hash(static_cast<int>(x), static_cast<int>(z), seed ^ 0x9E3779B9u);
}

MapTheme biomeAt(float worldX, float worldZ, const MapConfig& cfg) {
    (void)worldX;
    (void)worldZ;
    return cfg.theme;
}

bool isCityRoad(float worldX, float worldZ) {
    const int gx = static_cast<int>(floorf(worldX / static_cast<float>(CITY_ROAD_BLOCK)));
    const int gz = static_cast<int>(floorf(worldZ / static_cast<float>(CITY_ROAD_BLOCK)));
    return (gx & 1) == 0 || (gz & 1) == 0;
}

bool isNearCityRoad(float worldX, float worldZ, float maxDist) {
    const float step = static_cast<float>(CITY_ROAD_BLOCK) * 0.5f;
    const int rings = static_cast<int>(ceilf(maxDist / step));
    for (int dz = -rings; dz <= rings; ++dz) {
        for (int dx = -rings; dx <= rings; ++dx) {
            const float sx = worldX + static_cast<float>(dx) * step;
            const float sz = worldZ + static_cast<float>(dz) * step;
            if (isCityRoad(sx, sz)) {
                const float ddx = sx - worldX;
                const float ddz = sz - worldZ;
                if (ddx * ddx + ddz * ddz <= maxDist * maxDist) {
                    return true;
                }
            }
        }
    }
    return false;
}

float terrainHeight(float worldX, float worldZ, const MapConfig& cfg) {
    const uint32_t s = cfg.worldSeed;
    const float sx = worldX + static_cast<float>((s & 0xFF) * 0.01f);
    const float sz = worldZ + static_cast<float>(((s >> 8) & 0xFF) * 0.01f);

    switch (biomeAt(worldX, worldZ, cfg)) {
        case MapTheme::RURAL:
        default:
            return 4.0f * sinf(sx * 0.006f) * cosf(sz * 0.005f)
                 + 2.5f * sinf(sx * 0.012f + 1.2f) * sinf(sz * 0.011f)
                 + 1.5f * cosf((sx + sz) * 0.009f);
        case MapTheme::DESERT:
            return 3.0f * sinf(sx * 0.004f) * cosf(sz * 0.0035f)
                 + 2.0f * sinf(sx * 0.009f) * sinf(sz * 0.008f);
        case MapTheme::INDUSTRIAL:
            return 1.5f * sinf(sx * 0.008f) * cosf(sz * 0.008f);
        case MapTheme::CITY:
            return 0.0f;
    }
}

ObstacleSpec sampleObstacle(int cellX, int cellZ, const MapConfig& cfg) {
    ObstacleSpec spec;
    const uint32_t h = hash(cellX, cellZ, cfg.worldSeed);

    const bool isCity = cfg.theme == MapTheme::CITY;
    const float cellRate = isCity ? cfg.cityObstacleCellRate : cfg.obstacleCellRate;
    const float threshold = cellRate * 65535.0f;
    if (static_cast<float>(h & 0xFFFF) > threshold) {
        return spec;
    }

    constexpr int kCellSize = 110;
    spec.x = static_cast<float>(cellX * kCellSize) +
        static_cast<float>((h >> 4) % kCellSize) - kCellSize * 0.5f;
    spec.z = static_cast<float>(cellZ * kCellSize) +
        static_cast<float>((h >> 12) % kCellSize) - kCellSize * 0.5f;

    if (isCity) {
        spec.present = true;
        spec.kind = ObstacleKind::Building;
        spec.scale = 1.2f + static_cast<float>((h >> 8) & 0xFF) / 255.0f * 0.8f;
        spec.collisionRadius = 22.0f * spec.scale;
        return spec;
    }

    spec.present = true;
    spec.kind = ObstacleKind::Tree;
    spec.scale = 1.0f + static_cast<float>((h >> 8) & 0xFF) / 255.0f;
    spec.collisionRadius = 12.0f * spec.scale;
    return spec;
}

bool isSpawnPositionValid(float x, float z, const MapConfig& cfg,
                          const ObstacleField* obstacles, float clearRadius) {
    if (obstacles && obstacles->isPositionBlocked(x, z, clearRadius)) {
        return false;
    }
    return true;
}

void sampleEnemySpawn(float playerX, float playerZ, float playerAngle,
                      uint32_t spawnIndex, uint32_t spawnSalt,
                      const MapConfig& cfg, const ObstacleField* obstacles,
                      float& outX, float& outZ) {
    for (int attempt = 0; attempt < MAX_SPAWN_RETRIES; ++attempt) {
        const uint32_t salt = spawnSalt + static_cast<uint32_t>(attempt) * 1597334677u;
        const int anchorX = static_cast<int>(playerX / 80.0f);
        const int anchorZ = static_cast<int>(playerZ / 80.0f);
        const uint32_t h = hash(anchorX, anchorZ,
                                cfg.worldSeed ^ (spawnIndex * 2654435761u) ^ salt);

        constexpr float kForwardArcHalf = 65.0f;
        constexpr float kRearArcHalf = 50.0f;
        const bool spawnBehind = ((h >> 24) & 0xFFu) < 38u;
        const float arcHalf = spawnBehind ? kRearArcHalf : kForwardArcHalf;
        const float arcRoll = static_cast<float>((h >> 8) & 0xFF) *
            (2.0f * arcHalf / 255.0f) - arcHalf;
        const float angleOffset = spawnBehind ? (180.0f + arcRoll) : arcRoll;
        const float distSpan = cfg.enemySpawnDistMax - cfg.enemySpawnDistMin;
        const float dist = cfg.enemySpawnDistMin +
            static_cast<float>((h >> 16) & 0xFF) * (distSpan / 255.0f);

        const float radians =
            (playerAngle + angleOffset) * static_cast<float>(M_PI) / 180.0f;
        const float candidateX = playerX + sinf(radians) * dist;
        const float candidateZ = playerZ + cosf(radians) * dist;
        if (isSpawnPositionValid(candidateX, candidateZ, cfg, obstacles,
                                 ENEMY_SPAWN_CLEAR_RADIUS)) {
            outX = candidateX;
            outZ = candidateZ;
            return;
        }
    }

    const int anchorX = static_cast<int>(playerX / 80.0f);
    const int anchorZ = static_cast<int>(playerZ / 80.0f);
    const uint32_t h = hash(anchorX, anchorZ,
                            cfg.worldSeed ^ (spawnIndex * 2654435761u) ^ spawnSalt);
    constexpr float kForwardArcHalf = 65.0f;
    const float arcRoll = static_cast<float>((h >> 8) & 0xFF) *
        (2.0f * kForwardArcHalf / 255.0f) - kForwardArcHalf;
    const float distSpan = cfg.enemySpawnDistMax - cfg.enemySpawnDistMin;
    const float dist = cfg.enemySpawnDistMin +
        static_cast<float>((h >> 16) & 0xFF) * (distSpan / 255.0f);
    const float radians =
        (playerAngle + arcRoll) * static_cast<float>(M_PI) / 180.0f;
    outX = playerX + sinf(radians) * dist;
    outZ = playerZ + cosf(radians) * dist;
}

void sampleObjectiveSpawn(float playerX, float playerZ, float playerAngle,
                          uint32_t spawnIndex, uint32_t spawnSalt,
                          const MapConfig& cfg, const ObstacleField* obstacles,
                          float& outX, float& outZ) {
    for (int attempt = 0; attempt < MAX_SPAWN_RETRIES; ++attempt) {
        const uint32_t salt = spawnSalt + static_cast<uint32_t>(attempt) * 2654435761u;
        const int anchorX = static_cast<int>(playerX / 80.0f);
        const int anchorZ = static_cast<int>(playerZ / 80.0f);
        const uint32_t h = hash(anchorX, anchorZ,
                                cfg.worldSeed ^ (spawnIndex * 1597334677u) ^ salt);

        constexpr float kForwardArcHalf = 55.0f;
        const float angleOffset = static_cast<float>((h >> 8) & 0xFF) *
            (2.0f * kForwardArcHalf / 255.0f) - kForwardArcHalf;
        const float distSpan = cfg.objectiveSpawnDistMax - cfg.objectiveSpawnDistMin;
        const float dist = cfg.objectiveSpawnDistMin +
            static_cast<float>((h >> 16) & 0xFF) * (distSpan / 255.0f);

        const float radians =
            (playerAngle + angleOffset) * static_cast<float>(M_PI) / 180.0f;
        const float candidateX = playerX + sinf(radians) * dist;
        const float candidateZ = playerZ + cosf(radians) * dist;
        if (isSpawnPositionValid(candidateX, candidateZ, cfg, obstacles,
                                 OBJECTIVE_SPAWN_CLEAR_RADIUS)) {
            outX = candidateX;
            outZ = candidateZ;
            return;
        }
    }

    const int anchorX = static_cast<int>(playerX / 80.0f);
    const int anchorZ = static_cast<int>(playerZ / 80.0f);
    const uint32_t h = hash(anchorX, anchorZ,
                            cfg.worldSeed ^ (spawnIndex * 1597334677u) ^ spawnSalt);
    constexpr float kForwardArcHalf = 55.0f;
    const float angleOffset = static_cast<float>((h >> 8) & 0xFF) *
        (2.0f * kForwardArcHalf / 255.0f) - kForwardArcHalf;
    const float distSpan = cfg.objectiveSpawnDistMax - cfg.objectiveSpawnDistMin;
    const float dist = cfg.objectiveSpawnDistMin +
        static_cast<float>((h >> 16) & 0xFF) * (distSpan / 255.0f);
    const float radians =
        (playerAngle + angleOffset) * static_cast<float>(M_PI) / 180.0f;
    outX = playerX + sinf(radians) * dist;
    outZ = playerZ + cosf(radians) * dist;
}

} // namespace WorldGen
} // namespace Game
