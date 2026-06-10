#pragma once

/// Compile-time helpers for mech visual configs.
///
/// All MECH_* macros expand to const globals — zero runtime cost, same as hand-written defs.
///
/// Body-local coordinates (edit x/y/z in layout files):
///   +Y = up          +Z = forward (nose)
///   +X = right       -Z = back (missile bay)
///   Origin ≈ hover base between thrusters
///   x/y/z = position of the part's mesh CENTER (meshes are auto-centered in the rig)
///
/// Rotation (rx/ry/rz): degrees in body-local space (applied before frame lean + body facing).
///   rx = pitch   (e.g. 180 = flip upside down)
///   ry = yaw     (spin relative to body forward)
///   rz = roll    (splay outward; stays consistent when the mech turns)
///
/// Shapes: Cube | Pyramid | Capsule | Sphere | Cylinder
///   Cube: w, h, d = full width, height, depth
///   Pyramid: w = base size, h = height (d unused)
///   Sphere: w = radius, h = segment count (optional, default 6; use 6–10 for smooth)
///   Capsule: w = radius, h = cylinder height (d unused)
///   Cylinder: w = radius, h = height (d unused)
///
/// Example row:
///   MECH_PART( torso, Torso, Pyramid, 11, 18, 0, MECH_WHITE, 0, 31, 0, MECH_ROT(180,0,0), MECH_STAT(0,0,2,0) )

#include "mech_config.hpp"
#include "types.hpp"

namespace Game {
namespace MechLayout {

#define MECH_STAT(speed, turn, hp, hitW) MechStatMods{ speed, turn, hp, hitW }
#define MECH_STAT_NONE MechStatMods{}
#define MECH_ROT(rx, ry, rz) static_cast<int16_t>(rx), static_cast<int16_t>(ry), static_cast<int16_t>(rz)
#define MECH_ROT_NONE MECH_ROT(0, 0, 0)

#define MECH_PART(id, slot, shape, w, h, d, color, x, y, z, rx, ry, rz, stats) \
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
        rx,                                                                      \
        ry,                                                                      \
        rz,                                                                      \
        stats,                                                                   \
        nullptr,                                                                 \
    };

#define MECH_PART_UPG(id, slot, shape, w, h, d, color, x, y, z, rx, ry, rz, upgId, stats) \
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
        rx,                                                                      \
        ry,                                                                      \
        rz,                                                                      \
        stats,                                                                   \
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

#define MECH_WEAPON(id, displayName, kind, cooldown, range, cone, mx, my, mz, dmg, ...) \
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
        dmg,                                                                     \
        __VA_ARGS__,                                                             \
        nullptr,                                                                 \
    };

#define MECH_WEAPON_UPG(id, displayName, kind, cooldown, range, cone, mx, my, mz, dmg, upgId, ...) \
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
        dmg,                                                                     \
        __VA_ARGS__,                                                             \
        &WEAPON_##upgId,                                                         \
    };

} // namespace MechLayout
} // namespace Game
