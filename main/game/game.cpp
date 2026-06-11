#include "game.hpp"
#include "environment.hpp"
#include "FramebufferIO.hpp"
#include "terrain.hpp"
#include "rng.hpp"
#include "high_scores.hpp"
#include "game_ui.hpp"
#include "font.hpp"
#include "menu_showcase.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Game {

namespace {

bool isInWeaponArc(float fromX, float fromZ, float fromAngleDeg,
                   float targetX, float targetZ,
                   float maxRange, float aimConeDeg) {
    const float dx = targetX - fromX;
    const float dz = targetZ - fromZ;
    const float distSq = dx * dx + dz * dz;
    if (distSq > maxRange * maxRange) {
        return false;
    }

    float angleToTarget = atan2f(dx, dz) * 180.0f / static_cast<float>(M_PI);
    float angleDiff = angleToTarget - fromAngleDeg;
    while (angleDiff > 180.0f) angleDiff -= 360.0f;
    while (angleDiff < -180.0f) angleDiff += 360.0f;
    return fabsf(angleDiff) <= aimConeDeg;
}

} // namespace

MekaGame::MekaGame(uint16_t* framebuffer, int width, int height)
    : m_framebuffer(framebuffer)
    , m_width(width)
    , m_height(height)
{
}

void MekaGame::setRenderTarget(uint16_t* framebuffer) {
    m_framebuffer = framebuffer;
    if (m_scene) {
        m_scene->setFramebuffer(framebuffer);
    }
}

MekaGame::~MekaGame() {
    delete m_menuShowcase;
    delete m_particles;
    delete m_projectiles;
    delete m_enemies;
    delete m_objective;
    delete m_mech;
    delete m_obstacles;
    delete m_world;
    delete m_input;
    delete m_sunLight;
    delete m_ambient;
    delete m_camera;
    delete m_scene;
}

void MekaGame::randomizeSession() {
    Rng::init();
    m_mapConfig.worldSeed = Rng::nextU32() | 1u;
    m_mapConfig.theme = Rng::coinFlip() ? MapTheme::RURAL : MapTheme::DESERT;
    m_mapConfig.lighting =
        static_cast<EnvLightingMode>(Rng::nextRange(3));
    Terrain::setMapConfig(&m_mapConfig);
}

bool MekaGame::init() {
    if (!HighScores::init()) {
        return false;
    }
    HighScores::load(m_highScores);

    randomizeSession();

    m_camera = new Renderer::Camera();
    setupCamera();

    m_scene = new Renderer::Scene(m_framebuffer, nullptr, m_width, m_height);
    m_scene->setCamera(m_camera);

    setupLighting();

    m_input = new TouchInput_t();
    if (!m_input->init()) {
        return false;
    }

    m_world = new World(*m_scene);
    m_obstacles = new ObstacleField(*m_scene, m_mapConfig);
    m_projectiles = new ProjectileSystem(*m_scene);
    m_particles = new ParticleSystem(*m_scene);
    m_mech = new Mech(*m_scene);
    m_maxHealth = m_mech->getMaxHp();
    m_health = m_maxHealth;
    m_enemies = new EnemyManager(*m_scene, *m_projectiles, m_mapConfig);
    m_enemies->reset(m_mech->getX(), m_mech->getZ(), m_mech->getAngle());
    m_objective = new ObjectiveBuilding(*m_scene, m_mapConfig);
    m_objective->reset(m_mech->getX(), m_mech->getZ(), m_mech->getAngle());
    m_obstacles->update(m_mech->getX(), m_mech->getZ(), m_mech->getAngle());
    m_world->update(m_mech->getX(), m_mech->getZ(), m_cameraLookAhead,
                    0.0f, 0.0f);
    m_menuShowcase = new MenuShowcase(*m_scene);
    updateCamera();
    applyEnvironment();

    m_state = GameState::MENU;
    m_score.reset();
    m_uiPulseSec = 0.0f;

    return true;
}

