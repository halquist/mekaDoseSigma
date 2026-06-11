#pragma once

#include "Light.hpp"
#include "types.hpp"
#include <cstdint>

namespace Game {

enum class EnvLightingMode : uint8_t {
    Day,
    Twilight,
    Night,
};

enum class MapTheme : uint8_t {
    RURAL,
    DESERT,
    INDUSTRIAL,
    CITY,
};

/// Full day/night loop duration in seconds (demo: 60s; production target: 300s).
constexpr float kDayNightCycleSec = 300.0f;

struct EnvPalette {
    uint16_t sky = Colors::SKY_BLUE;
    uint16_t grass = Colors::GRASS;
    uint16_t treeFoliage = Colors::TREE_FOLIAGE;
    uint16_t treeTrunk = Colors::TREE_TRUNK;
    uint16_t rock = Colors::ROCK;
    Renderer::Color sunColor{255, 240, 220};
    uint16_t sunIntensity = 140;
    int32_t sunAzimuth = 20;
    int32_t sunElevation = -30;
    Renderer::Color ambientColor{100, 120, 90};
};

const EnvPalette& envPaletteFor(MapTheme theme, EnvLightingMode mode);

/// Blend two cycle palettes; @p t in 0..1.
void lerpEnvPalette(const EnvPalette& a, const EnvPalette& b, float t, EnvPalette& out);

/// @p phase01 wraps 0..1: day -> twilight -> night -> dawn -> day (smooth blend).
void envPaletteAtCyclePhase(MapTheme theme, float phase01, EnvPalette& out);

} // namespace Game
