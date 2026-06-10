#pragma once

/// Striker hover rig — edit part rows here to tweak look (compile-time only).
#include "mech_layout_macros.hpp"

namespace Game {
namespace MechCatalog {

MECH_FRAME(striker, "Striker Frame", 118.0f, 88.0f, 24.0f, 100, -10);

// ---- parts: id / slot / shape / w h d / color / x y z / rx ry rz / stats ----
// y positions are mesh centers (pyramids: old_base_y + height/2)
MECH_PART( thruster_l,     FootL,     Pyramid,  5, 22,  5, MECH_WHITE,  -9, 8, -1, 0, 0, 160, MECH_STAT_NONE )
MECH_PART( thruster_r,     FootR,     Pyramid,  5, 22,  5, MECH_WHITE,   9, 8, -1, 0, 0, 200, MECH_STAT_NONE )
MECH_PART( torso_hover,    Torso,     Pyramid, 14, 8,  6, MECH_WHITE,  0, 31,  0, 0, 0, 180, MECH_STAT_NONE )
MECH_PART( pelvis_hover,    Pelvis,     Pyramid, 10, 8,  0, MECH_BLUE,  0, 25,  0, 0, 0, 0, MECH_STAT_NONE )
MECH_PART( head_hover,     Head,      Sphere,  5,  7,  0, GRAY,  0, 40,  1, 0, 0, 0, MECH_STAT_NONE )
MECH_PART( shoulder_l_hover, ShoulderL, Pyramid,  8,  6,  5, MECH_RED,   -7, 35,  0, 0, 0, 200, MECH_STAT_NONE )
MECH_PART( shoulder_r_hover, ShoulderR, Pyramid,  8,  6,  5, MECH_RED,    7, 35,  0, 0, 0, 160, MECH_STAT_NONE )
MECH_PART( arm_l_hover,    ArmUpperL, Pyramid,  4,  15,  3, MECH_WHITE,-11, 27,  0, 0, 70, 160, MECH_STAT_NONE )
MECH_PART( arm_r_hover,    ArmUpperR, Pyramid,  4,  15,  3, MECH_WHITE, 11, 27,  0, 0, 70, 200, MECH_STAT_NONE )

// Rear stabilizer fins — pyramid base (thick) toward body hub, tip (thin) outward.
// Hub ≈ (0, 32, -4). Pyramid apex starts +Y; rz spins tip in the X/Y fan.
// Mirror rule across body X=0: x → -x, and (rx, ry, rz) → (rx, -ry, -rz)
//   (do NOT negate rx — rotation around X is symmetric under left/right flip)
MECH_PART( fin_l_up,   LegUpperL, Pyramid, 3, 22, 0, SPARK_ORANGE,  -10, 44, -5,  0, 0,  38, MECH_STAT_NONE )
MECH_PART( fin_l_mid,  LegLowerL, Pyramid, 3, 22, 0, SPARK_ORANGE, -14, 34, -5,  0, 0,  72, MECH_STAT_NONE )
MECH_PART( fin_l_low,  LegUpperR, Pyramid, 3, 22, 0, SPARK_ORANGE,  -11, 22, -5,  8, 0, 122, MECH_STAT_NONE )
MECH_PART( fin_r_up,   LegLowerR, Pyramid, 3, 22, 0, SPARK_ORANGE,   10, 44, -5,  0, 0, -38, MECH_STAT_NONE )
MECH_PART( fin_r_mid,  ArmLowerL, Pyramid, 3, 22, 0, SPARK_ORANGE,  14, 34, -5,  0, 0, -72, MECH_STAT_NONE )
MECH_PART( fin_r_low,  ArmLowerR, Pyramid, 3, 22, 0, SPARK_ORANGE,   11, 22, -5,  8, 0,-122, MECH_STAT_NONE )

// ---- weapons: muzzle x,y,z = local spawn (missiles: center back) ----
MECH_WEAPON( homing_missile_mk2, "Homing Missile Mk2", HomingMissile,
             0.28f, 380.0f, 50.0f,  0.0f, 29.0f, -7.0f, 8, MECH_STAT_NONE )

MECH_WEAPON_UPG( homing_missile_mk1, "Homing Missile Mk1", HomingMissile,
                 0.35f, 350.0f, 45.0f,  0.0f, 29.0f, -7.0f, 6,
                 homing_missile_mk2, MECH_STAT_NONE )

MECH_WEAPON( rapid_bolt_mk1, "Rapid Bolt", StraightBolt,
             0.18f, 280.0f, 55.0f, 14.0f, 28.0f,  0.0f, 5, MECH_STAT(-4, 0, 0, 0) )

static const MechComponentDef* const STRIKER_HOVER_PARTS[] = {
    &PART_thruster_l,
    &PART_thruster_r,
    &PART_pelvis_hover,
    &PART_torso_hover,
    &PART_head_hover,
    &PART_shoulder_l_hover,
    &PART_shoulder_r_hover,
    &PART_arm_l_hover,
    &PART_arm_r_hover,
    &PART_fin_l_up,
    &PART_fin_l_mid,
    &PART_fin_l_low,
    &PART_fin_r_up,
    &PART_fin_r_mid,
    &PART_fin_r_low,
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
