#include "game.hpp"
#include "environment.hpp"
#include "FramebufferIO.hpp"
#include "terrain.hpp"
#include "rng.hpp"
#include "high_scores.hpp"
#include "game_ui.hpp"
#include "font.hpp"
#include "menu_showcase.hpp"
#include "parallel_scene_render.hpp"
#include "run_upgrades.hpp"
#include <cmath>
#include <vector>

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
    delete m_drone;
    delete m_airStrike;
    delete m_enemies;
    delete m_portal;
    delete m_objective;
    delete m_mech;
    delete m_obstacles;
    delete m_world;
    delete m_input;
    delete m_sunLight;
    delete m_ambient;
    delete m_camera;
    m_parallelRenderer.shutdown();
    delete m_scene;
}

void MekaGame::randomizeSession() {
    Rng::init();
    m_worldTier = WorldTier{};
    m_mapConfig.worldIndex = 0;
    m_mapConfig.theme = m_worldTier.themeForWorld();
    m_mapConfig.worldSeed = Rng::nextU32() | 1u;
    m_dayNightPhase = Rng::nextFloat01();
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

    if (!m_parallelRenderer.init()) {
        return false;
    }

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
    m_drone = new CompanionDrone(*m_scene);
    m_airStrike = new AirStrikeSystem(*m_scene);
    m_maxHealth = m_mech->getMaxHp();
    m_health = m_maxHealth;
    m_enemies = new EnemyManager(*m_scene, *m_projectiles, m_mapConfig);
    m_enemies->reset(m_mech->getX(), m_mech->getZ(), m_mech->getAngle(), m_obstacles);
    applyWorldTier();
    m_objective = new ObjectiveBuilding(*m_scene, m_mapConfig);
    m_objective->reset(m_mech->getX(), m_mech->getZ(), m_mech->getAngle(), m_obstacles);
    m_portal = new WorldPortal(*m_scene, m_mapConfig);
    m_portal->hide();
    m_obstacles->update(m_mech->getX(), m_mech->getZ(), m_mech->getAngle());
    m_world->update(m_mech->getX(), m_mech->getZ(), m_cameraLookAhead,
                    0.0f, 0.0f);
    m_menuShowcase = new MenuShowcase(*m_scene);
    updateCamera();
    applyEnvironment();

    m_state = GameState::MENU;
    m_score.reset();
    m_score.setPointValues(m_worldTier.killPointValue(),
                           m_worldTier.objectivePointValue());
    m_uiPulseSec = 0.0f;
    resetUiTouchLock();

    return true;
}

void MekaGame::resetUiTouchLock() {
    m_uiTouchLockSec = 1.0f;
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
    EnvPalette activePalette;
    envPaletteAtCyclePhase(m_mapConfig.theme, m_dayNightPhase, activePalette);

    EnvPalette ruralPalette;
    EnvPalette desertPalette;
    envPaletteAtCyclePhase(MapTheme::RURAL, m_dayNightPhase, ruralPalette);
    envPaletteAtCyclePhase(MapTheme::DESERT, m_dayNightPhase, desertPalette);

    m_scene->setBackcolor(activePalette.sky);
    m_sunLight->color = activePalette.sunColor;
    m_sunLight->intensity = activePalette.sunIntensity;
    m_sunLight->updateDirection(
        Vector3{activePalette.sunAzimuth, activePalette.sunElevation, 0});
    m_ambient->color = activePalette.ambientColor;

    m_world->applyEnvironment(activePalette);
    m_obstacles->applyEnvironment(activePalette, ruralPalette, desertPalette);
}

void MekaGame::updateDayNightCycle(float deltaTime) {
    m_dayNightPhase += deltaTime / kDayNightCycleSec;
    m_dayNightPhase -= floorf(m_dayNightPhase);

    // Only recompute palettes and push to scene/materials when the phase has
    // changed enough to be visually meaningful (~1.5 s at the 300 s cycle).
    static constexpr float kEnvApplyThreshold = 0.005f;
    if (fabsf(m_dayNightPhase - m_lastEnvApplyPhase) >= kEnvApplyThreshold) {
        m_lastEnvApplyPhase = m_dayNightPhase;
        applyEnvironment();
    }
}

