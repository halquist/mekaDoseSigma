#pragma once

/// Striker hover rig — edit part rows here to tweak look (compile-time only).
#include "mech_layout_macros.hpp"

namespace Game {
namespace MechCatalog {

MECH_FRAME(striker, "Striker Frame", 118.0f, 88.0f, 24.0f, 15, -10);

// ---- parts: id / slot / shape / w h d / color / x y z / stats ----
MECH_PART( thruster_l,     FootL,     Cube,    5,  3,  7, MECH_BLUE,  -6,  8, -1, MECH_STAT_NONE )
MECH_PART( thruster_r,     FootR,     Cube,    5,  3,  7, MECH_BLUE,   6,  8, -1, MECH_STAT_NONE )
MECH_PART( torso_hover,    Torso,     Pyramid, 11, 18,  0, MECH_WHITE,  0, 22,  0, MECH_STAT(0, 0, 2, 0) )
MECH_PART( head_hover,     Head,      Cube,    7,  7,  7, MECH_WHITE,  0, 36,  1, MECH_STAT_NONE )
MECH_PART( shoulder_l_hover, ShoulderL, Cube,  4,  3,  5, MECH_RED,   -9, 32,  0, MECH_STAT_NONE )
MECH_PART( shoulder_r_hover, ShoulderR, Cube,  4,  3,  5, MECH_RED,    9, 32,  0, MECH_STAT_NONE )
MECH_PART( arm_l_hover,    ArmUpperL, Cube,    3,  9,  3, MECH_WHITE,-11, 26,  0, MECH_STAT_NONE )
MECH_PART( arm_r_hover,    ArmUpperR, Cube,    3,  9,  3, MECH_WHITE, 11, 26,  0, MECH_STAT_NONE )

// ---- weapons: muzzle x,y,z = local spawn (missiles: center back) ----
MECH_WEAPON( homing_missile_mk2, "Homing Missile Mk2", HomingMissile,
             0.28f, 380.0f, 50.0f,  0.0f, 29.0f, -7.0f, MECH_STAT_NONE )

MECH_WEAPON_UPG( homing_missile_mk1, "Homing Missile Mk1", HomingMissile,
                 0.35f, 350.0f, 45.0f,  0.0f, 29.0f, -7.0f,
                 homing_missile_mk2, MECH_STAT_NONE )

MECH_WEAPON( rapid_bolt_mk1, "Rapid Bolt", StraightBolt,
             0.18f, 280.0f, 55.0f, 14.0f, 28.0f,  0.0f, MECH_STAT(-4, 0, 0, 0) )

static const MechComponentDef* const STRIKER_HOVER_PARTS[] = {
    &PART_thruster_l,
    &PART_thruster_r,
    &PART_torso_hover,
    &PART_head_hover,
    &PART_shoulder_l_hover,
    &PART_shoulder_r_hover,
    &PART_arm_l_hover,
    &PART_arm_r_hover,
};

const MechLoadoutPreset LOADOUT_STRIKER_HOVER = {
    "loadout_striker_hover",
    &FRAME_striker,
    STRIKER_HOVER_PARTS,
    static_cast<uint8_t>(sizeof(STRIKER_HOVER_PARTS) / sizeof(STRIKER_HOVER_PARTS[0])),
    &WEAPON_homing_missile_mk1,
    nullptr,
};

} // namespace MechCatalog
} // namespace Game