void MekaGame::setupCamera() {
    m_camera->setFOV(static_cast<int32_t>(60), static_cast<int32_t>(m_width));
    m_camera->nearPlane = 10;
    m_camera->farPlane = 1500;
}

void MekaGame::setupLighting() {
    m_sunLight = new Renderer::DirectionalLight(
        Vector3{20, -30, 0},
        Color{255, 240, 220},
        140
    );
    m_ambient = new Renderer::AmbientLight(Color{100, 120, 90});
    m_scene->setDirectionalLight(m_sunLight);
    m_scene->setAmbientLight(m_ambient);
}

void MekaGame::applyEnvironment() {
    const EnvPalette& palette =
        envPaletteFor(m_mapConfig.theme, m_mapConfig.lighting);

    m_scene->setBackcolor(palette.sky);
    m_sunLight->color = palette.sunColor;
    m_sunLight->intensity = palette.sunIntensity;
    m_sunLight->updateDirection(
        Vector3{palette.sunAzimuth, palette.sunElevation, 0});
    m_ambient->color = palette.ambientColor;

    m_world->applyEnvironment(palette);
    m_obstacles->applyEnvironment(palette);
}

void MekaGame::updateCamera() {
    const int32_t pivotX = m_mech->getRenderPivotX();
    const int32_t pivotZ = m_mech->getRenderPivotZ();
    const int32_t pivotAngle = m_mech->getRenderPivotAngle();

    const float radians = static_cast<float>(pivotAngle) * M_PI / 180.0f;
    const float sinR = sinf(radians);
    const float cosR = cosf(radians);

    const float rigY = m_mech->getBaseY() + m_cameraRigOffsetY;

    const float camX = static_cast<float>(pivotX) - sinR * m_cameraDistance;
    const float camZ = static_cast<float>(pivotZ) - cosR * m_cameraDistance;
    const float camY = rigY + m_cameraHeightAboveMech;

    const float lookX = static_cast<float>(pivotX) + sinR * m_cameraLookAhead;
    const float lookZ = static_cast<float>(pivotZ) + cosR * m_cameraLookAhead;
    const float lookY = rigY - m_cameraLookDown;

    m_camera->setPosition(static_cast<int32_t>(lroundf(camX)),
                          static_cast<int32_t>(lroundf(camY)),
                          static_cast<int32_t>(lroundf(camZ)));
    m_camera->lookAt(Vector3{
        static_cast<int32_t>(lroundf(lookX)),
        static_cast<int32_t>(lroundf(lookY)),
        static_cast<int32_t>(lroundf(lookZ))});
}

void MekaGame::handleAutoFire() {
    const WeaponDef* weapon =
        m_mech->loadout().weapon(m_mech->loadout().activeWeaponSlot());
    if (!weapon) {
        return;
    }

    const float mechX = m_mech->getX();
    const float mechZ = m_mech->getZ();
    const float mechAngle = m_mech->getAngle();

    float targetX = 0.0f;
    float targetZ = 0.0f;
    float targetAimY = 0.0f;
    bool haveTarget = false;

    Enemy* enemyTarget = m_enemies->findClosestInArc(
        mechX, mechZ, mechAngle, weapon->range, weapon->aimConeDeg);
    if (enemyTarget) {
        targetX = enemyTarget->getX();
        targetZ = enemyTarget->getZ();
        targetAimY = enemyTarget->getMissileAimY();
        haveTarget = true;
    } else if (m_objective->isAlive() && m_objective->isVisible() &&
               isInWeaponArc(mechX, mechZ, mechAngle,
                             m_objective->getX(), m_objective->getZ(),
                             weapon->range, weapon->aimConeDeg)) {
        targetX = m_objective->getX();
        targetZ = m_objective->getZ();
        targetAimY = m_objective->getMissileAimY();
        haveTarget = true;
    }

    if (!haveTarget) {
        return;
    }

    m_mech->tryAutoFire(*m_projectiles, targetX, targetZ, targetAimY, true);
}

