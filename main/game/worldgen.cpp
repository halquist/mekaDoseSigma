#include "worldgen.hpp"
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

namespace {

constexpr uint32_t kBiomeLayoutSalt = 0xB10BEB1Eu;

MapTheme baseBiomeForCell(int biomeCellX, int biomeCellZ, uint32_t seed) {
    const uint32_t h = hash(biomeCellX, biomeCellZ, seed ^ kBiomeLayoutSalt);
    return (h & 1u) ? MapTheme::DESERT : MapTheme::RURAL;
}

} // namespace

MapTheme biomeAt(float worldX, float worldZ, const MapConfig& cfg) {
    const float cellSize = static_cast<float>(BIOME_CELL_SIZE);
    const int biomeCellX = static_cast<int>(floorf(worldX / cellSize));
    const int biomeCellZ = static_cast<int>(floorf(worldZ / cellSize));
    return baseBiomeForCell(biomeCellX, biomeCellZ, cfg.worldSeed);
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
            return 0.8f * sinf(sx * 0.015f) * cosf(sz * 0.015f);
    }
}

ObstacleSpec sampleObstacle(int cellX, int cellZ, const MapConfig& cfg) {
    ObstacleSpec spec;
    const uint32_t h = hash(cellX, cellZ, cfg.worldSeed);

    const float cellRate = cfg.obstacleCellRate;
    const float threshold = cellRate * 65535.0f;
    if (static_cast<float>(h & 0xFFFF) > threshold) {
        return spec;
    }

    spec.present = true;
    spec.scale = 1.0f + static_cast<float>((h >> 8) & 0xFF) / 255.0f;

    constexpr int kCellSize = 110;
    spec.x = cellX * kCellSize + static_cast<float>((h >> 4) % kCellSize) -
             kCellSize * 0.5f;
    spec.z = cellZ * kCellSize + static_cast<float>((h >> 12) % kCellSize) -
             kCellSize * 0.5f;

    return spec;
}

void sampleEnemySpawn(float playerX, float playerZ, float playerAngle,
                      uint32_t spawnIndex, uint32_t spawnSalt,
                      const MapConfig& cfg, float& outX, float& outZ) {
    const int anchorX = static_cast<int>(playerX / 80.0f);
    const int anchorZ = static_cast<int>(playerZ / 80.0f);
    const uint32_t h = hash(anchorX, anchorZ,
                            cfg.worldSeed ^ (spawnIndex * 2654435761u) ^ spawnSalt);

    // Forward arc only — full-ring spawns often landed behind the player and were
    // instantly culled, wasting the spawn timer for 20+ seconds at a time.
    constexpr float kForwardArcHalf = 72.0f;
    const float angleOffset = static_cast<float>((h >> 8) & 0xFF) *
        (2.0f * kForwardArcHalf / 255.0f) - kForwardArcHalf;
    const float distSpan = cfg.enemySpawnDistMax - cfg.enemySpawnDistMin;
    const float dist = cfg.enemySpawnDistMin +
        static_cast<float>((h >> 16) & 0xFF) * (distSpan / 255.0f);

    const float radians =
        (playerAngle + angleOffset) * static_cast<float>(M_PI) / 180.0f;
    outX = playerX + sinf(radians) * dist;
    outZ = playerZ + cosf(radians) * dist;
}

void sampleObjectiveSpawn(float playerX, float playerZ, float playerAngle,
                          uint32_t spawnIndex, uint32_t spawnSalt,
                          const MapConfig& cfg, float& outX, float& outZ) {
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