void MekaGame::updateCamera(float deltaTime) {
    (void)deltaTime;
    m_cameraPivotX = static_cast<float>(m_mech->getRenderPivotX());
    m_cameraPivotZ = static_cast<float>(m_mech->getRenderPivotZ());
    m_cameraYaw    = static_cast<float>(m_mech->getRenderPivotAngle());
    m_cameraInitialized = true;

    const float radians = m_cameraYaw * static_cast<float>(M_PI) / 180.0f;
    const float sinR = sinf(radians);
    const float cosR = cosf(radians);

    const float rigY = m_mech->getBaseY() + m_cameraRigOffsetY;

    const float camX = m_cameraPivotX - sinR * m_cameraDistance;
    const float camZ = m_cameraPivotZ - cosR * m_cameraDistance;
    const float camY = rigY + m_cameraHeightAboveMech;

    const float lookX = m_cameraPivotX + sinR * m_cameraLookAhead;
    const float lookZ = m_cameraPivotZ + cosR * m_cameraLookAhead;
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
    float targetX = 0.0f;
    float targetZ = 0.0f;
    float targetAimY = 0.0f;
    if (!findAutoFireTarget(targetX, targetZ, targetAimY)) {
        return;
    }

    m_mech->tryAutoFire(*m_projectiles, targetX, targetZ, targetAimY, true);
    m_drone->tryFire(*m_projectiles, targetX, targetZ, targetAimY,
                     m_mech->loadout().bonuses().droneTier);
}

bool MekaGame::findAutoFireTarget(float& targetX, float& targetZ, float& targetAimY) {
    const RunBonuses& bonuses = m_mech->loadout().bonuses();
    const float range = playerAutoFireRange(bonuses);
    const float aimCone = laserAimConeDeg();

    const float mechX = m_mech->getX();
    const float mechZ = m_mech->getZ();
    const float mechAngle = m_mech->getAngle();

    Enemy* enemyTarget = m_enemies->findClosestInArc(
        mechX, mechZ, mechAngle, range, aimCone);
    if (enemyTarget) {
        targetX = enemyTarget->getX();
        targetZ = enemyTarget->getZ();
        targetAimY = enemyTarget->getMissileAimY();
        return true;
    }

    if (m_objective->isAlive() && m_objective->isVisible() &&
        isInWeaponArc(mechX, mechZ, mechAngle,
                      m_objective->getX(), m_objective->getZ(),
                      range, aimCone)) {
        targetX = m_objective->getX();
        targetZ = m_objective->getZ();
        targetAimY = m_objective->getMissileAimY();
        return true;
    }

    return false;
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
    auto tryHitEnemy = [&](Enemy& enemy) {
        if (!enemy.isAlive()) {
            return;
        }

        float enemyMinY = 0.0f;
        float enemyMaxY = 0.0f;
        enemy.getHitVerticalRange(enemyMinY, enemyMaxY);
        int hitDamage = 0;
        if (!m_projectiles->checkEnemyHit(enemy.getX(),
                                         enemy.getZ(),
                                         enemyMinY,
                                         enemyMaxY,
                                         enemy.getWidth(),
                                         &hitDamage)) {
            return;
        }

        const int damage =
            hitDamage > 0 ? hitDamage : m_mech->getWeaponDamage();
        const bool wasAlive = enemy.isAlive();
        bool hitEnemyShield = false;
        enemy.takeDamage(damage, &hitEnemyShield);
        m_particles->spawnHitEffect(
            enemy.getX(), enemy.getAimY(), enemy.getZ(),
            hitEnemyShield ? Colors::SHIELD_ENEMY : Colors::SPARK_ORANGE);
        if (wasAlive && !enemy.isAlive()) {
            m_score.kills++;
            m_particles->spawnDeathEffect(
                enemy.getX(), enemy.getAimY(), enemy.getZ());
            if (enemy.isPortalBoss()) {
                if (m_portal->isActive() && m_portal->isLocked()) {
                    m_portal->setLocked(false);
                }
                m_enemies->onPortalBossDefeated();
            }
        }
    };

    for (int i = 0; i < EnemyManager::MAX_ENEMIES; ++i) {
        tryHitEnemy(m_enemies->enemy(i));
    }

    Enemy* portalBoss = m_enemies->portalBoss();
    if (portalBoss) {
        tryHitEnemy(*portalBoss);
    }
}

