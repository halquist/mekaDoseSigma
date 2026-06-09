#pragma once

#include "Jet.hpp"
#include "mech_config.hpp"
#include "mech_loadout.hpp"
#include <array>

namespace Game {

/// Builds and poses all mech part meshes from a loadout.
class MechRig {
public:
    explicit MechRig(Renderer::Scene& scene);

    void rebuild(const MechLoadout& loadout);
    void updatePose(float x, float z, float baseY, float angle, int8_t pitch,
                    const MechLoadout& loadout);
    void setHidden(bool hidden);

    void transformLocalToWorld(float bodyX, float bodyZ, float baseY, float angle,
                               int8_t pitch, float localX, float localY, float localZ,
                               float& worldX, float& worldY, float& worldZ) const;

    void weaponMuzzleLocal(const WeaponDef& weapon, float& lx, float& ly, float& lz) const;

private:
    struct PartInstance {
        Renderer::Object* obj = nullptr;
        Renderer::Material mat;
        MechPartSlot slot = MechPartSlot::Torso;
        bool built = false;
    };

    Renderer::Object* createPartMesh(const MechComponentDef& def, Renderer::Material* mat);
    void placePart(PartInstance& inst, float bodyX, float bodyZ, float baseY,
                   float angle, int8_t pitch,
                   float localX, float localY, float localZ);

    Renderer::Scene& m_scene;
    std::array<PartInstance, static_cast<size_t>(MechPartSlot::COUNT)> m_parts;
};

} // namespace Game
