#pragma once

#include "Jet.hpp"
#include "mech_config.hpp"
#include "mech_loadout.hpp"
#include <array>

namespace Game {

enum class MechPalette : uint8_t {
    Default,
    EnemyRed,
};

/// Builds and poses all mech part meshes from a loadout.
class MechRig {
public:
    explicit MechRig(Renderer::Scene& scene);

    void rebuild(const MechLoadout& loadout, MechPalette palette = MechPalette::Default);
    void updatePose(int32_t x, int32_t z, float baseY, int32_t angle, int8_t pitch,
                    const MechLoadout& loadout);
    void setHidden(bool hidden);
    void ensureBuilt(const MechLoadout& loadout, MechPalette palette = MechPalette::Default);

    void transformLocalToWorld(float bodyX, float bodyZ, float baseY, float angle,
                               int8_t pitch, float localX, float localY, float localZ,
                               float& worldX, float& worldY, float& worldZ) const;

    void weaponMuzzleLocal(const WeaponDef& weapon, float& lx, float& ly, float& lz) const;

    Renderer::Object* bodyObject() const { return m_bodyObj; }

private:
    struct PartMaterial {
        Renderer::Material mat;
        bool active = false;
    };

    Renderer::Object* createPartMesh(const MechComponentDef& def, Renderer::Material* mat);
    void appendPartToBody(Renderer::Object& body, const MechComponentDef& def,
                          Renderer::Material* mat);
    void hideBodyObject();

    Renderer::Scene& m_scene;
    Renderer::Object* m_bodyObj = nullptr;
    std::array<PartMaterial, static_cast<size_t>(MechPartSlot::COUNT)> m_partMats;
    bool m_bodyBuilt = false;
    bool m_hidden = false;
};

} // namespace Game
