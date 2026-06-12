#include "world_tier.hpp"
#include <algorithm>

namespace Game {

namespace {

constexpr MapTheme kWorldThemeOrder[] = {
    MapTheme::RURAL,
    MapTheme::DESERT,
    MapTheme::INDUSTRIAL,
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
    return std::min(5, 1 + index / 4);
}

float WorldTier::spawnIntervalMin() const {
    return std::max(3.0f, 6.5f - static_cast<float>(index) * 0.45f);
}

float WorldTier::spawnIntervalJitter() const {
    return std::max(2.0f, 5.5f - static_cast<float>(index) * 0.35f);
}

float WorldTier::initialSpawnDelay() const {
    return std::max(2.5f, 4.5f - static_cast<float>(index) * 0.25f);
}

float WorldTier::refillSpawnDelay() const {
    return std::max(3.0f, 5.0f - static_cast<float>(index) * 0.3f);
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

int WorldTier::killPointValue() const {
    return 100 + index * 25;
}

int WorldTier::objectivePointValue() const {
    return 500 + index * 100;
}

} // namespace Game
