#pragma once

#include "environment.hpp"
#include <cstdint>

namespace Game {

struct WorldTier {
    int index = 0;

    MapTheme themeForWorld() const;
    MapTheme nextTheme(MapTheme previous) const;

    int maxEnemies() const;
    float spawnIntervalMin() const;
    float spawnIntervalJitter() const;
    float initialSpawnDelay() const;
    float refillSpawnDelay() const;

    float enemySpeedScale() const;
    float enemyDamageScale() const;
    float enemyShieldUseChance() const;

    int killPointValue() const;
    int objectivePointValue() const;
};

constexpr int kObjectivesPerPortal = 6;

} // namespace Game
