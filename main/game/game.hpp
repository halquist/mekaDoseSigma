#pragma once

#include "Jet.hpp"
#include "types.hpp"
#include "world.hpp"
#include "input.hpp"
#include "mech.hpp"
#include "enemy.hpp"
#include "projectile.hpp"
#include "particles.hpp"
#include "digits.hpp"
#include "map_config.hpp"
#include "obstacles.hpp"

namespace Game {

class MekaGame {
public:
    MekaGame(uint16_t* framebuffer, int width, int height);
    ~MekaGame();

    bool init();
    void update(float deltaTime);
    void render();

private:
    void setupCamera();
    void setupLighting();
    void updateCamera();
    void handleAutoFire();
    void resetMatch();
    void drawEdgeFlash(uint16_t color, int thickness);

    uint16_t* m_framebuffer;
    int m_width;
    int m_height;

    Renderer::Scene* m_scene = nullptr;
    Renderer::Camera* m_camera = nullptr;
    Renderer::DirectionalLight* m_sunLight = nullptr;
    Renderer::AmbientLight* m_ambient = nullptr;

    TouchInput_t* m_input = nullptr;
    World* m_world = nullptr;
    ObstacleField* m_obstacles = nullptr;
    Mech* m_mech = nullptr;
    Enemy* m_enemy = nullptr;
    ProjectileSystem* m_projectiles = nullptr;
    ParticleSystem* m_particles = nullptr;

    MapConfig m_mapConfig;
    uint32_t m_enemySpawnIndex = 0;

    GameState m_state = GameState::PLAYING;
    int m_health = 15;
    int m_maxHealth = 15;

    float m_damageFlash = 0;

    float m_cameraDistance = 118.0f;
    /// Height above mech base (world Y follows terrain hover).
    float m_cameraHeightAboveMech = 34.0f;
    float m_cameraLookAhead = 520.0f;
    /// Look target below mech base; larger = steeper downward view.
    float m_cameraLookDown = 90.0f;
    /// Parallel shift applied to camera and look-at together (same angle).
    /// Increase to push the mech lower on screen.
    float m_cameraRigOffsetY = 55.0f;
};

} // namespace Game
