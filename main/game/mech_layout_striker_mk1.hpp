#pragma once

/// Full biped Mk1 rig — heavier (capsules). Edit rows here to tweak look.
#include "mech_layout_macros.hpp"

namespace Game {
namespace MechCatalog {

// Uses FRAME_striker from mech_layout_striker_hover.hpp (included first in catalog.cpp)

// ---- legs & feet ----
MECH_PART( foot_l_mk1,       FootL,     Cube,    10,  4, 14, MECH_JOINT,  -9,  3,  3, 0, 0, 0, MECH_STAT_NONE )
MECH_PART( foot_r_mk1,       FootR,     Cube,    10,  4, 14, MECH_JOINT,   9,  3,  3, 0, 0, 0, MECH_STAT_NONE )
MECH_PART( leg_lower_l_mk1,  LegLowerL, Capsule,  5, 14,  0, MECH_WHITE,  -9, 12,  0, 0, 0, 0, MECH_STAT_NONE )
MECH_PART( leg_lower_r_mk1,  LegLowerR, Capsule,  5, 14,  0, MECH_WHITE,   9, 12,  0, 0, 0, 0, MECH_STAT_NONE )
MECH_PART( leg_upper_l_mk2,  LegUpperL, Capsule,  6, 16,  0, MECH_BLUE,   -9, 24,  0, 0, 0, 0, MECH_STAT(8, 0, 0, 0) )
MECH_PART( leg_upper_r_mk2,  LegUpperR, Capsule,  6, 16,  0, MECH_BLUE,    9, 24,  0, 0, 0, 0, MECH_STAT(8, 0, 0, 0) )
MECH_PART_UPG( leg_upper_l_mk1, LegUpperL, Capsule, 6, 14, 0, MECH_BLUE, -9, 24, 0, 0, 0, 0, leg_upper_l_mk2, MECH_STAT(4, 0, 0, 0) )
MECH_PART_UPG( leg_upper_r_mk1, LegUpperR, Capsule, 6, 14, 0, MECH_BLUE,  9, 24, 0, 0, 0, 0, leg_upper_r_mk2, MECH_STAT(4, 0, 0, 0) )

// ---- upper body ----
MECH_PART( torso_mk1,       Torso,     Cube,    20, 22, 14, MECH_WHITE,   0, 32,  0, 0, 0, 0, MECH_STAT(0, 0, 2, 0) )
MECH_PART( head_mk1,         Head,      Cube,    12, 10, 10, MECH_WHITE,  0, 48,  2, 0, 0, 0, MECH_STAT_NONE )
MECH_PART( shoulder_l_mk1,   ShoulderL, Cube,     8,  6, 10, MECH_RED,  -15, 40,  0, 0, 0, 0, MECH_STAT_NONE )
MECH_PART( shoulder_r_mk1,   ShoulderR, Cube,     8,  6, 10, MECH_RED,   15, 40,  0, 0, 0, 0, MECH_STAT_NONE )
MECH_PART( arm_upper_l_mk1,   ArmUpperL, Capsule,  4, 12,  0, MECH_WHITE,-18, 34,  0, 0, 0, 0, MECH_STAT_NONE )
MECH_PART( arm_upper_r_mk1,   ArmUpperR, Capsule,  4, 12,  0, MECH_WHITE, 18, 34,  0, 0, 0, 0, MECH_STAT_NONE )
MECH_PART( arm_lower_l_mk1,   ArmLowerL, Capsule,  4, 12,  0, MECH_BLUE, -20, 24,  0, 0, 0, 0, MECH_STAT_NONE )
MECH_PART( arm_lower_r_mk1,   ArmLowerR, Capsule,  4, 12,  0, MECH_BLUE,  20, 24,  0, 0, 0, 0, MECH_STAT_NONE )

static const MechComponentDef* const STRIKER_MK1_PARTS[] = {
    &PART_foot_l_mk1,
    &PART_foot_r_mk1,
    &PART_leg_lower_l_mk1,
    &PART_leg_lower_r_mk1,
    &PART_leg_upper_l_mk1,
    &PART_leg_upper_r_mk1,
    &PART_torso_mk1,
    &PART_head_mk1,
    &PART_shoulder_l_mk1,
    &PART_shoulder_r_mk1,
    &PART_arm_upper_l_mk1,
    &PART_arm_upper_r_mk1,
    &PART_arm_lower_l_mk1,
    &PART_arm_lower_r_mk1,
};

const MechLoadoutPreset LOADOUT_STRIKER_MK1 = {
    "loadout_striker_mk1",
    &FRAME_striker,
    STRIKER_MK1_PARTS,
    static_cast<uint8_t>(sizeof(STRIKER_MK1_PARTS) / sizeof(STRIKER_MK1_PARTS[0])),
    &WEAPON_homing_missile_mk1,
    nullptr,
};

} // namespace MechCatalog
} // namespace Game
