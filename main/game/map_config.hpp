#pragma once

#include "environment.hpp"
#include <cstdint>

namespace Game {

/// Tunables for one play session / map module. Extend for bosses and victory rules.
struct MapConfig {
    uint32_t worldSeed = 0x53495341u; // "SISA"
    MapTheme theme = MapTheme::RURAL;
    EnvLightingMode lighting = EnvLightingMode::Day;

    /// Fraction of grid cells that contain a tree (0..1).
    float obstacleCellRate = 0.125f;

    /// Enemy ring spawn (world units from player).
    float enemySpawnDistMin = 520.0f;
    float enemySpawnDistMax = 720.0f;

    /// Objective building spawn (world units ahead of player).
    float objectiveSpawnDistMin = 680.0f;
    float objectiveSpawnDistMax = 920.0f;

    // Future hooks:
    // int difficulty = 1;
    // int victoryKillCount = 10;
    // int bossId = 0;
};

} // namespace Game
