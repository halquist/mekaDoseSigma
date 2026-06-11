#include "objective.hpp"
#include "scene_util.hpp"
#include "terrain.hpp"
#include "worldgen.hpp"
#include "rng.hpp"
#include "types.hpp"
#include <cmath>

namespace Game {

ObjectiveBuilding::ObjectiveBuilding(Renderer::Scene& scene, const MapConfig& mapConfig)
    : m_scene(scene)
    , m_mapConfig(mapConfig)
    , m_bodyMat(Colors::OBJECTIVE_BODY)
    , m_domeMat(Colors::OBJECTIVE_DOME)
{
    m_bodyMat.shadingMode = Renderer::ShadingMode::GOURAUD;
    m_domeMat.shadingMode = Renderer::ShadingMode::GOURAUD;

    m_body = Primitives::createCube(
        static_cast<int32_t>(BODY_WIDTH),
        static_cast<int32_t>(BODY_HEIGHT),
        static_cast<int32_t>(BODY_DEPTH),
        &m_bodyMat);
    m_dome = Primitives::createSphere(
        static_cast<int32_t>(DOME_RADIUS), DOME_SEGMENTS, &m_domeMat);

    m_scene.addObject(m_body);
    m_scene.addObject(m_dome);
    hide();
}

void ObjectiveBuilding::hide() {
    stashSceneObject(m_body);
    stashSceneObject(m_dome);
}

void ObjectiveBuilding::pickSpawn(float playerX, float playerZ, float playerAngle) {
    const uint32_t spawnSalt = Rng::nextU32();
    WorldGen::sampleObjectiveSpawn(
        playerX, playerZ, playerAngle, m_spawnIndex, spawnSalt,
        m_mapConfig, m_x, m_z);
}

void ObjectiveBuilding::spawnNext(float playerX, float playerZ, float playerAngle) {
    m_spawnIndex++;
    m_destroyed = false;
    m_visible = false;
    m_health = MAX_HEALTH;
    m_hasTarget = true;
    pickSpawn(playerX, playerZ, playerAngle);
    hide();
}

void ObjectiveBuilding::respawn(float playerX, float playerZ, float playerAngle) {
    spawnNext(playerX, playerZ, playerAngle);
}

void ObjectiveBuilding::dismissTarget() {
    m_hasTarget = false;
    m_destroyed = false;
    m_visible = false;
    hide();
}

void ObjectiveBuilding::reset(float playerX, float playerZ, float playerAngle) {
    m_spawnIndex = 0;
    spawnNext(playerX, playerZ, playerAngle);
}

void ObjectiveBuilding::syncVisual() {
    const int16_t ix = static_cast<int16_t>(lroundf(m_x));
    const int16_t iz = static_cast<int16_t>(lroundf(m_z));
    const int16_t bodyY = static_cast<int16_t>(lroundf(m_baseY));
    const int16_t domeY =
        static_cast<int16_t>(lroundf(m_baseY + BODY_HEIGHT + DOME_RADIUS - 4.0f));

    showSceneObject(m_body, ix, bodyY, iz);
    m_body->setRotation(0, 0, 0);
    showSceneObject(m_dome, ix, domeY, iz);
    m_dome->setRotation(0, 0, 0);
}

void ObjectiveBuilding::update(float playerX, float playerZ, float playerAngle) {
    (void)playerX;
    (void)playerZ;
    (void)playerAngle;

    if (!m_hasTarget || m_destroyed) {
        hide();
        return;
    }

    if (!Terrain::isInsideMesh(m_x, m_z)) {
        m_visible = false;
        hide();
        return;
    }

    m_baseY = Terrain::groundHeight(m_x, m_z);
    m_visible = true;
    syncVisual();
}

float ObjectiveBuilding::getAimY() const {
    if (!m_hasTarget) return 0.0f;
    const float base = m_visible ? m_baseY : Terrain::groundHeight(m_x, m_z);
    return base + BODY_HEIGHT * 0.55f + DOME_RADIUS * 0.35f;
}

float ObjectiveBuilding::getMissileAimY() const {
    if (!m_hasTarget) return getAimY();
    const float base = m_visible ? m_baseY : Terrain::groundHeight(m_x, m_z);
    const float minY = base + BODY_HEIGHT * 0.25f;
    const float maxY = base + BODY_HEIGHT + DOME_RADIUS * 0.6f;
    return minY + Rng::nextFloat01() * (maxY - minY);
}

void ObjectiveBuilding::getHitVerticalRange(float& minY, float& maxY) const {
    const float base = m_visible ? m_baseY : Terrain::groundHeight(m_x, m_z);
    minY = base;
    maxY = base + BODY_HEIGHT + DOME_RADIUS * 1.2f;
}

void ObjectiveBuilding::takeDamage() {
    if (!isAlive() || !m_visible) return;
    m_health--;
    if (m_health <= 0) {
        m_destroyed = true;
        hide();
    }
}

} // namespace Game