void MekaGame::handleObjectiveCombat() {
    if (!m_objective->isAlive() || !m_objective->isVisible()) return;

    float minY = 0.0f;
    float maxY = 0.0f;
    m_objective->getHitVerticalRange(minY, maxY);
    if (!m_projectiles->checkEnemyHit(m_objective->getX(),
                                      m_objective->getZ(),
                                      minY,
                                      maxY,
                                      m_objective->getWidth())) {
        return;
    }

    m_objective->takeDamage();
    m_particles->spawnHitEffect(
        m_objective->getX(), m_objective->getAimY(), m_objective->getZ());
    if (m_objective->isDestroyed()) {
        m_particles->spawnDeathEffect(
            m_objective->getX(), m_objective->getAimY(), m_objective->getZ());
        m_score.objectives++;
        beginUpgradePick();
    }
}

void MekaGame::handleEnemyCombat() {
    for (int i = 0; i < EnemyManager::MAX_ENEMIES; ++i) {
        Enemy& enemy = m_enemies->enemy(i);
        if (!enemy.isAlive()) continue;

        float enemyMinY = 0.0f;
        float enemyMaxY = 0.0f;
        enemy.getHitVerticalRange(enemyMinY, enemyMaxY);
        if (m_projectiles->checkEnemyHit(enemy.getX(),
                                         enemy.getZ(),
                                         enemyMinY,
                                         enemyMaxY,
                                         enemy.getWidth())) {
            const bool wasAlive = enemy.isAlive();
            bool hitEnemyShield = false;
            enemy.takeDamage(m_mech->getWeaponDamage(), &hitEnemyShield);
            m_particles->spawnHitEffect(
                enemy.getX(), enemy.getAimY(), enemy.getZ(),
                hitEnemyShield ? Colors::SHIELD_ENEMY : Colors::SPARK_ORANGE);
            if (wasAlive && !enemy.isAlive()) {
                m_score.kills++;
                m_particles->spawnDeathEffect(
                    enemy.getX(), enemy.getAimY(), enemy.getZ());
            }
        }
    }
}

void MekaGame::beginUpgradePick() {
    m_upgradePicker.roll(m_mech->loadout());
    m_state = GameState::UPGRADE_PICK;
}

void MekaGame::finishUpgradePick(int choiceIndex) {
    if (choiceIndex < 0 || choiceIndex > 1) {
        return;
    }

    UpgradePicker::apply(m_upgradePicker.option(choiceIndex).id,
                         *m_mech, m_health, m_maxHealth);
    m_maxHealth = m_mech->getMaxHp();
    if (m_health > m_maxHealth) {
        m_health = m_maxHealth;
    }

    m_objective->respawn(m_mech->getX(), m_mech->getZ(), m_mech->getAngle());
    m_state = GameState::PLAYING;
}

void MekaGame::startNewRun() {
    randomizeSession();
    m_state = GameState::PLAYING;
    m_score.reset();
    m_newHighScore = false;
    m_maxHealth = m_mech->getMaxHp();
    m_health = m_maxHealth;
    m_damageFlash = 0;
    m_mech->reset();
    m_enemies->reset(m_mech->getX(), m_mech->getZ(), m_mech->getAngle());
    m_objective->reset(m_mech->getX(), m_mech->getZ(), m_mech->getAngle());
    m_obstacles->reset();
    m_obstacles->update(m_mech->getX(), m_mech->getZ(), m_mech->getAngle());
    m_world->update(m_mech->getX(), m_mech->getZ(), m_cameraLookAhead,
                    0.0f, 0.0f);
    m_projectiles->reset();
    m_particles->reset();
    m_menuShowcase->suspend();
    updateCamera();
    applyEnvironment();
}

void MekaGame::returnToMenu() {
    m_state = GameState::MENU;
    m_score.reset();
    m_newHighScore = false;
    m_damageFlash = 0;
    m_mech->reset();
    m_enemies->reset(m_mech->getX(), m_mech->getZ(), m_mech->getAngle());
    m_projectiles->reset();
    m_particles->reset();
    m_menuShowcase->resume();
    updateCamera();
}

void MekaGame::onPlayerDefeat() {
    m_newHighScore = HighScores::tryAddScore(m_score.total());
    HighScores::load(m_highScores);
    m_state = GameState::DEFEAT;
}

