#include "world_tier.hpp"
#include "run_upgrades.hpp"
#include <algorithm>

namespace Game {

namespace {

constexpr MapTheme kWorldThemeOrder[] = {
    MapTheme::RURAL,
    MapTheme::DESERT,
    MapTheme::CITY,
};

MapTheme themeAtIndex(int worldIndex) {
    const int count = static_cast<int>(sizeof(kWorldThemeOrder) / sizeof(kWorldThemeOrder[0]));
    const int idx = ((worldIndex % count) + count) % count;
    return kWorldThemeOrder[idx];
}

} // namespace

MapTheme WorldTier::themeForWorld() const {
    return themeAtIndex(index);
}

MapTheme WorldTier::nextTheme(MapTheme previous) const {
    const int count = static_cast<int>(sizeof(kWorldThemeOrder) / sizeof(kWorldThemeOrder[0]));
    int start = 0;
    for (int i = 0; i < count; ++i) {
        if (kWorldThemeOrder[i] == previous) {
            start = (i + 1) % count;
            break;
        }
    }
    return kWorldThemeOrder[start];
}

int WorldTier::maxEnemies() const {
    if (index < 2) {
        return 2;
    }
    if (index < 4) {
        return 3;
    }
    if (index < 7) {
        return 4;
    }
    return 5;
}

float WorldTier::spawnIntervalMin() const {
    const float base = std::max(3.0f, 6.5f - static_cast<float>(index) * 0.45f);
    return index < 2 ? base * 2.0f : base;
}

float WorldTier::spawnIntervalJitter() const {
    const float base = std::max(2.0f, 5.5f - static_cast<float>(index) * 0.35f);
    return index < 2 ? base * 2.0f : base;
}

float WorldTier::initialSpawnDelay() const {
    const float base = std::max(2.5f, 4.5f - static_cast<float>(index) * 0.25f);
    return index < 2 ? base * 2.0f : base;
}

float WorldTier::refillSpawnDelay() const {
    const float base = std::max(3.0f, 5.0f - static_cast<float>(index) * 0.3f);
    return index < 2 ? base * 2.0f : base;
}

float WorldTier::enemySpeedScale() const {
    return 1.0f + static_cast<float>(index) * 0.08f;
}

float WorldTier::enemyDamageScale() const {
    return 1.0f + static_cast<float>(index) * 0.20f;
}

float WorldTier::enemyHpScale() const {
    return 1.0f + static_cast<float>(index) * 0.45f;
}

float WorldTier::enemyFireRateScale() const {
    return 1.0f + static_cast<float>(index) * 0.14f;
}

float WorldTier::enemyShieldUseChance() const {
    return std::min(0.85f, 0.28f + static_cast<float>(index) * 0.09f);
}

float WorldTier::enemyEngageRange() const {
    constexpr float kGrowthPerWorld = 12.5f;
    const float baseRange = laserRangeForTier(1);
    const float maxRange = missileRangeForTier(6);
    return std::min(maxRange, baseRange + static_cast<float>(index) * kGrowthPerWorld);
}

float WorldTier::enemyAirStrikeRange() const {
    constexpr float kLegacyAirRatio = 190.0f / 420.0f;
    return enemyEngageRange() * kLegacyAirRatio;
}

int WorldTier::killPointValue() const {
    return 100 + index * 25;
}

int WorldTier::objectivePointValue() const {
    return 500 + index * 100;
}

int WorldTier::objectivesPerPortal() const {
    if (index < 6) {
        return 3;
    }
    return 3 + 1 + (index - 6) / 3;
}

} // namespace Game
