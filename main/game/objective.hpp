#pragma once

#include "Jet.hpp"
#include "map_config.hpp"

namespace Game {

class ObjectiveBuilding {
public:
    ObjectiveBuilding(Renderer::Scene& scene, const MapConfig& mapConfig);

    void reset(float playerX, float playerZ, float playerAngle);
    void respawn(float playerX, float playerZ, float playerAngle);
    void update(float playerX, float playerZ, float playerAngle);

    bool hasTarget() const { return m_hasTarget; }
    bool isVisible() const { return m_hasTarget && m_visible; }
    bool isDestroyed() const { return m_destroyed; }

    float getX() const { return m_x; }
    float getZ() const { return m_z; }
    float getAimY() const;
    float getMissileAimY() const;
    void getHitVerticalRange(float& minY, float& maxY) const;
    float getWidth() const { return BODY_WIDTH; }

    void takeDamage();
    bool isAlive() const { return m_hasTarget && !m_destroyed && m_health > 0; }

private:
    void hide();
    void syncVisual();
    void pickSpawn(float playerX, float playerZ, float playerAngle);
    void spawnNext(float playerX, float playerZ, float playerAngle);

    Renderer::Scene& m_scene;
    const MapConfig& m_mapConfig;

    Renderer::Object* m_body = nullptr;
    Renderer::Object* m_dome = nullptr;
    Renderer::Material m_bodyMat;
    Renderer::Material m_domeMat;

    float m_x = 0.0f;
    float m_z = 0.0f;
    float m_baseY = 0.0f;

    int m_health = 0;
    uint32_t m_spawnIndex = 0;
    bool m_hasTarget = false;
    bool m_visible = false;
    bool m_destroyed = false;

    static constexpr int MAX_HEALTH = 12;
    static constexpr float BODY_WIDTH = 48.0f;
    static constexpr float BODY_DEPTH = 36.0f;
    static constexpr float BODY_HEIGHT = 22.0f;
    static constexpr float DOME_RADIUS = 18.0f;
    static constexpr int DOME_SEGMENTS = 8;
};

} // namespace Game
