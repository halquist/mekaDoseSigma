#pragma once

#include "Jet.hpp"
#include "map_config.hpp"

namespace Game {

class ObstacleField;

class WorldPortal {
public:
    WorldPortal(Renderer::Scene& scene, const MapConfig& mapConfig);

    void hide();
    void spawn(float playerX, float playerZ, float playerAngle, uint32_t spawnIndex,
               const ObstacleField* obstacles);

    void update(float playerX, float playerZ, float playerAngle);

    bool isActive() const { return m_active; }
    bool isVisible() const { return m_active && m_visible; }
    bool isLocked() const { return m_locked; }
    bool isUsable() const { return m_active && !m_locked; }

    void setLocked(bool locked);

    float getX() const { return m_x; }
    float getZ() const { return m_z; }
    float getAimY() const;

    bool playerTouches(float playerX, float playerZ, float playerWidth) const;

private:
    void hideVisual();
    void syncVisual();

    Renderer::Scene& m_scene;
    const MapConfig& m_mapConfig;

    Renderer::Object* m_voidPanel = nullptr;
    Renderer::Object* m_leftFrame = nullptr;
    Renderer::Object* m_rightFrame = nullptr;
    Renderer::Object* m_topFrame = nullptr;

    Renderer::Material m_voidMat;
    Renderer::Material m_frameMat;

    float m_x = 0.0f;
    float m_z = 0.0f;
    float m_baseY = 0.0f;

    bool m_active = false;
    bool m_visible = false;
    bool m_locked = false;

    static constexpr float PORTAL_WIDTH = 52.0f;
    static constexpr float PORTAL_HEIGHT = 58.0f;
    static constexpr float VOID_SIZE = 34.0f;
    static constexpr float FRAME_THICKNESS = 9.0f;
    static constexpr float TOUCH_RADIUS = 58.0f;
};

} // namespace Game
