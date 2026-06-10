#pragma once

#include "mech_config.hpp"

namespace Game {

namespace MechCatalog {

// --- Frames ---
extern const MechFrameDef FRAME_striker;
#define FRAME_STRIKER FRAME_striker

// --- Striker hover rig (default) — tweak in mech_layout_striker_hover.hpp ---
extern const MechComponentDef PART_thruster_l;
extern const MechComponentDef PART_thruster_r;
extern const MechComponentDef PART_torso_hover;
extern const MechComponentDef PART_head_hover;
extern const MechComponentDef PART_shoulder_l_hover;
extern const MechComponentDef PART_shoulder_r_hover;
extern const MechComponentDef PART_arm_l_hover;
extern const MechComponentDef PART_arm_r_hover;
extern const MechComponentDef PART_fin_l_up;
extern const MechComponentDef PART_fin_l_mid;
extern const MechComponentDef PART_fin_l_low;
extern const MechComponentDef PART_fin_r_up;
extern const MechComponentDef PART_fin_r_mid;
extern const MechComponentDef PART_fin_r_low;

#define PART_THRUSTER_L PART_thruster_l
#define PART_THRUSTER_R PART_thruster_r
#define PART_TORSO_HOVER PART_torso_hover
#define PART_HEAD_HOVER PART_head_hover
#define PART_SHOULDER_L_HOVER PART_shoulder_l_hover
#define PART_SHOULDER_R_HOVER PART_shoulder_r_hover
#define PART_ARM_L_HOVER PART_arm_l_hover
#define PART_ARM_R_HOVER PART_arm_r_hover

// --- Full biped Mk1 — tweak in mech_layout_striker_mk1.hpp ---
extern const MechComponentDef PART_foot_l_mk1;
extern const MechComponentDef PART_foot_r_mk1;
extern const MechComponentDef PART_leg_lower_l_mk1;
extern const MechComponentDef PART_leg_lower_r_mk1;
extern const MechComponentDef PART_leg_upper_l_mk1;
extern const MechComponentDef PART_leg_upper_r_mk1;
extern const MechComponentDef PART_leg_upper_l_mk2;
extern const MechComponentDef PART_leg_upper_r_mk2;
extern const MechComponentDef PART_torso_mk1;
extern const MechComponentDef PART_head_mk1;
extern const MechComponentDef PART_shoulder_l_mk1;
extern const MechComponentDef PART_shoulder_r_mk1;
extern const MechComponentDef PART_arm_upper_l_mk1;
extern const MechComponentDef PART_arm_upper_r_mk1;
extern const MechComponentDef PART_arm_lower_l_mk1;
extern const MechComponentDef PART_arm_lower_r_mk1;

#define PART_FOOT_L_MK1 PART_foot_l_mk1
#define PART_FOOT_R_MK1 PART_foot_r_mk1
#define PART_LEG_LOWER_L_MK1 PART_leg_lower_l_mk1
#define PART_LEG_LOWER_R_MK1 PART_leg_lower_r_mk1
#define PART_LEG_UPPER_L_MK1 PART_leg_upper_l_mk1
#define PART_LEG_UPPER_R_MK1 PART_leg_upper_r_mk1
#define PART_LEG_UPPER_L_MK2 PART_leg_upper_l_mk2
#define PART_LEG_UPPER_R_MK2 PART_leg_upper_r_mk2
#define PART_TORSO_MK1 PART_torso_mk1
#define PART_HEAD_MK1 PART_head_mk1
#define PART_SHOULDER_L_MK1 PART_shoulder_l_mk1
#define PART_SHOULDER_R_MK1 PART_shoulder_r_mk1
#define PART_ARM_UPPER_L_MK1 PART_arm_upper_l_mk1
#define PART_ARM_UPPER_R_MK1 PART_arm_upper_r_mk1
#define PART_ARM_LOWER_L_MK1 PART_arm_lower_l_mk1
#define PART_ARM_LOWER_R_MK1 PART_arm_lower_r_mk1

// --- Weapons ---
extern const WeaponDef WEAPON_homing_missile_mk1;
extern const WeaponDef WEAPON_homing_missile_mk2;
extern const WeaponDef WEAPON_rapid_bolt_mk1;

#define WEAPON_HOMING_MISSILE_MK1 WEAPON_homing_missile_mk1
#define WEAPON_HOMING_MISSILE_MK2 WEAPON_homing_missile_mk2
#define WEAPON_RAPID_BOLT_MK1 WEAPON_rapid_bolt_mk1

// --- Presets ---
extern const MechLoadoutPreset LOADOUT_STRIKER_HOVER;
extern const MechLoadoutPreset LOADOUT_STRIKER_MK1;

const MechComponentDef* findComponentById(const char* id);
const WeaponDef* findWeaponById(const char* id);

} // namespace MechCatalog

} // namespace Game