void MekaGame::drawEdgeFlash(uint16_t color, int thickness) {
    const uint16_t packed = fbPack(color);
    for (int t = 0; t < thickness; t++) {
        for (int x = 0; x < m_width; x++) {
            m_framebuffer[t * m_width + x] = packed;
            m_framebuffer[(m_height - 1 - t) * m_width + x] = packed;
        }
        for (int y = 0; y < m_height; y++) {
            m_framebuffer[y * m_width + t] = packed;
            m_framebuffer[y * m_width + (m_width - 1 - t)] = packed;
        }
    }
}

void MekaGame::update(float deltaTime) {
    TouchInput touch = m_input->read();
    m_uiPulseSec += deltaTime;

    if (m_damageFlash > 0) {
        m_damageFlash -= deltaTime;
    }

    m_particles->update(deltaTime);

    if (m_state == GameState::MENU) {
        if (m_menuShowcase->isActive()) {
            m_menuShowcase->update(deltaTime);
        }
        if (touch.touched) {
            startNewRun();
        }
        return;
    }

    if (m_state == GameState::UPGRADE_PICK) {
        if (touch.touched) {
            const int choice =
                GameUi::upgradePickFromTouch(touch.x, touch.y, m_width, m_height);
            if (choice >= 0) {
                finishUpgradePick(choice);
            }
        }
        return;
    }

    if (m_state == GameState::DEFEAT) {
        m_projectiles->update(deltaTime);
        if (touch.touched) {
            returnToMenu();
        }
        return;
    }

    if (m_state == GameState::PLAYING) {
        m_score.timeSec += deltaTime;

        m_mech->update(touch, deltaTime, m_width, m_obstacles);
        handleAutoFire();

        m_enemies->update(deltaTime,
                          m_mech->getX(), m_mech->getZ(), m_mech->getAngle(),
                          m_mech->getBaseY());
        m_objective->update(m_mech->getX(), m_mech->getZ(), m_mech->getAngle());
        m_world->update(m_mech->getX(), m_mech->getZ(), m_cameraLookAhead,
                        deltaTime, m_mech->getTurnActivity());
        m_obstacles->update(m_mech->getX(), m_mech->getZ(), m_mech->getAngle());
        updateCamera();

        m_projectiles->update(deltaTime);

        handleEnemyCombat();
        handleObjectiveCombat();

        float impactX = 0.0f;
        float impactZ = 0.0f;
        float impactY = 0.0f;
        if (m_projectiles->checkMissileTargetImpact(&impactX, &impactZ, &impactY)) {
            m_particles->spawnHitEffect(impactX, impactY, impactZ);
        }

        float hitX = 0.0f;
        float hitY = 0.0f;
        float hitZ = 0.0f;
        const int rawDamage = m_projectiles->checkPlayerHit(m_mech->getX(),
                                                         m_mech->getZ(),
                                                         m_mech->getBaseY(),
                                                         m_mech->getWidth(),
                                                         &hitX, &hitZ, &hitY);
        if (rawDamage > 0) {
            const ShieldDamageResult absorbed = m_mech->absorbDamage(rawDamage);
            if (absorbed.healthDamage > 0) {
                m_health -= absorbed.healthDamage;
                if (m_health < 0) {
                    m_health = 0;
                }
                m_damageFlash = 0.35f;
                m_particles->spawnHitEffect(
                    m_mech->getX(), m_mech->getBaseY(), m_mech->getZ(),
                    Colors::SPARK_ORANGE);
            } else if (absorbed.hitShield) {
                float fx = hitX;
                float fy = hitY;
                float fz = hitZ;
                MechAbility::shieldHitPosition(
                    m_mech->getX(), m_mech->getBaseY(), m_mech->getZ(),
                    hitX, hitY, hitZ, fx, fy, fz);
                m_particles->spawnHitEffect(fx, fy, fz, Colors::SHIELD_BLUE);
            }
        }

        if (m_health <= 0) {
            m_mech->explode();
            m_particles->spawnDeathEffect(
                m_mech->getX(), m_mech->getBaseY(), m_mech->getZ());
            onPlayerDefeat();
        }
    }
}

