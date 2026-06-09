#pragma once

/// Compile-time helpers for mech visual configs.
///
/// All MECH_* macros expand to const globals — zero runtime cost, same as hand-written defs.
///
/// Body-local coordinates (edit x/y/z in layout files):
///   +Y = up          +Z = forward (nose)
///   +X = right       -Z = back (missile bay)
///   Origin ≈ hover base between thrusters
///
/// Shapes: Cube | Pyramid | Capsule | Sphere | Cylinder
///   Cube/Capsule/Cylinder: w, h, d  (Pyramid: w = base, h = height; Capsule: w = radius)
///
/// Example row (columns line up for quick scanning):
///   MECH_PART( thruster_l, FootL, Cube, 5, 3, 7, MECH_BLUE, -6, 8, -1 )
///   MECH_PART( torso,      Torso, Pyramid, 11, 18, 0, MECH_WHITE, 0, 22, 0, MECH_STAT(0,0,2,0) )

#include "mech_config.hpp"
#include "types.hpp"

namespace Game {
namespace MechLayout {

#define MECH_STAT(speed, turn, hp, hitW) MechStatMods{ speed, turn, hp, hitW }
#define MECH_STAT_NONE MechStatMods{}

#define MECH_PART(id, slot, shape, w, h, d, color, x, y, z, ...)                \
    const MechComponentDef PART_##id = {                                         \
        #id,                                                                     \
        MechPartSlot::slot,                                                      \
        MechPrimitiveKind::shape,                                                \
        static_cast<int16_t>(w),                                                 \
        static_cast<int16_t>(h),                                                 \
        static_cast<int16_t>(d),                                                 \
        Colors::color,                                                           \
        x,                                                                       \
        y,                                                                       \
        z,                                                                       \
        0,                                                                       \
        __VA_ARGS__,                                                             \
        nullptr,                                                                 \
    };

#define MECH_PART_UPG(id, slot, shape, w, h, d, color, x, y, z, upgId, ...)    \
    const MechComponentDef PART_##id = {                                         \
        #id,                                                                     \
        MechPartSlot::slot,                                                      \
        MechPrimitiveKind::shape,                                                \
        static_cast<int16_t>(w),                                                 \
        static_cast<int16_t>(h),                                                 \
        static_cast<int16_t>(d),                                                 \
        Colors::color,                                                           \
        x,                                                                       \
        y,                                                                       \
        z,                                                                       \
        0,                                                                       \
        __VA_ARGS__,                                                             \
        &PART_##upgId,                                                           \
    };

#define MECH_FRAME(id, displayName, speed, turn, hitW, hp, pitch)               \
    const MechFrameDef FRAME_##id = {                                            \
        "frame_" #id,                                                            \
        displayName,                                                             \
        speed,                                                                   \
        turn,                                                                    \
        hitW,                                                                    \
        hp,                                                                      \
        pitch,                                                                   \
    };

#define MECH_WEAPON(id, displayName, kind, cooldown, range, cone, mx, my, mz, ...) \
    const WeaponDef WEAPON_##id = {                                              \
        "weapon_" #id,                                                           \
        displayName,                                                             \
        WeaponKind::kind,                                                        \
        cooldown,                                                                  \
        range,                                                                   \
        cone,                                                                    \
        mx,                                                                      \
        my,                                                                      \
        mz,                                                                      \
        __VA_ARGS__,                                                             \
        nullptr,                                                                 \
    };

#define MECH_WEAPON_UPG(id, displayName, kind, cooldown, range, cone, mx, my, mz, upgId, ...) \
    const WeaponDef WEAPON_##id = {                                              \
        "weapon_" #id,                                                           \
        displayName,                                                             \
        WeaponKind::kind,                                                        \
        cooldown,                                                                  \
        range,                                                                   \
        cone,                                                                    \
        mx,                                                                      \
        my,                                                                      \
        mz,                                                                      \
        __VA_ARGS__,                                                             \
        &WEAPON_##upgId,                                                         \
    };

} // namespace MechLayout
} // namespace Game
