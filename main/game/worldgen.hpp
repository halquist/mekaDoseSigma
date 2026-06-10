#pragma once

#include "map_config.hpp"
#include <cstdint>

namespace Game {

namespace WorldGen {

uint32_t hash(int cx, int cz, uint32_t seed);
uint32_t hashPosition(float x, float z, uint32_t seed);

float terrainHeight(float worldX, float worldZ, const MapConfig& cfg);

struct ObstacleSpec {
    bool present = false;
    float x = 0;
    float z = 0;
    float scale = 1.0f;
};

ObstacleSpec sampleObstacle(int cellX, int cellZ, const MapConfig& cfg);

void sampleEnemySpawn(float playerX, float playerZ, float playerAngle,
                      uint32_t spawnIndex, uint32_t spawnSalt,
                      const MapConfig& cfg, float& outX, float& outZ);

void sampleObjectiveSpawn(float playerX, float playerZ, float playerAngle,
                          uint32_t spawnIndex, uint32_t spawnSalt,
                          const MapConfig& cfg, float& outX, float& outZ);

} // namespace WorldGen
} // namespace Game
