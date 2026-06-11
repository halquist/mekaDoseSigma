#include "menu_showcase.hpp"
#include "mech_catalog.hpp"
#include "scene_util.hpp"
#include "types.hpp"
#include "Jet.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Game {

namespace {

constexpr float kSpinRateDeg = 54.0f;
constexpr float kModelCycleSec = 4.5f;

Renderer::Object* createStealthBomber(Renderer::Material* mat) {
    auto* jet = new Renderer::Object();
    jet->cullingMode = Renderer::CullingMode::NO_CULLING;

    auto addV = [&](int32_t x, int32_t y, int32_t z) -> uint16_t {
        Renderer::Object::Vertex v;
        v.position = {x, y, z};
        jet->addVertex(v);
        return static_cast<uint16_t>(jet->vertices.size() - 1);
    };

    const int32_t yTop = 2;
    const int32_t yBot = 0;

    const uint16_t nT = addV(0, yTop, 26);
    const uint16_t lT = addV(-32, yTop, -18);
    const uint16_t rT = addV(32, yTop, -18);
    const uint16_t tT = addV(0, yTop, -22);

    const uint16_t nB = addV(0, yBot, 26);
    const uint16_t lB = addV(-32, yBot, -18);
    const uint16_t rB = addV(32, yBot, -18);
    const uint16_t tB = addV(0, yBot, -22);

    jet->addTriangle(nT, lT, rT, mat);
    jet->addTriangle(lT, tT, rT, mat);
    jet->addTriangle(nB, rB, lB, mat);
    jet->addTriangle(lB, tB, rB, mat);
    jet->addTriangle(nT, nB, lB, mat);
    jet->addTriangle(nT, lB, lT, mat);
    jet->addTriangle(nT, rT, rB, mat);
    jet->addTriangle(nT, rB, nB, mat);
    jet->addTriangle(lT, lB, tB, mat);
    jet->addTriangle(lT, tB, tT, mat);
    jet->addTriangle(rT, tT, tB, mat);
    jet->addTriangle(rT, tB, rB, mat);

    jet->calculateBoundingBox();
    const int32_t cx = jet->centreVolume.x;
    const int32_t cy = jet->centreVolume.y;
    const int32_t cz = jet->centreVolume.z;
    for (auto& v : jet->vertices) {
        v.position.x -= cx;
        v.position.y -= cy;
        v.position.z -= cz;
    }
    jet->calculateBoundingBox();
    return jet;
}

int32_t yawDegrees(float spinAngle) {
    int32_t yaw = static_cast<int32_t>(floorf(spinAngle));
    yaw %= 360;
    if (yaw < 0) {
        yaw += 360;
    }
    return yaw;
}

int32_t toScreen(int32_t value) {
    return static_cast<int32_t>(floorf(static_cast<float>(value) + 0.5f));
}

} // namespace

MenuShowcase::MenuShowcase(Renderer::Scene& scene)
    : m_scene(scene)
    , m_playerRig(scene)
    , m_enemyRig(scene)
    , m_tankBodyMat(Colors::TANK_BODY)
    , m_tankTurretMat(Colors::TANK_TURRET)
    , m_tankBarrelMat(Colors::TANK_BARREL)
    , m_airJetMat(Colors::AIR_JET)
{
    m_tankBodyMat.shadingMode = Renderer::ShadingMode::GOURAUD;
    m_tankTurretMat.shadingMode = Renderer::ShadingMode::GOURAUD;
    m_tankBarrelMat.shadingMode = Renderer::ShadingMode::GOURAUD;
    m_airJetMat.shadingMode = Renderer::ShadingMode::GOURAUD;

    m_playerLoadout.applyPreset(MechCatalog::LOADOUT_STRIKER_HOVER);
    m_enemyLoadout.applyPreset(MechCatalog::LOADOUT_STRIKER_HOVER);

    m_playerRig.rebuild(m_playerLoadout, MechPalette::Default);
    m_enemyRig.rebuild(m_enemyLoadout, MechPalette::EnemyRed);

    buildProps();
    hideAll();
    showCurrentKind();
    syncPose();
}

MenuShowcase::~MenuShowcase() {
    suspend();
}

void MenuShowcase::buildProps() {
    m_tankBody = Primitives::createCube(36, 14, 28, &m_tankBodyMat);
    m_tankTurret = Primitives::createCube(18, 10, 18, &m_tankTurretMat);
    m_tankBarrel = Primitives::createCube(5, 5, 18, &m_tankBarrelMat);
    m_airJet = createStealthBomber(&m_airJetMat);

    m_scene.addObject(m_tankBody);
    m_scene.addObject(m_tankTurret);
    m_scene.addObject(m_tankBarrel);
    m_scene.addObject(m_airJet);
}

void MenuShowcase::hideAll() {
    m_playerRig.setHidden(true);
    m_enemyRig.setHidden(true);
    stashSceneObject(m_tankBody);
    stashSceneObject(m_tankTurret);
    stashSceneObject(m_tankBarrel);
    stashSceneObject(m_airJet);
}

void MenuShowcase::showCurrentKind() {
    hideAll();
    switch (m_kind) {
    case Kind::PlayerMech:
        m_playerRig.setHidden(false);
        break;
    case Kind::EnemyMech:
        m_enemyRig.setHidden(false);
        break;
    case Kind::Tank:
        showSceneObject(m_tankBody, 0, 0, 0);
        showSceneObject(m_tankTurret, 0, 12, 0);
        showSceneObject(m_tankBarrel, 0, 12, 16);
        break;
    case Kind::AirJet:
        showSceneObject(m_airJet, 0, 88, 0);
        break;
    default:
        break;
    }
}

