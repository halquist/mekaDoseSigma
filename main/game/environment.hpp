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

struct EnvPalette {
    uint16_t sky = Colors::SKY_BLUE;
    uint16_t grass = Colors::GRASS;
    uint16_t treeFoliage = Colors::TREE_FOLIAGE;
    Renderer::Color sunColor{255, 240, 220};
    uint16_t sunIntensity = 140;
    int32_t sunAzimuth = 20;
    int32_t sunElevation = -30;
    Renderer::Color ambientColor{100, 120, 90};
};

const EnvPalette& envPaletteFor(EnvLightingMode mode);

} // namespace Game
