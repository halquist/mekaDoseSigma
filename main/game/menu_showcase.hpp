#pragma once

#include "Jet.hpp"
#include "mech_loadout.hpp"
#include "mech_rig.hpp"

namespace Game {

/// Rotating unit previews for the main menu (player + enemies).
class MenuShowcase {
public:
    explicit MenuShowcase(Renderer::Scene& scene);
    ~MenuShowcase();

    void update(float deltaTime);
    void syncPose();

    void suspend();
    void resume();

    bool isActive() const { return m_active; }
    int activeObjects(Renderer::Object** out, int maxCount) const;
    void applyCamera(Renderer::Camera& camera) const;

private:
    enum class Kind : uint8_t {
        PlayerMech,
        EnemyMech,
        Tank,
        AirJet,
        COUNT
    };

    void buildProps();
    void hideAll();
    void showCurrentKind();
    void setKind(Kind kind);
    void advanceKind();
    void updateCameraSmooth(float deltaTime);

    Renderer::Scene& m_scene;
    MechRig m_playerRig;
    MechRig m_enemyRig;
    MechLoadout m_playerLoadout;
    MechLoadout m_enemyLoadout;

    Renderer::Object* m_tankBody = nullptr;
    Renderer::Object* m_tankTurret = nullptr;
    Renderer::Object* m_tankBarrel = nullptr;
    Renderer::Object* m_airJet = nullptr;

    Renderer::Material m_tankBodyMat;
    Renderer::Material m_tankTurretMat;
    Renderer::Material m_tankBarrelMat;
    Renderer::Material m_airJetMat;

    Kind m_kind = Kind::PlayerMech;
    float m_spinAngle = 0.0f;
    float m_time = 0.0f;
    float m_cycleTimer = 0.0f;
    float m_camX = 0.0f;
    float m_camY = 38.0f;
    float m_camZ = 100.0f;
    float m_camLookY = 26.0f;
    bool m_active = true;
    bool m_camInitialized = false;
};

} // namespace Game