void MenuShowcase::setKind(Kind kind) {
    m_kind = kind;
    m_spinAngle = 0.0f;
    m_camInitialized = false;
    showCurrentKind();
    syncPose();
}

void MenuShowcase::advanceKind() {
    const auto next =
        static_cast<Kind>((static_cast<uint8_t>(m_kind) + 1) %
                          static_cast<uint8_t>(Kind::COUNT));
    setKind(next);
}

void MenuShowcase::suspend() {
    hideAll();
    m_active = false;
}

void MenuShowcase::resume() {
    m_active = true;
    m_camInitialized = false;
    showCurrentKind();
    syncPose();
}

void MenuShowcase::update(float deltaTime) {
    if (!m_active) {
        return;
    }

    m_time += deltaTime;
    m_cycleTimer += deltaTime;
    if (m_cycleTimer >= kModelCycleSec) {
        m_cycleTimer = 0.0f;
        advanceKind();
    }

    m_spinAngle += kSpinRateDeg * deltaTime;
    syncPose();
    updateCameraSmooth(deltaTime);
}

void MenuShowcase::syncPose() {
    if (!m_active) {
        return;
    }

    const int32_t yaw = yawDegrees(m_spinAngle);

    switch (m_kind) {
    case Kind::PlayerMech:
        if (m_playerRig.bodyObject()) {
            m_playerRig.bodyObject()->setPosition(0, 12, 0);
            m_playerRig.bodyObject()->setRotation(m_playerLoadout.visualPitch(), yaw, 0);
        }
        break;
    case Kind::EnemyMech:
        if (m_enemyRig.bodyObject()) {
            m_enemyRig.bodyObject()->setPosition(0, 12, 0);
            m_enemyRig.bodyObject()->setRotation(m_enemyLoadout.visualPitch(), yaw, 0);
        }
        break;
    case Kind::Tank:
        showSceneObject(m_tankBody, 0, 0, 0);
        m_tankBody->setRotation(0, yaw, 0);
        showSceneObject(m_tankTurret, 0, 12, 0);
        m_tankTurret->setRotation(0, yaw, 0);
        showSceneObject(m_tankBarrel, 0, 12, 16);
        m_tankBarrel->setRotation(0, yaw, 0);
        break;
    case Kind::AirJet:
        showSceneObject(m_airJet, 0, 88, 0);
        m_airJet->setRotation(-12, yaw, 14);
        break;
    default:
        break;
    }
}

void MenuShowcase::updateCameraSmooth(float deltaTime) {
    float lookY = 26.0f;
    float baseDist = 108.0f;
    float camHeight = 36.0f;

    switch (m_kind) {
    case Kind::PlayerMech:
    case Kind::EnemyMech:
        lookY = 28.0f;
        baseDist = 96.0f + sinf(m_time * 0.55f) * 14.0f;
        camHeight = 32.0f + sinf(m_time * 0.85f) * 8.0f;
        break;
    case Kind::Tank:
        lookY = 8.0f;
        baseDist = 110.0f + sinf(m_time * 0.48f) * 16.0f;
        camHeight = 26.0f + sinf(m_time * 0.7f) * 6.0f;
        break;
    case Kind::AirJet:
        lookY = 86.0f;
        baseDist = 116.0f + sinf(m_time * 0.52f) * 12.0f;
        camHeight = 44.0f + sinf(m_time * 0.75f) * 10.0f;
        break;
    default:
        break;
    }

    const float orbit = m_time * 0.38f + sinf(m_time * 0.23f) * 0.35f;
    const float targetX = sinf(orbit) * baseDist;
    const float targetZ = cosf(orbit) * baseDist;
    const float targetY = camHeight + cosf(m_time * 0.61f) * 4.0f;

    if (!m_camInitialized) {
        m_camX = targetX;
        m_camY = targetY;
        m_camZ = targetZ;
        m_camLookY = lookY;
        m_camInitialized = true;
        return;
    }

    const float blend = 1.0f - expf(-10.0f * deltaTime);
    m_camX += (targetX - m_camX) * blend;
    m_camY += (targetY - m_camY) * blend;
    m_camZ += (targetZ - m_camZ) * blend;
    m_camLookY += (lookY - m_camLookY) * blend;
}

int MenuShowcase::activeObjects(Renderer::Object** out, int maxCount) const {
    if (!m_active || !out || maxCount <= 0) {
        return 0;
    }

    int count = 0;
    auto add = [&](Renderer::Object* obj) {
        if (obj && count < maxCount) {
            out[count++] = obj;
        }
    };

    switch (m_kind) {
    case Kind::PlayerMech:
        add(m_playerRig.bodyObject());
        break;
    case Kind::EnemyMech:
        add(m_enemyRig.bodyObject());
        break;
    case Kind::Tank:
        add(m_tankBody);
        add(m_tankTurret);
        add(m_tankBarrel);
        break;
    case Kind::AirJet:
        add(m_airJet);
        break;
    default:
        break;
    }
    return count;
}

void MenuShowcase::applyCamera(Renderer::Camera& camera) const {
    if (!m_active) {
        return;
    }

    camera.setPosition(toScreen(static_cast<int32_t>(m_camX)),
                       toScreen(static_cast<int32_t>(m_camY)),
                       toScreen(static_cast<int32_t>(m_camZ)));
    camera.lookAt(Vector3{0, toScreen(static_cast<int32_t>(m_camLookY)), 0});
}

} // namespace Game
