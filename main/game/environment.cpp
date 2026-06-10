#include "environment.hpp"

namespace Game {

namespace {

const EnvPalette kDayPalette = {
    Colors::SKY_BLUE,
    Colors::GRASS,
    Colors::TREE_FOLIAGE,
    {255, 240, 220},
    140,
    20,
    -30,
    {100, 120, 90},
};

const EnvPalette kTwilightPalette = {
    Colors::rgb(72, 48, 108),
    Colors::rgb(50, 88, 40),
    Colors::rgb(38, 72, 34),
    {240, 165, 200},
    105,
    28,
    -16,
    {82, 72, 98},
};

const EnvPalette kNightPalette = {
    Colors::rgb(22, 26, 48),
    Colors::rgb(34, 52, 30),
    Colors::rgb(24, 42, 22),
    {155, 170, 215},
    80,
    12,
    -38,
    {62, 66, 82},
};

} // namespace

const EnvPalette& envPaletteFor(EnvLightingMode mode) {
    switch (mode) {
    case EnvLightingMode::Twilight:
        return kTwilightPalette;
    case EnvLightingMode::Night:
        return kNightPalette;
    case EnvLightingMode::Day:
    default:
        return kDayPalette;
    }
}

} // namespace Game