void MekaGame::beginUpgradePick() {
    m_upgradePicker.roll(m_mech->loadout());
    m_upgradePickChoice = -1;
    m_upgradePickConfirmSec = 0.0f;
    m_upgradePickAnimSec = 0.0f;
    m_state = GameState::UPGRADE_PICK;
    resetUiTouchLock();
}

void MekaGame::applyWorldTier() {
    m_enemies->applyWorldTier(m_worldTier);
    m_projectiles->setEnemyDamageScale(m_worldTier.enemyDamageScale());
    m_score.setPointValues(m_worldTier.killPointValue(),
                           m_worldTier.objectivePointValue());
}

void MekaGame::applyWorldTransition() {
    m_health += kPortalHealAmount;
    if (m_health > m_maxHealth) {
        m_health = m_maxHealth;
    }

    m_worldTier.index++;
    m_mapConfig.worldIndex = m_worldTier.index;
    m_mapConfig.theme = m_worldTier.nextTheme(m_mapConfig.theme);
    m_mapConfig.worldSeed = Rng::nextU32() | 1u;
    Terrain::setMapConfig(&m_mapConfig);
    applyWorldTier();

    const float px = m_mech->getX();
    const float pz = m_mech->getZ();
    const float angle = m_mech->getAngle();

    m_world->resetAt(px, pz);
    m_obstacles->reset();
    m_obstacles->update(px, pz, angle);
    m_projectiles->reset();
    m_portal->hide();
    m_portalBossPending = false;
    m_objective->respawn(px, pz, angle, m_obstacles);
    applyEnvironment();
    updateCamera();
}

void MekaGame::beginPortalTransition() {
    m_portalTransitionSec = 0.0f;
    m_portalWorldSwapped = false;
    m_projectiles->reset();
    m_enemies->setSpawnPaused(true);
    m_state = GameState::PORTAL_TRANSITION;
}

void MekaGame::updatePortalTransition(float deltaTime) {
    m_portalTransitionSec += deltaTime;

    if (!m_portalWorldSwapped && m_portalTransitionSec >= kPortalGrowSec) {
        applyWorldTransition();
        m_enemies->clearAllEnemies();
        m_portalWorldSwapped = true;
    }

    if (m_portalTransitionSec >= kPortalGrowSec + kPortalFadeSec) {
        m_enemies->setSpawnPaused(false);
        m_enemies->prepareSpawnAfterTransition(
            m_mech->getX(), m_mech->getZ(), m_mech->getAngle());
        m_portalTransitionSec = 0.0f;
        m_portalWorldSwapped = false;
        m_state = GameState::PLAYING;
    }
}

void MekaGame::handlePortalTransition() {
    if (!m_portal->isUsable()) {
        return;
    }
    if (!m_portal->playerTouches(m_mech->getX(), m_mech->getZ(), m_mech->getWidth())) {
        return;
    }
    beginPortalTransition();
}

