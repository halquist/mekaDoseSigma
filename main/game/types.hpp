#pragma once

#include <cstdint>

namespace Game {

namespace Colors {
    constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
        return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    }

    constexpr uint16_t BLACK         = 0x0000;
    // constexpr uint16_t SKY_BLUE      = rgb(135, 200, 235);
    constexpr uint16_t SKY_BLUE      = rgb(90, 140, 180);
    constexpr uint16_t GRASS         = rgb(72, 140, 55);
    constexpr uint16_t GRASS_LIGHT   = GRASS;
    constexpr uint16_t GRASS_DARK    = GRASS;
    constexpr uint16_t GRAY          = rgb(100, 105, 95);
    constexpr uint16_t OLIVE         = rgb(80, 100, 60);
    constexpr uint16_t COCKPIT_GLASS = rgb(70, 120, 140);
    constexpr uint16_t STEEL_BLUE    = rgb(90, 140, 180);
    constexpr uint16_t TRACER_YELLOW = rgb(255, 230, 120);
    constexpr uint16_t ENEMY_BODY    = rgb(180, 70, 60);
    constexpr uint16_t AIR_JET       = rgb(120, 130, 150);
    constexpr uint16_t BLIMP_BODY    = rgb(188, 192, 198);
    constexpr uint16_t BLIMP_COCKPIT = rgb(42, 46, 52);
    constexpr uint16_t BOMB_BODY     = rgb(45, 48, 42);
    constexpr uint16_t TANK_BODY     = rgb(60, 70, 45);
    constexpr uint16_t TANK_TURRET   = rgb(45, 52, 35);
    constexpr uint16_t TANK_BARREL   = rgb(35, 40, 28);
    constexpr uint16_t ORANGE        = rgb(255, 120, 40);
    constexpr uint16_t HUD_BG        = rgb(40, 50, 35);
    constexpr uint16_t HUD_TEXT      = rgb(230, 235, 220);
    constexpr uint16_t HEALTH_FILL   = rgb(80, 160, 60);
    constexpr uint16_t DAMAGE_RED    = rgb(200, 40, 30);
    constexpr uint16_t VICTORY_GREEN = rgb(60, 180, 70);
    constexpr uint16_t SPARK_ORANGE  = rgb(255, 160, 60);
    constexpr uint16_t SPARK_RUST    = rgb(200, 90, 50);
    constexpr uint16_t TREE_TRUNK      = rgb(90, 60, 35);
    constexpr uint16_t TREE_FOLIAGE    = rgb(45, 110, 40);
    constexpr uint16_t ROCK            = rgb(95, 90, 85);
    constexpr uint16_t MISSILE_GREY    = rgb(180, 185, 190);
    constexpr uint16_t MISSILE_TRAIL   = rgb(255, 30, 220);
    constexpr uint16_t ENEMY_MISSILE_TRAIL = rgb(40, 255, 90);
    constexpr uint16_t MECH_WHITE      = rgb(235, 235, 245);
    constexpr uint16_t MECH_BLUE       = rgb(40, 70, 180);
    constexpr uint16_t SHIELD_BLUE     = rgb(50, 110, 220);
    constexpr uint16_t SHIELD_HUD      = rgb(70, 130, 235);
    constexpr uint16_t SHIELD_ENEMY    = rgb(130, 220, 150);
    constexpr uint16_t MECH_RED        = rgb(200, 45, 55);
    constexpr uint16_t MECH_JOINT      = rgb(55, 58, 65);
    constexpr uint16_t OBJECTIVE_BODY    = rgb(105, 110, 115);
    constexpr uint16_t OBJECTIVE_DOME    = rgb(255, 30, 220);
    constexpr uint16_t OBJECTIVE_ARROW   = rgb(255, 210, 60);
    constexpr uint16_t PORTAL_VOID       = rgb(255, 40, 200);
    constexpr uint16_t PORTAL_FRAME      = rgb(110, 110, 118);
    constexpr uint16_t PORTAL_ARROW      = rgb(255, 120, 220);
    constexpr uint16_t CITY_GROUND       = rgb(68, 70, 74);
    constexpr uint16_t CITY_ROAD         = rgb(32, 34, 38);
    constexpr uint16_t CITY_BUILDING     = rgb(100, 108, 120);
    constexpr uint16_t CITY_SKY          = rgb(78, 88, 102);
    constexpr uint16_t INDUSTRIAL_GROUND = rgb(88, 86, 82);

    // Legacy aliases kept for gradual migration
    constexpr uint16_t CYAN          = STEEL_BLUE;
    constexpr uint16_t HOT_PINK      = SPARK_RUST;
    constexpr uint16_t DARK_BLUE     = SKY_BLUE;
}

enum class GameState {
    MENU,
    MENU_TRANSITION,
    PLAYING,
    PORTAL_TRANSITION,
    UPGRADE_PICK,
    VICTORY,
    DEFEAT
};

struct TouchInput {
    bool touched = false;
    int16_t x = 0;
    int16_t y = 0;
};

} // namespace Game
