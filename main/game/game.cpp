#include "game.hpp"
#include "terrain.hpp"
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
    delete m_enemy;
    delete m_mech;
    delete m_obstacles;
    delete m_world;
    delete m_input;
    delete m_sunLight;
    delete m_ambient;
    delete m_camera;
    delete m_scene;
}

bool MekaGame::init() {
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

    Terrain::setMapConfig(&m_mapConfig);

    m_world = new World(*m_scene);
    m_obstacles = new ObstacleField(*m_scene, m_mapConfig);
    m_projectiles = new ProjectileSystem(*m_scene);
    m_particles = new ParticleSystem(*m_scene);
    m_mech = new Mech(*m_scene);
    m_maxHealth = m_mech->getMaxHp();
    m_health = m_maxHealth;
    m_enemy = new Enemy(*m_scene, *m_projectiles, m_mapConfig);
    m_enemySpawnIndex = 0;
    m_enemy->reset(m_mech->getX(), m_mech->getZ(), m_mech->getAngle(), m_enemySpawnIndex);
    m_obstacles->update(m_mech->getX(), m_mech->getZ());
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
    float playerX = m_mech->getX();
    float playerZ = m_mech->getZ();
    float playerAngle = m_mech->getAngle();

    float radians = playerAngle * M_PI / 180.0f;

    float baseY = m_mech->getBaseY();
    float rigY = baseY + m_cameraRigOffsetY;

    float camX = playerX - sinf(radians) * m_cameraDistance;
    float camZ = playerZ - cosf(radians) * m_cameraDistance;
    float camY = rigY + m_cameraHeightAboveMech;

    float lookX = playerX + sinf(radians) * m_cameraLookAhead;
    float lookZ = playerZ + cosf(radians) * m_cameraLookAhead;
    float lookY = rigY - m_cameraLookDown;

    m_camera->setPosition((int16_t)camX, (int16_t)camY, (int16_t)camZ);
    m_camera->lookAt(Vector3{(int16_t)lookX, (int16_t)lookY, (int16_t)lookZ});
}

void MekaGame::handleAutoFire() {
    if (!m_enemy->isAlive()) return;
    m_mech->tryAutoFire(*m_projectiles,
                        m_enemy->getX(), m_enemy->getZ(), m_enemy->getAimY(),
                        m_enemy->isAlive());
}

void MekaGame::resetMatch() {
    m_state = GameState::PLAYING;
    m_maxHealth = m_mech->getMaxHp();
    m_health = m_maxHealth;
    m_damageFlash = 0;
    m_enemySpawnIndex = 0;
    m_mech->reset();
    m_enemy->reset(m_mech->getX(), m_mech->getZ(), m_mech->getAngle(), m_enemySpawnIndex);
    m_obstacles->reset();
    m_obstacles->update(m_mech->getX(), m_mech->getZ());
    m_projectiles->reset();
    m_particles->reset();
    updateCamera();
}

void MekaGame::drawEdgeFlash(uint16_t color, int thickness) {
    for (int t = 0; t < thickness; t++) {
        for (int x = 0; x < m_width; x++) {
            m_framebuffer[t * m_width + x] = color;
            m_framebuffer[(m_height - 1 - t) * m_width + x] = color;
        }
        for (int y = 0; y < m_height; y++) {
            m_framebuffer[y * m_width + t] = color;
            m_framebuffer[y * m_width + (m_width - 1 - t)] = color;
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

        m_enemy->update(deltaTime,
                        m_mech->getX(), m_mech->getZ(), m_mech->getAngle());
        m_world->update(m_mech->getX(), m_mech->getZ());
        m_obstacles->update(m_mech->getX(), m_mech->getZ());
        updateCamera();

        m_projectiles->update(deltaTime);

        if (m_projectiles->checkEnemyHit(m_enemy->getX(),
                                         m_enemy->getZ(),
                                         m_enemy->getAimY(),
                                         m_enemy->getWidth())) {
            m_enemy->takeDamage();
            m_particles->spawnHitEffect(m_enemy->getX(), m_enemy->getZ());
        }

        float impactX = 0.0f;
        float impactZ = 0.0f;
        if (m_projectiles->checkMissileTargetImpact(&impactX, &impactZ)) {
            m_particles->spawnHitEffect(impactX, impactZ);
        }

        if (m_projectiles->checkPlayerHit(m_mech->getX(),
                                            m_mech->getZ(),
                                            m_mech->getWidth())) {
            m_health--;
            m_damageFlash = 0.35f;
            m_particles->spawnHitEffect(m_mech->getX(), m_mech->getZ());
        }

        if (!m_enemy->isAlive()) {
            m_particles->spawnDeathEffect(m_enemy->getX(), m_enemy->getZ());
            m_enemySpawnIndex++;
            m_enemy->reset(m_mech->getX(), m_mech->getZ(), m_mech->getAngle(),
                           m_enemySpawnIndex);
        } else if (m_health <= 0) {
            m_mech->explode();
            m_particles->spawnDeathEffect(m_mech->getX(), m_mech->getZ());
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

    Digits::drawNumber(m_framebuffer, m_width, m_height,
                       displayHealth, m_width / 2, 8, Colors::HUD_TEXT);
    Digits::drawHealthBar(m_framebuffer, m_width, m_height,
                          displayHealth, m_maxHealth,
                          m_width / 2, 18,
                          Colors::HEALTH_FILL, Colors::HUD_BG);

    if (m_damageFlash > 0) {
        drawEdgeFlash(Colors::DAMAGE_RED, 4);
    } else if (m_state == GameState::VICTORY) {
        drawEdgeFlash(Colors::VICTORY_GREEN, 5);
    } else if (m_state == GameState::DEFEAT) {
        drawEdgeFlash(Colors::DAMAGE_RED, 6);
    }
}

} // namespace Game
