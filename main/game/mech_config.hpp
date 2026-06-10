#pragma once

#include <cstdint>

namespace Game {

/// Body segment slot for a bipedal mech (Gundam-style rig).
enum class MechPartSlot : uint8_t {
    FootL,
    FootR,
    LegLowerL,
    LegLowerR,
    LegUpperL,
    LegUpperR,
    Pelvis,
    Torso,
    Head,
    ShoulderL,
    ShoulderR,
    ArmUpperL,
    ArmUpperR,
    ArmLowerL,
    ArmLowerR,
    COUNT
};

enum class MechPrimitiveKind : uint8_t {
    Cube,
    Pyramid,
    Sphere,
    Capsule,
    Cylinder,
};

/// Stat deltas applied when a component or weapon is equipped.
struct MechStatMods {
    float speed = 0.0f;
    float turnRate = 0.0f;
    int hp = 0;
    float hitWidth = 0.0f;
};

/// One swappable visual + gameplay module (leg, arm, torso, etc.).
struct MechComponentDef {
    const char* id = nullptr;
    MechPartSlot slot = MechPartSlot::Torso;
    MechPrimitiveKind primitive = MechPrimitiveKind::Cube;
    int16_t dimA = 8;
    int16_t dimB = 8;
    int16_t dimC = 8;
    uint16_t color = 0;
    float localX = 0.0f;
    float localY = 0.0f;
    float localZ = 0.0f;
    /// Euler rotation in degrees (X=pitch, Y=yaw, Z=roll), pivoted at mesh center.
    int16_t localRotX = 0;
    int16_t localRotY = 0;
    int16_t localRotZ = 0;
    MechStatMods stats;
    /// Next tier when upgraded; nullptr if already max.
    const MechComponentDef* upgradeTo = nullptr;
};

/// Chassis / frame — base stats before components and weapons.
struct MechFrameDef {
    const char* id = nullptr;
    const char* displayName = nullptr;
    float baseSpeed = 120.0f;
    float baseTurnRate = 90.0f;
    float hitWidth = 28.0f;
    int baseMaxHp = 15;
    int8_t visualPitch = -8;
};

enum class WeaponKind : uint8_t {
    HomingMissile,
    StraightBolt,
};

/// Weapon module (mount stats + fire profile).
struct WeaponDef {
    const char* id = nullptr;
    const char* displayName = nullptr;
    WeaponKind kind = WeaponKind::HomingMissile;
    float cooldown = 0.35f;
    float range = 350.0f;
    float aimConeDeg = 45.0f;
    float muzzleLocalX = 22.0f;
    float muzzleLocalY = 22.0f;
    float muzzleLocalZ = 0.0f;
    MechStatMods stats;
    const WeaponDef* upgradeTo = nullptr;
};

static constexpr int MECH_WEAPON_SLOTS = 2;

/// Factory preset: frame + default parts + weapons.
struct MechLoadoutPreset {
    const char* id = nullptr;
    const MechFrameDef* frame = nullptr;
    const MechComponentDef* const* parts = nullptr;
    uint8_t partCount = 0;
    const WeaponDef* primaryWeapon = nullptr;
    const WeaponDef* secondaryWeapon = nullptr;
};

} // namespace Game
