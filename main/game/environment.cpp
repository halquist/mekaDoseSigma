#include "environment.hpp"

namespace Game {

namespace {

const EnvPalette kRuralDay = {
    Colors::SKY_BLUE,
    Colors::GRASS,
    Colors::TREE_FOLIAGE,
    Colors::TREE_TRUNK,
    Colors::ROCK,
    {255, 240, 220},
    140,
    20,
    -30,
    {100, 120, 90},
};

const EnvPalette kRuralTwilight = {
    Colors::rgb(72, 48, 108),
    Colors::rgb(50, 88, 40),
    Colors::rgb(38, 72, 34),
    Colors::rgb(70, 48, 30),
    Colors::rgb(82, 78, 72),
    {240, 165, 200},
    105,
    28,
    -16,
    {82, 72, 98},
};

const EnvPalette kRuralNight = {
    Colors::rgb(22, 26, 48),
    Colors::rgb(34, 52, 30),
    Colors::rgb(24, 42, 22),
    Colors::rgb(40, 28, 18),
    Colors::rgb(55, 50, 48),
    {155, 170, 215},
    80,
    12,
    -38,
    {62, 66, 82},
};

const EnvPalette kDesertDay = {
    Colors::rgb(168, 196, 228),
    Colors::rgb(196, 168, 108),
    Colors::rgb(140, 118, 72),
    Colors::rgb(120, 88, 52),
    Colors::rgb(150, 130, 100),
    {255, 228, 180},
    155,
    24,
    -28,
    {130, 110, 80},
};

const EnvPalette kDesertTwilight = {
    Colors::rgb(92, 58, 72),
    Colors::rgb(120, 88, 58),
    Colors::rgb(88, 62, 42),
    Colors::rgb(78, 52, 34),
    Colors::rgb(96, 72, 58),
    {255, 170, 130},
    110,
    30,
    -14,
    {88, 68, 58},
};

const EnvPalette kDesertNight = {
    Colors::rgb(28, 24, 38),
    Colors::rgb(72, 58, 38),
    Colors::rgb(52, 40, 28),
    Colors::rgb(44, 32, 22),
    Colors::rgb(58, 48, 40),
    {180, 150, 120},
    75,
    14,
    -36,
    {58, 50, 44},
};

const EnvPalette& paletteForThemeLighting(MapTheme theme, EnvLightingMode mode) {
    if (theme == MapTheme::DESERT) {
        switch (mode) {
        case EnvLightingMode::Twilight: return kDesertTwilight;
        case EnvLightingMode::Night: return kDesertNight;
        case EnvLightingMode::Day:
        default: return kDesertDay;
        }
    }

    switch (mode) {
    case EnvLightingMode::Twilight: return kRuralTwilight;
    case EnvLightingMode::Night: return kRuralNight;
    case EnvLightingMode::Day:
    default: return kRuralDay;
    }
}

} // namespace

const EnvPalette& envPaletteFor(MapTheme theme, EnvLightingMode mode) {
    return paletteForThemeLighting(theme, mode);
}

} // namespace Game
