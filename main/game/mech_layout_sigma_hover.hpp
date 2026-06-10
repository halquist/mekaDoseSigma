#pragma once

/// Sigma hover variant — edit part rows here.
#include "mech_layout_macros.hpp"

namespace Game {
namespace MechCatalog {

MECH_PART( thruster_l,     FootL,     Cube,    5,  3,  7, MECH_BLUE,  -6,  8, -1, 0, 0, 0, MECH_STAT_NONE )
MECH_PART( thruster_r,     FootR,     Cube,    5,  3,  7, MECH_BLUE,   6,  8, -1, 0, 0, 0, MECH_STAT_NONE )
MECH_PART( torso_hover,    Torso,     Pyramid, 11, 18,  0, MECH_WHITE,  0, 31,  0, 0, 0, 0, MECH_STAT(0, 0, 2, 0) )
MECH_PART( head_hover,     Head,      Cube,    7,  7,  7, MECH_WHITE,  0, 36,  1, 0, 0, 0, MECH_STAT_NONE )
MECH_PART( shoulder_l_hover, ShoulderL, Cube,  4,  3,  5, MECH_RED,   -9, 32,  0, 0, 0, 0, MECH_STAT_NONE )
MECH_PART( shoulder_r_hover, ShoulderR, Cube,  4,  3,  5, MECH_RED,    9, 32,  0, 0, 0, 0, MECH_STAT_NONE )
MECH_PART( arm_l_hover,    ArmUpperL, Cube,    3,  9,  3, MECH_WHITE,-11, 26,  0, 0, 0, 0, MECH_STAT_NONE )
MECH_PART( arm_r_hover,    ArmUpperR, Cube,    3,  9,  3, MECH_WHITE, 11, 26,  0, 0, 0, 0, MECH_STAT_NONE )

} // namespace MechCatalog
} // namespace Game