void MekaGame::drawPortalTransitionOverlay() {
    if (m_state != GameState::PORTAL_TRANSITION) {
        return;
    }

    const float halfW = static_cast<float>(m_width) * 0.5f;
    const float halfH = static_cast<float>(m_height) * 0.5f;
    const float maxRadius = sqrtf(halfW * halfW + halfH * halfH) + 8.0f;
    float radius = 0.0f;
    uint8_t alpha = 255;

    if (m_portalTransitionSec < kPortalGrowSec) {
        const float t = m_portalTransitionSec / kPortalGrowSec;
        const float eased = t * t * (3.0f - 2.0f * t);
        radius = maxRadius * eased;
        alpha = 255;
    } else {
        radius = maxRadius;
        const float fadeT =
            (m_portalTransitionSec - kPortalGrowSec) / kPortalFadeSec;
        const float eased = fadeT * fadeT * (3.0f - 2.0f * fadeT);
        alpha = static_cast<uint8_t>(255.0f * (1.0f - eased));
    }

    GameUi::drawPortalTransitionSphere(
        m_framebuffer, m_width, m_height, radius, alpha);
}

void MekaGame::resumeAfterUpgradePick() {
    m_mech->deployPendingShield();
    const int needed = m_worldTier.objectivesPerPortal();
    const int sincePortal = m_score.objectives - m_objectivesAtLastPortal;
    if (sincePortal >= needed) {
        m_objective->dismissTarget();
        m_portal->spawn(m_mech->getX(), m_mech->getZ(), m_mech->getAngle(),
                        static_cast<uint32_t>(m_score.objectives), m_obstacles);
        m_objectivesAtLastPortal = m_score.objectives;
        m_portalBossPending = true;
    } else {
        m_portal->hide();
        m_portalBossPending = false;
        m_objective->respawn(m_mech->getX(), m_mech->getZ(), m_mech->getAngle(),
                             m_obstacles);
    }
    m_state = GameState::PLAYING;
}

void MekaGame::applyUpgradePick(int choiceIndex) {
    if (choiceIndex < 0 || choiceIndex > 1) {
        return;
    }

    if (m_upgradePicker.isAllMaxed()) {
        m_score.addBonus(kUpgradePickBonusPoints);
    }

    const UpgradeOption& choice = m_upgradePicker.option(choiceIndex);
    const uint8_t oldAirStrikeTier = m_mech->loadout().bonuses().airStrikeTier;
    UpgradePicker::apply(choice, *m_mech, m_health, m_maxHealth);
    if (choice.id == UpgradeId::AirStrike) {
        m_airStrike->onTierEquipped(choice.tier, oldAirStrikeTier);
    }
    m_maxHealth = m_mech->getMaxHp();
    if (m_health > m_maxHealth) {
        m_health = m_maxHealth;
    }
}

void MekaGame::skipUpgradePick() {
    m_score.addBonus(kUpgradePickBonusPoints);
    m_upgradePickChoice = 2;
    m_upgradePickConfirmSec = 1.0f;
}

void MekaGame::prepareNewRun() {
    randomizeSession();
    applyWorldTier();
    m_score.reset();
    m_score.setPointValues(m_worldTier.killPointValue(),
                           m_worldTier.objectivePointValue());
    m_newHighScore = false;
    m_maxHealth = m_mech->getMaxHp();
    m_health = m_maxHealth;
    m_damageFlash = 0;
    m_objectivesAtLastPortal = 0;
    m_portalBossPending = false;
    m_mech->reset();
    m_drone->reset();
    m_airStrike->reset();
    m_enemies->reset(m_mech->getX(), m_mech->getZ(), m_mech->getAngle(), m_obstacles);
    m_objective->reset(m_mech->getX(), m_mech->getZ(), m_mech->getAngle(), m_obstacles);
    m_portal->hide();
    m_obstacles->reset();
    m_obstacles->update(m_mech->getX(), m_mech->getZ(), m_mech->getAngle());
    m_world->resetAt(m_mech->getX(), m_mech->getZ());
    m_world->update(m_mech->getX(), m_mech->getZ(), m_cameraLookAhead,
                    0.0f, 0.0f);
    m_projectiles->reset();
    m_particles->reset();
    m_menuShowcase->suspend();
    updateCamera();
    applyEnvironment();
    m_menuTransitionGameReady = true;
}

