#include "environment.hpp"
#include <cmath>

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

const EnvPalette kRuralDawn = {
    Colors::rgb(148, 108, 128),
    Colors::rgb(58, 92, 48),
    Colors::rgb(42, 82, 38),
    Colors::rgb(62, 42, 26),
    Colors::rgb(72, 68, 62),
    {255, 200, 140},
    100,
    18,
    -22,
    {92, 84, 78},
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

const EnvPalette kDesertDawn = {
    Colors::rgb(168, 112, 92),
    Colors::rgb(132, 100, 64),
    Colors::rgb(96, 72, 48),
    Colors::rgb(88, 58, 38),
    Colors::rgb(104, 82, 68),
    {255, 190, 120},
    115,
    20,
    -20,
    {102, 88, 72},
};

const EnvPalette* kRuralCycle[] = {
    &kRuralDay, &kRuralTwilight, &kRuralNight, &kRuralDawn,
};
const EnvPalette* kDesertCycle[] = {
    &kDesertDay, &kDesertTwilight, &kDesertNight, &kDesertDawn,
};

const EnvPalette kCityDay = {
    Colors::CITY_SKY,
    Colors::CITY_GROUND,
    Colors::CITY_BUILDING,
    Colors::CITY_ROAD,
    Colors::CITY_ROAD,
    {220, 225, 235},
    125,
    18,
    -28,
    {90, 95, 105},
};

const EnvPalette kCityTwilight = {
    Colors::rgb(48, 52, 72),
    Colors::rgb(42, 44, 48),
    Colors::rgb(58, 62, 72),
    Colors::rgb(28, 30, 34),
    Colors::rgb(28, 30, 34),
    {200, 150, 180},
    95,
    24,
    -14,
    {72, 68, 88},
};

const EnvPalette kCityNight = {
    Colors::rgb(18, 22, 36),
    Colors::rgb(30, 32, 36),
    Colors::rgb(40, 44, 52),
    Colors::rgb(20, 22, 26),
    Colors::rgb(20, 22, 26),
    {140, 155, 200},
    70,
    10,
    -36,
    {48, 52, 68},
};

const EnvPalette kCityDawn = {
    Colors::rgb(108, 98, 118),
    Colors::rgb(48, 50, 54),
    Colors::rgb(62, 68, 78),
    Colors::rgb(30, 32, 36),
    Colors::rgb(30, 32, 36),
    {255, 190, 150},
    100,
    16,
    -20,
    {82, 78, 88},
};

const EnvPalette* kCityCycle[] = {
    &kCityDay, &kCityTwilight, &kCityNight, &kCityDawn,
};

constexpr int kCycleKeyframeCount = 4;

float clamp01(float t) {
    if (t <= 0.0f) {
        return 0.0f;
    }
    if (t >= 1.0f) {
        return 1.0f;
    }
    return t;
}

uint16_t lerpRgb565(uint16_t a, uint16_t b, float t) {
    t = clamp01(t);
    const int ar = ((a >> 11) & 0x1F) << 3;
    const int ag = ((a >> 5) & 0x3F) << 2;
    const int ab = (a & 0x1F) << 3;
    const int br = ((b >> 11) & 0x1F) << 3;
    const int bg = ((b >> 5) & 0x3F) << 2;
    const int bb = (b & 0x1F) << 3;
    return Colors::rgb(
        static_cast<uint8_t>(ar + static_cast<int>((br - ar) * t)),
        static_cast<uint8_t>(ag + static_cast<int>((bg - ag) * t)),
        static_cast<uint8_t>(ab + static_cast<int>((bb - ab) * t)));
}

Renderer::Color lerpColor(Renderer::Color a, Renderer::Color b, float t) {
    t = clamp01(t);
    return {
        static_cast<uint8_t>(a.r + static_cast<int>((b.r - a.r) * t)),
        static_cast<uint8_t>(a.g + static_cast<int>((b.g - a.g) * t)),
        static_cast<uint8_t>(a.b + static_cast<int>((b.b - a.b) * t)),
    };
}

int32_t lerpI32(int32_t a, int32_t b, float t) {
    return static_cast<int32_t>(lroundf(a + (b - a) * clamp01(t)));
}

uint16_t lerpU16(uint16_t a, uint16_t b, float t) {
    return static_cast<uint16_t>(lroundf(a + (b - a) * clamp01(t)));
}

void lerpEnvPaletteImpl(const EnvPalette& a, const EnvPalette& b, float t, EnvPalette& out) {
    out.sky = lerpRgb565(a.sky, b.sky, t);
    out.grass = lerpRgb565(a.grass, b.grass, t);
    out.treeFoliage = lerpRgb565(a.treeFoliage, b.treeFoliage, t);
    out.treeTrunk = lerpRgb565(a.treeTrunk, b.treeTrunk, t);
    out.rock = lerpRgb565(a.rock, b.rock, t);
    out.sunColor = lerpColor(a.sunColor, b.sunColor, t);
    out.sunIntensity = lerpU16(a.sunIntensity, b.sunIntensity, t);
    out.sunAzimuth = lerpI32(a.sunAzimuth, b.sunAzimuth, t);
    out.sunElevation = lerpI32(a.sunElevation, b.sunElevation, t);
    out.ambientColor = lerpColor(a.ambientColor, b.ambientColor, t);
}

} // namespace

void lerpEnvPalette(const EnvPalette& a, const EnvPalette& b, float t, EnvPalette& out) {
    lerpEnvPaletteImpl(a, b, t, out);
}

namespace {

const EnvPalette& paletteForThemeLighting(MapTheme theme, EnvLightingMode mode) {
    if (theme == MapTheme::DESERT) {
        switch (mode) {
        case EnvLightingMode::Twilight: return kDesertTwilight;
        case EnvLightingMode::Night: return kDesertNight;
        case EnvLightingMode::Day:
        default: return kDesertDay;
        }
    }
    if (theme == MapTheme::CITY) {
        switch (mode) {
        case EnvLightingMode::Twilight: return kCityTwilight;
        case EnvLightingMode::Night: return kCityNight;
        case EnvLightingMode::Day:
        default: return kCityDay;
        }
    }

    switch (mode) {
    case EnvLightingMode::Twilight: return kRuralTwilight;
    case EnvLightingMode::Night: return kRuralNight;
    case EnvLightingMode::Day:
    default: return kRuralDay;
    }
}

const EnvPalette* const* cycleForTheme(MapTheme theme) {
    if (theme == MapTheme::DESERT) {
        return kDesertCycle;
    }
    if (theme == MapTheme::CITY) {
        return kCityCycle;
    }
    return kRuralCycle;
}

} // namespace

const EnvPalette& envPaletteFor(MapTheme theme, EnvLightingMode mode) {
    return paletteForThemeLighting(theme, mode);
}

void envPaletteAtCyclePhase(MapTheme theme, float phase01, EnvPalette& out) {
    phase01 -= floorf(phase01);
    if (phase01 < 0.0f) {
        phase01 += 1.0f;
    }

    const float scaled = phase01 * static_cast<float>(kCycleKeyframeCount);
    int segment = static_cast<int>(scaled);
    if (segment >= kCycleKeyframeCount) {
        segment = 0;
    }

    const float localT = scaled - static_cast<float>(segment);
    const EnvPalette* const* cycle = cycleForTheme(theme);
    const EnvPalette& from = *cycle[segment];
    const EnvPalette& to = *cycle[(segment + 1) % kCycleKeyframeCount];
    lerpEnvPaletteImpl(from, to, localT, out);
}

} // namespace Game
