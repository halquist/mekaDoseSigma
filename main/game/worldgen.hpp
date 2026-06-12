#pragma once

#include "map_config.hpp"
#include "environment.hpp"
#include <cstdint>

namespace Game {

class ObstacleField;

namespace WorldGen {

constexpr int BIOME_CELL_SIZE = 440;
constexpr int CITY_ROAD_BLOCK = 168;
constexpr int MAX_SPAWN_RETRIES = 8;
constexpr float ENEMY_SPAWN_CLEAR_RADIUS = 40.0f;
constexpr float OBJECTIVE_SPAWN_CLEAR_RADIUS = 50.0f;

uint32_t hash(int cx, int cz, uint32_t seed);
uint32_t hashPosition(float x, float z, uint32_t seed);

MapTheme biomeAt(float worldX, float worldZ, const MapConfig& cfg);

float terrainHeight(float worldX, float worldZ, const MapConfig& cfg);

bool isCityRoad(float worldX, float worldZ);
bool isNearCityRoad(float worldX, float worldZ, float maxDist);

enum class ObstacleKind : uint8_t {
    Tree,
    Building,
};

struct ObstacleSpec {
    bool present = false;
    ObstacleKind kind = ObstacleKind::Tree;
    float x = 0.0f;
    float z = 0.0f;
    float scale = 1.0f;
    float collisionRadius = 12.0f;
};

ObstacleSpec sampleObstacle(int cellX, int cellZ, const MapConfig& cfg);

bool isSpawnPositionValid(float x, float z, const MapConfig& cfg,
                          const ObstacleField* obstacles, float clearRadius);

void sampleEnemySpawn(float playerX, float playerZ, float playerAngle,
                      uint32_t spawnIndex, uint32_t spawnSalt,
                      const MapConfig& cfg, const ObstacleField* obstacles,
                      float& outX, float& outZ);

void sampleObjectiveSpawn(float playerX, float playerZ, float playerAngle,
                          uint32_t spawnIndex, uint32_t spawnSalt,
                          const MapConfig& cfg, const ObstacleField* obstacles,
                          float& outX, float& outZ);

} // namespace WorldGen
} // namespace Game