void MekaGame::beginMenuTransition() {
    m_menuTransitionSec = 0.0f;
    m_menuTransitionGameReady = false;
    m_state = GameState::MENU_TRANSITION;
    resetUiTouchLock();
}

void MekaGame::startNewRun() {
    prepareNewRun();
    m_state = GameState::PLAYING;
    m_menuTransitionGameReady = false;
}

void MekaGame::updateMenuTransition(float deltaTime, const TouchInput& touch) {
    (void)touch;
    m_menuTransitionSec += deltaTime;

    if (!m_menuTransitionGameReady &&
        m_menuTransitionSec >= GameUi::menuDoorGameplayStartTime()) {
        prepareNewRun();
    }

    if (m_menuTransitionSec >= GameUi::menuDoorGameplayStartTime()) {
        updateGameplay(deltaTime, touch);
    } else if (m_menuShowcase->isActive()) {
        m_menuShowcase->update(deltaTime);
    }

    if (m_menuTransitionSec >= GameUi::menuDoorTransitionDuration()) {
        m_state = GameState::PLAYING;
        m_menuTransitionSec = 0.0f;
        m_menuTransitionGameReady = false;
    }
}

void MekaGame::updateGameplay(float deltaTime, const TouchInput& touch) {
    updateDayNightCycle(deltaTime);

    m_mech->update(touch, deltaTime, m_width, m_height, m_obstacles, m_enemies);
    m_drone->update(deltaTime, m_mech->getX(), m_mech->getZ(), m_mech->getBaseY(),
                    m_mech->loadout().bonuses().droneTier > 0 && m_mech->isAlive());
    handleAutoFire();
    const float airStrikeSkyY = m_mech->getBaseY() + m_cameraRigOffsetY +
                                m_cameraHeightAboveMech + 220.0f;
    m_airStrike->update(deltaTime,
                        m_mech->loadout().bonuses().airStrikeTier,
                        m_mech->getX(), m_mech->getZ(), m_mech->getAngle(),
                        airStrikeSkyY, *m_enemies, *m_particles, m_score, m_portal);

    m_enemies->update(deltaTime,
                      m_mech->getX(), m_mech->getZ(), m_mech->getAngle(),
                      m_mech->getBaseY(), m_obstacles);
    m_objective->update(m_mech->getX(), m_mech->getZ(), m_mech->getAngle());
    m_portal->update(m_mech->getX(), m_mech->getZ(), m_mech->getAngle());
    if (m_portalBossPending && m_portal->isVisible() &&
        !m_enemies->isPortalBossAlive()) {
        m_enemies->spawnPortalBoss(m_portal->getX(), m_portal->getZ(),
                                   m_mech->getAngle(),
                                   m_mech->getX(), m_mech->getZ());
        m_portalBossPending = false;
    }
    handlePortalTransition();
    m_world->update(m_mech->getX(), m_mech->getZ(), m_cameraLookAhead,
                    deltaTime, m_mech->getTurnActivity());
    m_obstacles->update(m_mech->getX(), m_mech->getZ(), m_mech->getAngle());
    updateCamera(deltaTime);

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
        m_mech->ability().onDamageTaken();
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

void MekaGame::returnToMenu() {
    m_state = GameState::MENU;
    m_score.reset();
    m_newHighScore = false;
    m_damageFlash = 0;
    m_objectivesAtLastPortal = 0;
    m_portalBossPending = false;
    m_upgradePickChoice = -1;
    m_upgradePickConfirmSec = 0.0f;
    m_worldTier = WorldTier{};
    m_mapConfig.worldIndex = 0;
    m_mapConfig.theme = m_worldTier.themeForWorld();
    Terrain::setMapConfig(&m_mapConfig);
    applyWorldTier();
    m_mech->reset();
    m_drone->reset();
    m_airStrike->reset();
    m_world->resetAt(m_mech->getX(), m_mech->getZ());
    m_enemies->reset(m_mech->getX(), m_mech->getZ(), m_mech->getAngle(), m_obstacles);
    m_objective->reset(m_mech->getX(), m_mech->getZ(), m_mech->getAngle(), m_obstacles);
    m_portal->hide();
    m_projectiles->reset();
    m_particles->reset();
    m_menuShowcase->resume();
    GameUi::resetMenuStreaks();
    updateCamera();
    applyEnvironment();
    resetUiTouchLock();
}

void MekaGame::onPlayerDefeat() {
    m_newHighScore = HighScores::tryAddScore(m_score.total());
    HighScores::load(m_highScores);
    m_upgradePickChoice = -1;
    m_upgradePickConfirmSec = 0.0f;
    m_state = GameState::DEFEAT;
    resetUiTouchLock();
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
    m_lastDeltaTime = deltaTime;
    m_uiPulseSec += deltaTime;

    if (m_uiTouchLockSec > 0.0f) {
        m_uiTouchLockSec -= deltaTime;
        if (m_uiTouchLockSec < 0.0f) {
            m_uiTouchLockSec = 0.0f;
        }
    }

    if (m_damageFlash > 0) {
        m_damageFlash -= deltaTime;
    }

    m_particles->update(deltaTime);

    const bool uiTouchAllowed = m_uiTouchLockSec <= 0.0f;

    if (m_state == GameState::MENU) {
        if (m_menuShowcase->isActive()) {
            m_menuShowcase->update(deltaTime);
        }
        if (uiTouchAllowed && touch.touched) {
            beginMenuTransition();
        }
        return;
    }

    if (m_state == GameState::MENU_TRANSITION) {
        updateMenuTransition(deltaTime, touch);
        return;
    }

    if (m_state == GameState::UPGRADE_PICK) {
        m_upgradePickAnimSec += deltaTime;

        if (m_upgradePickConfirmSec > 0.0f) {
            m_upgradePickConfirmSec -= deltaTime;
            if (m_upgradePickConfirmSec <= 0.0f) {
                if (m_upgradePickChoice != 2) {
                    applyUpgradePick(m_upgradePickChoice);
                }
                resumeAfterUpgradePick();
                m_upgradePickChoice = -1;
            }
            return;
        }

        if (uiTouchAllowed && touch.touched) {
            const int choice =
                GameUi::upgradePickFromTouch(touch.x, touch.y, m_width, m_height);
            if (choice == 2) {
                skipUpgradePick();
            } else if (choice >= 0) {
                m_upgradePickChoice = choice;
                m_upgradePickConfirmSec = 1.0f;
            }
        }
        return;
    }

    if (m_state == GameState::DEFEAT) {
        m_projectiles->update(deltaTime);
        if (uiTouchAllowed && touch.touched) {
            returnToMenu();
        }
        return;
    }

    if (m_state == GameState::PORTAL_TRANSITION) {
        updateDayNightCycle(deltaTime);
        m_world->update(m_mech->getX(), m_mech->getZ(), m_cameraLookAhead,
                        deltaTime, 0.0f);
        m_obstacles->update(m_mech->getX(), m_mech->getZ(), m_mech->getAngle());
        m_particles->update(deltaTime);
        updatePortalTransition(deltaTime);
        return;
    }

    if (m_state == GameState::PLAYING) {
        updateGameplay(deltaTime, touch);
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

    // Save and filter every scene object — no cap. Pre-allocated projectile
    // meshes pushed the object count well past the old 160 limit, which left
    // gameplay mech parts enabled during menu rendering.
    std::vector<SavedState> saved;
    saved.reserve(objects.size());
    for (Renderer::Object* obj : objects) {
        bool keep = false;
        for (int i = 0; i < activeCount; ++i) {
            if (obj == activeList[i]) {
                keep = true;
                break;
            }
        }

        saved.push_back({obj, obj->enabled});
        obj->enabled = keep;
    }

    m_scene->setBackcolor(Colors::BLACK);
    m_menuShowcase->applyCamera(*m_camera);
    m_parallelRenderer.render(*m_scene, m_height);

    GameUi::drawMenuStreaks(m_framebuffer, m_width, m_height, m_lastDeltaTime);

    for (const SavedState& entry : saved) {
        entry.obj->enabled = entry.enabled;
    }
}

void MekaGame::drawHudArcs() {
    const MechAbility& ability = m_mech->ability();
    const int maxShieldHp = shieldCapacityForTier(m_mech->loadout().bonuses().shieldTier);
    const bool shieldActive = ability.showShieldBar();

    bool showShieldCapacityRing = shieldActive;
    if (m_state == GameState::UPGRADE_PICK
        && m_upgradePickConfirmSec > 0.0f
        && m_upgradePickChoice >= 0) {
        const UpgradeOption& choice = m_upgradePicker.option(m_upgradePickChoice);
        if (choice.id == UpgradeId::Shield) {
            showShieldCapacityRing = true;
        }
    }

    Digits::drawHealthAndShieldArcs(
        m_framebuffer, m_width, m_height,
        m_health, m_maxHealth, kHpCapMk6,
        Colors::HEALTH_FILL, Colors::HUD_BG,
        showShieldCapacityRing, maxShieldHp, kShieldCapMk6,
        shieldActive, ability.shieldHp(),
        Colors::SHIELD_HUD);
}

void MekaGame::render() {
    if (m_state == GameState::MENU_TRANSITION) {
        const GameUi::MenuDoorAnim doorAnim =
            GameUi::menuDoorAnimForTime(m_menuTransitionSec);
        if (!doorAnim.showGameplay) {
            renderMenuScene();
            GameUi::drawMenu(m_framebuffer, m_width, m_height, m_highScores,
                             m_uiPulseSec);
        } else {
            m_parallelRenderer.render(*m_scene, m_height);
            drawHudArcs();
        }
        GameUi::drawMenuDoors(m_framebuffer, m_width, m_height, doorAnim);
        return;
    }

    const bool fullScreenUi =
        m_state == GameState::MENU ||
        m_state == GameState::UPGRADE_PICK ||
        m_state == GameState::DEFEAT;

    if (!fullScreenUi) {
        m_parallelRenderer.render(*m_scene, m_height);

        if (m_state == GameState::PORTAL_TRANSITION) {
            drawPortalTransitionOverlay();
            return;
        }

        drawHudArcs();

        const MechAbility* ability = nullptr;
        if (m_state == GameState::PLAYING && m_mech->isAlive()) {
            ability = &m_mech->ability();
        }

        if (ability != nullptr && ability->showReadyIcon()) {
            Digits::drawAbilityReadyIcon(m_framebuffer, m_width, m_height,
                                         Colors::SHIELD_HUD);
        }

        if (m_state == GameState::PLAYING && m_portal->isActive()) {
            ObjectiveHud::drawArrow(m_framebuffer, m_width, m_height, *m_camera,
                                    m_mech->getX(), m_mech->getZ(), m_mech->getAngle(),
                                    m_portal->getX(), m_portal->getAimY(),
                                    m_portal->getZ(), Colors::PORTAL_ARROW);
        } else if (m_state == GameState::PLAYING &&
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
        m_parallelRenderer.render(*m_scene, m_height);
        const GameUi::UpgradePickAnim upgradeAnim =
            GameUi::upgradePickAnimForTime(m_upgradePickAnimSec,
                                          m_upgradePickConfirmSec,
                                          m_upgradePickChoice);
        GameUi::drawUpgradePick(m_framebuffer, m_width, m_height,
                                m_upgradePicker.option(0),
                                m_upgradePicker.option(1),
                                m_upgradePickChoice,
                                m_score.total(),
                                upgradeAnim);
        drawHudArcs();
    }
}

} // namespace Game
