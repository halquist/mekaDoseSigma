#include "game.hpp"
#include "FramebufferIO.hpp"
#include "terrain.hpp"
#include "rng.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Game {

MekaGame::MekaGame(uint16_t* framebuffer, int width, int height)
    : m_framebuffer(framebuffer)
    , m_width(width)
    , m_height(height)
{
}

MekaGame::~MekaGame() {
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
    Terrain::setMapConfig(&m_mapConfig);
}

bool MekaGame::init() {
    randomizeSession();

    m_camera = new Renderer::Camera();
    setupCamera();

    m_scene = new Renderer::Scene(m_framebuffer, nullptr, m_width, m_height);
    m_scene->setBackcolor(Colors::SKY_BLUE);
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
    updateCamera();

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
    const float weaponRange = weapon ? weapon->range : 350.0f;
    const float maxDistSq = weaponRange * weaponRange;

    float targetX = 0.0f;
    float targetZ = 0.0f;
    float targetAimY = 0.0f;
    bool haveTarget = false;
    float bestDistSq = maxDistSq;

    Enemy* enemyTarget = m_enemies->findClosestAlive(
        m_mech->getX(), m_mech->getZ(), weaponRange);
    if (enemyTarget) {
        const float dx = enemyTarget->getX() - m_mech->getX();
        const float dz = enemyTarget->getZ() - m_mech->getZ();
        bestDistSq = dx * dx + dz * dz;
        targetX = enemyTarget->getX();
        targetZ = enemyTarget->getZ();
        targetAimY = enemyTarget->getMissileAimY();
        haveTarget = true;
    }

    if (m_objective->isAlive() && m_objective->isVisible()) {
        const float dx = m_objective->getX() - m_mech->getX();
        const float dz = m_objective->getZ() - m_mech->getZ();
        const float distSq = dx * dx + dz * dz;
        if (distSq < bestDistSq) {
            bestDistSq = distSq;
            targetX = m_objective->getX();
            targetZ = m_objective->getZ();
            targetAimY = m_objective->getMissileAimY();
            haveTarget = true;
        }
    }

    if (!haveTarget) return;

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
        m_objective->respawn(m_mech->getX(), m_mech->getZ(), m_mech->getAngle());
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
            enemy.takeDamage();
            m_particles->spawnHitEffect(
                enemy.getX(), enemy.getAimY(), enemy.getZ());
            if (!enemy.isAlive()) {
                m_particles->spawnDeathEffect(
                    enemy.getX(), enemy.getAimY(), enemy.getZ());
            }
        }
    }
}

void MekaGame::resetMatch() {
    randomizeSession();
    m_state = GameState::PLAYING;
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
    updateCamera();
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

    if (m_damageFlash > 0) {
        m_damageFlash -= deltaTime;
    }

    m_particles->update(deltaTime);

    if (m_state == GameState::PLAYING) {
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

        const int damage = m_projectiles->checkPlayerHit(m_mech->getX(),
                                                         m_mech->getZ(),
                                                         m_mech->getBaseY(),
                                                         m_mech->getWidth());
        if (damage > 0) {
            m_health -= damage;
            if (m_health < 0) {
                m_health = 0;
            }
            m_damageFlash = 0.35f;
            m_particles->spawnHitEffect(
                m_mech->getX(), m_mech->getBaseY(), m_mech->getZ());
        }

        if (m_health <= 0) {
            m_mech->explode();
            m_particles->spawnDeathEffect(
                m_mech->getX(), m_mech->getBaseY(), m_mech->getZ());
            m_state = GameState::DEFEAT;
        }
    } else {
        m_projectiles->update(deltaTime);
        if (touch.touched) {
            resetMatch();
        }
    }
}

void MekaGame::render() {
    m_scene->render();

    int displayHealth = m_health;
    if (m_state == GameState::DEFEAT) {
        displayHealth = 0;
    }

    Digits::drawHealthArc(m_framebuffer, m_width, m_height,
                          displayHealth, m_maxHealth,
                          Colors::HEALTH_FILL, Colors::HUD_BG);

    if (m_state == GameState::PLAYING &&
        m_objective->hasTarget() && !m_objective->isDestroyed()) {
        ObjectiveHud::drawArrow(m_framebuffer, m_width, m_height, *m_camera,
                                m_mech->getX(), m_mech->getZ(), m_mech->getAngle(),
                                m_objective->getX(), m_objective->getAimY(),
                                m_objective->getZ(), Colors::OBJECTIVE_ARROW);
    }

    if (m_damageFlash > 0) {
        drawEdgeFlash(Colors::DAMAGE_RED, 4);
    } else if (m_state == GameState::VICTORY) {
        drawEdgeFlash(Colors::VICTORY_GREEN, 5);
    } else if (m_state == GameState::DEFEAT) {
        drawEdgeFlash(Colors::DAMAGE_RED, 6);
    }
}

} // namespace Game
