#include "mech_rig.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Game {

MechRig::MechRig(Renderer::Scene& scene)
    : m_scene(scene)
{
}

Renderer::Object* MechRig::createPartMesh(const MechComponentDef& def,
                                          Renderer::Material* mat) {
    mat->shadingMode = Renderer::ShadingMode::GOURAUD;
    switch (def.primitive) {
    case MechPrimitiveKind::Cube:
        return Primitives::createCube(def.dimA, def.dimB, def.dimC, mat);
    case MechPrimitiveKind::Pyramid:
        return Primitives::createPyramid(def.dimA, def.dimB, mat);
    case MechPrimitiveKind::Sphere:
        return Primitives::createSphere(def.dimA, 3, mat);
    case MechPrimitiveKind::Capsule:
        return Primitives::createCapsule(def.dimA, def.dimB, 3, mat);
    case MechPrimitiveKind::Cylinder:
        return Primitives::createCylinder(def.dimA, def.dimB, 3, true, mat);
    }
    return Primitives::createCube(4, 4, 4, mat);
}

void MechRig::rebuild(const MechLoadout& loadout) {
    for (uint8_t i = 0; i < static_cast<uint8_t>(MechPartSlot::COUNT); ++i) {
        const MechPartSlot slot = static_cast<MechPartSlot>(i);
        const MechComponentDef* def = loadout.part(slot);
        PartInstance& inst = m_parts[i];

        if (!def) {
            if (inst.obj) {
                inst.obj->setPosition(0, -1000, 0);
            }
            inst.built = false;
            continue;
        }

        inst.slot = slot;
        inst.mat = Renderer::Material(def->color);
        if (!inst.obj) {
            inst.obj = createPartMesh(*def, &inst.mat);
            inst.obj->cullingMode = Renderer::CullingMode::NO_CULLING;
            m_scene.addObject(inst.obj);
        } else {
            for (auto& tri : inst.obj->triangles) {
                tri.material = &inst.mat;
            }
            inst.mat.color = def->color;
        }
        inst.built = true;
    }
}

void MechRig::transformLocalToWorld(float bodyX, float bodyZ, float baseY,
                                    float angle, int8_t pitch,
                                    float localX, float localY, float localZ,
                                    float& worldX, float& worldY,
                                    float& worldZ) const {
    const float pitchRad = pitch * M_PI / 180.0f;
    const float yaw = angle * M_PI / 180.0f;
    const float cx = cosf(pitchRad), sx = sinf(pitchRad);
    const float cy = cosf(yaw), sy = sinf(yaw);

    const float kx = localX;
    const float ky = localY * cx - localZ * sx;
    const float kz = localY * sx + localZ * cx;

    worldX = bodyX + kx * cy + kz * sy;
    worldY = baseY + ky;
    worldZ = bodyZ + -kx * sy + kz * cy;
}

void MechRig::placePart(PartInstance& inst, float bodyX, float bodyZ, float baseY,
                        float angle, int8_t pitch,
                        float localX, float localY, float localZ) {
    if (!inst.obj || !inst.built) return;

    float wx, wy, wz;
    transformLocalToWorld(bodyX, bodyZ, baseY, angle, pitch,
                          localX, localY, localZ, wx, wy, wz);
    inst.obj->setPosition((int16_t)wx, (int16_t)wy, (int16_t)wz);
    inst.obj->setRotation(pitch, (int16_t)angle, 0);
}

void MechRig::updatePose(float x, float z, float baseY, float angle, int8_t pitch,
                         const MechLoadout& loadout) {
    for (uint8_t i = 0; i < static_cast<uint8_t>(MechPartSlot::COUNT); ++i) {
        const MechPartSlot slot = static_cast<MechPartSlot>(i);
        const MechComponentDef* def = loadout.part(slot);
        PartInstance& inst = m_parts[i];
        if (!def || !inst.built) {
            if (inst.obj) inst.obj->setPosition(0, -1000, 0);
            continue;
        }

        placePart(inst, x, z, baseY, angle, pitch,
                  def->localX, def->localY, def->localZ);
    }
}

void MechRig::setHidden(bool hidden) {
    const int16_t y = hidden ? -1000 : 0;
    for (auto& inst : m_parts) {
        if (inst.obj && inst.built) {
            inst.obj->setPosition(0, y, 0);
        }
    }
}

void MechRig::weaponMuzzleLocal(const WeaponDef& weapon,
                                float& lx, float& ly, float& lz) const {
    lx = weapon.muzzleLocalX;
    ly = weapon.muzzleLocalY;
    lz = weapon.muzzleLocalZ;
}

} // namespace Game