void MekaGame::renderMenuScene() {
    if (!m_menuShowcase || !m_menuShowcase->isActive()) {
        GameUi::fillScreen(m_framebuffer, m_width, m_height, Colors::BLACK);
        return;
    }

    struct SavedState {
        Renderer::Object* obj = nullptr;
        bool enabled = false;
    };

    Renderer::Object* activeList[8];
    const int activeCount = m_menuShowcase->activeObjects(activeList, 8);
    auto& objects = m_scene->getObjects();

    SavedState saved[160];
    int savedCount = 0;
    for (Renderer::Object* obj : objects) {
        if (savedCount >= 160) {
            break;
        }

        bool keep = false;
        for (int i = 0; i < activeCount; ++i) {
            if (obj == activeList[i]) {
                keep = true;
                break;
            }
        }

        saved[savedCount].obj = obj;
        saved[savedCount].enabled = obj->enabled;
        obj->enabled = keep;
        ++savedCount;
    }

    m_scene->setBackcolor(Colors::BLACK);
    m_menuShowcase->applyCamera(*m_camera);
    m_scene->render();

    for (int i = 0; i < savedCount; ++i) {
        saved[i].obj->enabled = saved[i].enabled;
    }
}

void MekaGame::render() {
    const bool fullScreenUi =
        m_state == GameState::MENU ||
        m_state == GameState::UPGRADE_PICK ||
        m_state == GameState::DEFEAT;

    if (!fullScreenUi) {
        m_scene->render();

        int displayHealth = m_health;

        const MechAbility* ability = nullptr;
        if (m_state == GameState::PLAYING && m_mech->isAlive()) {
            ability = &m_mech->ability();
        }

        Digits::drawHealthAndShieldArcs(
            m_framebuffer, m_width, m_height,
            displayHealth, m_maxHealth,
            Colors::HEALTH_FILL, Colors::HUD_BG,
            ability != nullptr && ability->showShieldBar(),
            ability != nullptr ? ability->shieldHp() : 0,
            ability != nullptr ? ability->maxShieldHp() : 1,
            Colors::SHIELD_HUD);

        if (ability != nullptr && ability->showReadyIcon()) {
            Digits::drawAbilityReadyIcon(m_framebuffer, m_width, m_height,
                                         Colors::SHIELD_HUD);
        }

        if (m_state == GameState::PLAYING &&
            m_objective->hasTarget() && !m_objective->isDestroyed()) {
            ObjectiveHud::drawArrow(m_framebuffer, m_width, m_height, *m_camera,
                                    m_mech->getX(), m_mech->getZ(), m_mech->getAngle(),
                                    m_objective->getX(), m_objective->getAimY(),
                                    m_objective->getZ(), Colors::OBJECTIVE_ARROW);
        }

        if (m_state == GameState::PLAYING) {
            Font::drawNumberRight(m_framebuffer, m_width, m_height, m_score.total(),
                                  m_width - 12, 30, 2, Colors::HUD_TEXT);
        }

        if (m_damageFlash > 0) {
            drawEdgeFlash(Colors::DAMAGE_RED, 4);
        } else if (m_state == GameState::VICTORY) {
            drawEdgeFlash(Colors::VICTORY_GREEN, 5);
        }
        return;
    }

    if (m_state == GameState::MENU) {
        renderMenuScene();
        GameUi::drawMenu(m_framebuffer, m_width, m_height, m_highScores, m_uiPulseSec);
    } else if (m_state == GameState::DEFEAT) {
        GameUi::drawDefeat(m_framebuffer, m_width, m_height,
                           m_score, m_highScores, m_newHighScore);
    } else if (m_state == GameState::UPGRADE_PICK) {
        GameUi::drawUpgradePick(m_framebuffer, m_width, m_height,
                                m_upgradePicker.option(0),
                                m_upgradePicker.option(1));
    }
}

} // namespace Game
