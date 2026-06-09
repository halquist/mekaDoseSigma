#pragma once

#include "Jet.hpp"
#include "types.hpp"
#include "input.hpp"
#include "mech_loadout.hpp"
#include "mech_rig.hpp"
#include "mech_catalog.hpp"

namespace Game {

class ObstacleField;
class ProjectileSystem;

class Mech {
public:
    Mech(Renderer::Scene& scene,
         const MechLoadoutPreset& preset = MechCatalog::LOADOUT_STRIKER_HOVER);

    void update(const TouchInput& input, float deltaTime, int screenWidth,
                ObstacleField* obstacles = nullptr);
    void reset();

    MechLoadout& loadout() { return m_loadout; }
    const MechLoadout& loadout() const { return m_loadout; }

    void rebuildVisual();

    bool tryAutoFire(ProjectileSystem& projectiles,
                     float targetX, float targetZ, float targetAimY,
                     bool targetAlive);

    float getX() const { return m_x; }
    float getZ() const { return m_z; }
    float getBaseY() const { return m_baseY; }
    float getAngle() const { return m_angle; }
    float getWidth() const { return m_loadout.hitWidth(); }
    int getMaxHp() const { return m_loadout.maxHp(); }

    bool isAlive() const { return m_alive; }
    void explode();

private:
    void updateVisual();
    void getMuzzleWorld(float& wx, float& wy, float& wz) const;

    Renderer::Scene& m_scene;
    MechLoadout m_loadout;
    MechRig m_rig;

    float m_x = 0;
    float m_z = 0;
    float m_baseY = 0;
    float m_angle = 0;
    float m_fireCooldown = 0;
    bool m_alive = true;
};

} // namespace Game
