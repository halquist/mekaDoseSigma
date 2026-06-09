idf_component_register(
    SRCS "main.cpp"
          "game/game.cpp"
          "game/world.cpp"
          "game/terrain.cpp"
          "game/worldgen.cpp"
          "game/obstacles.cpp"
          "game/input.cpp"
          "game/mech.cpp"
          "game/mech_catalog.cpp"
          "game/mech_loadout.cpp"
          "game/mech_rig.cpp"
          "game/enemy.cpp"
          "game/projectile.cpp"
          "game/particles.cpp"
          "game/digits.cpp"
    INCLUDE_DIRS "."
    REQUIRES driver esp_lcd esp_timer
)
