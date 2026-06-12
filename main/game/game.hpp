#pragma once

#include "Jet.hpp"
#include "types.hpp"
#include "world.hpp"
#include "input.hpp"
#include "mech.hpp"
#include "enemy_manager.hpp"
#include "projectile.hpp"
#include "particles.hpp"
#include "digits.hpp"
#include "map_config.hpp"
#include "obstacles.hpp"
#include "objective.hpp"
#include "objective_hud.hpp"
#include "portal.hpp"
#include "score.hpp"
#include "run_upgrades.hpp"
#include "companion_drone.hpp"
#include "air_strike.hpp"
#include "world_tier.hpp"
#include "high_scores.hpp"
#include "menu_showcase.hpp"
#include "parallel_scene_render.hpp"

namespace Game {

class MekaGame {
public:
    MekaGame(uint16_t* framebuffer, int width, int height);
    ~MekaGame();

    bool init();
    void setRenderTarget(uint16_t* framebuffer);
    void update(float deltaTime);
    void render();

private:
    void setupCamera();
    void setupLighting();
    void applyEnvironment();
    void updateDayNightCycle(float deltaTime);
    void updateCamera();
    bool findAutoFireTarget(float& targetX, float& targetZ, float& targetAimY);
    void handleAutoFire();
    void handleEnemyCombat();
    void handleObjectiveCombat();
    void beginUpgradePick();
    void applyUpgradePick(int choiceIndex);
    void skipUpgradePick();
    void resumeAfterUpgradePick();
    void startNewRun();
    void returnToMenu();
    void applyWorldTier();
    void applyWorldTransition();
    void beginPortalTransition();
    void updatePortalTransition(float deltaTime);
    void handlePortalTransition();
    void onPlayerDefeat();
    void randomizeSession();
    void drawEdgeFlash(uint16_t color, int thickness);
    void drawHudArcs();
    void drawPortalTransitionOverlay();
    void renderMenuScene();
    void resetUiTouchLock();

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
    CompanionDrone* m_drone = nullptr;
    AirStrikeSystem* m_airStrike = nullptr;
    EnemyManager* m_enemies = nullptr;
    ObjectiveBuilding* m_objective = nullptr;
    WorldPortal* m_portal = nullptr;
    ProjectileSystem* m_projectiles = nullptr;
    ParticleSystem* m_particles = nullptr;
    MenuShowcase* m_menuShowcase = nullptr;
    ParallelSceneRenderer m_parallelRenderer;

    MapConfig m_mapConfig;
    WorldTier m_worldTier;
    RunScore m_score;
    UpgradePicker m_upgradePicker;
    int m_highScores[HighScores::kTopCount] = {};
    bool m_newHighScore = false;
    float m_uiPulseSec = 0.0f;
    float m_uiTouchLockSec = 0.0f;
    float m_upgradePickConfirmSec = 0.0f;
    int m_upgradePickChoice = -1;

    GameState m_state = GameState::MENU;
    int m_health = 100;
    int m_maxHealth = 100;

    float m_damageFlash = 0;
    float m_dayNightPhase = 0.0f;

    float m_cameraDistance = 118.0f;
    float m_cameraHeightAboveMech = 34.0f;
    float m_cameraLookAhead = 520.0f;
    float m_cameraLookDown = 90.0f;
    float m_cameraRigOffsetY = 55.0f;

    float m_portalTransitionSec = 0.0f;
    bool m_portalWorldSwapped = false;
    int m_objectivesAtLastPortal = 0;
    bool m_portalBossPending = false;

    static constexpr float kPortalGrowSec = 0.55f;
    static constexpr float kPortalFadeSec = 0.55f;
    static constexpr int kPortalHealAmount = 100;
};

} // namespace Game
