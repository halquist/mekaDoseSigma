#include "portal.hpp"
#include "scene_util.hpp"
#include "terrain.hpp"
#include "worldgen.hpp"
#include "rng.hpp"
#include "types.hpp"
#include <cmath>

namespace Game {

WorldPortal::WorldPortal(Renderer::Scene& scene, const MapConfig& mapConfig)
    : m_scene(scene)
    , m_mapConfig(mapConfig)
    , m_voidMat(Colors::PORTAL_VOID, 150)
    , m_frameMat(Colors::PORTAL_FRAME)
{
    m_voidMat.shadingMode = Renderer::ShadingMode::GOURAUD;
    m_frameMat.shadingMode = Renderer::ShadingMode::GOURAUD;

    const int32_t voidSize = static_cast<int32_t>(VOID_SIZE);
    const int32_t frameW = static_cast<int32_t>(FRAME_THICKNESS);
    const int32_t frameH = static_cast<int32_t>(PORTAL_HEIGHT - FRAME_THICKNESS);
    const int32_t topW = static_cast<int32_t>(PORTAL_WIDTH);
    const int32_t topH = static_cast<int32_t>(FRAME_THICKNESS);
    const int32_t depth = 6;

    m_voidPanel = Primitives::createCube(voidSize, voidSize, depth, &m_voidMat);
    m_leftFrame = Primitives::createCube(frameW, frameH, depth, &m_frameMat);
    m_rightFrame = Primitives::createCube(frameW, frameH, depth, &m_frameMat);
    m_topFrame = Primitives::createCube(topW, topH, depth, &m_frameMat);

    m_scene.addObject(m_voidPanel);
    m_scene.addObject(m_leftFrame);
    m_scene.addObject(m_rightFrame);
    m_scene.addObject(m_topFrame);
    hide();
}

void WorldPortal::hideVisual() {
    m_visible = false;
    stashSceneObject(m_voidPanel);
    stashSceneObject(m_leftFrame);
    stashSceneObject(m_rightFrame);
    stashSceneObject(m_topFrame);
}

void WorldPortal::hide() {
    m_active = false;
    m_locked = false;
    hideVisual();
}

void WorldPortal::spawn(float playerX, float playerZ, float playerAngle,
                        uint32_t spawnIndex, const ObstacleField* obstacles) {
    const uint32_t spawnSalt = Rng::nextU32() ^ 0xA11CE001u;
    WorldGen::sampleObjectiveSpawn(
        playerX, playerZ, playerAngle, spawnIndex, spawnSalt,
        m_mapConfig, obstacles, m_x, m_z);
    m_active = true;
    m_visible = false;
    m_locked = true;
    m_baseY = Terrain::groundHeight(m_x, m_z);
    hideVisual();
}

void WorldPortal::setLocked(bool locked) {
    if (m_locked == locked) {
        return;
    }
    m_locked = locked;
    if (m_active) {
        syncVisual();
    }
}

void WorldPortal::syncVisual() {
    m_voidMat.alpha = m_locked ? 70 : 150;

    const int16_t ix = static_cast<int16_t>(lroundf(m_x));
    const int16_t iz = static_cast<int16_t>(lroundf(m_z));
    const int16_t voidY =
        static_cast<int16_t>(lroundf(m_baseY + PORTAL_HEIGHT * 0.42f));
    const int16_t frameY =
        static_cast<int16_t>(lroundf(m_baseY + PORTAL_HEIGHT * 0.38f));
    const int16_t topY =
        static_cast<int16_t>(lroundf(m_baseY + PORTAL_HEIGHT * 0.72f));

    const int16_t halfVoid = static_cast<int16_t>(VOID_SIZE * 0.5f);
    const int16_t halfFrame = static_cast<int16_t>(FRAME_THICKNESS * 0.5f);
    const int16_t sideOffset = halfVoid + halfFrame + 1;

    showSceneObject(m_voidPanel, ix, voidY, iz);
    showSceneObject(m_leftFrame, ix - sideOffset, frameY, iz);
    showSceneObject(m_rightFrame, ix + sideOffset, frameY, iz);
    showSceneObject(m_topFrame, ix, topY, iz);
}

void WorldPortal::update(float playerX, float playerZ, float playerAngle) {
    (void)playerX;
    (void)playerZ;
    (void)playerAngle;

    if (!m_active) {
        hide();
        return;
    }

    if (!Terrain::isInsideMesh(m_x, m_z)) {
        hideVisual();
        return;
    }

    m_baseY = Terrain::groundHeight(m_x, m_z);
    m_visible = true;
    syncVisual();
}

float WorldPortal::getAimY() const {
    return m_baseY + PORTAL_HEIGHT * 0.5f;
}

bool WorldPortal::playerTouches(float playerX, float playerZ,
                                float playerWidth) const {
    if (!isUsable()) {
        return false;
    }

    const float dx = playerX - m_x;
    const float dz = playerZ - m_z;
    const float reach = TOUCH_RADIUS + playerWidth * 0.5f;
    return dx * dx + dz * dz <= reach * reach;
}

} // namespace Game
