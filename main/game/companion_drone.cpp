#include "companion_drone.hpp"
#include "projectile.hpp"
#include "run_upgrades.hpp"
#include "scene_util.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Game {

namespace {

Renderer::Object* createOctahedron(Renderer::Material* mat) {
    auto* body = new Renderer::Object();
    body->cullingMode = Renderer::CullingMode::NO_CULLING;

    auto addV = [&](int32_t x, int32_t y, int32_t z) -> uint16_t {
        Renderer::Object::Vertex v;
        v.position = {x, y, z};
        body->addVertex(v);
        return static_cast<uint16_t>(body->vertices.size() - 1);
    };

    const int32_t r = 4;
    const int32_t h = 5;

    const uint16_t top = addV(0, h, 0);
    const uint16_t bot = addV(0, -h, 0);
    const uint16_t px = addV(r, 0, 0);
    const uint16_t nx = addV(-r, 0, 0);
    const uint16_t pz = addV(0, 0, r);
    const uint16_t nz = addV(0, 0, -r);

    body->addTriangle(top, px, pz, mat);
    body->addTriangle(top, pz, nx, mat);
    body->addTriangle(top, nx, nz, mat);
    body->addTriangle(top, nz, px, mat);
    body->addTriangle(bot, pz, px, mat);
    body->addTriangle(bot, nx, pz, mat);
    body->addTriangle(bot, nz, nx, mat);
    body->addTriangle(bot, px, nz, mat);

    body->calculateBoundingBox();
    return body;
}

} // namespace

CompanionDrone::CompanionDrone(Renderer::Scene& scene)
    : m_scene(scene)
    , m_bodyMat(Colors::MECH_JOINT)
{
    m_bodyMat.shadingMode = Renderer::ShadingMode::UNLIT;
    m_body = createOctahedron(&m_bodyMat);
    m_scene.addObject(m_body);
    hide();
}

void CompanionDrone::hide() {
    m_active = false;
    stashSceneObject(m_body);
}

void CompanionDrone::reset() {
    m_orbitAngle = 0.0f;
    m_fireCooldown = 0.0f;
    hide();
}

void CompanionDrone::syncVisual() {
    showSceneObject(m_body,
                    static_cast<int32_t>(lroundf(m_x)),
                    static_cast<int32_t>(lroundf(m_y)),
                    static_cast<int32_t>(lroundf(m_z)));
}

void CompanionDrone::update(float deltaTime, float mechX, float mechZ, float mechBaseY,
                            bool active) {
    if (m_fireCooldown > 0.0f) {
        m_fireCooldown -= deltaTime;
        if (m_fireCooldown < 0.0f) {
            m_fireCooldown = 0.0f;
        }
    }

    if (!active) {
        if (m_active) {
            hide();
        }
        return;
    }

    m_active = true;
    if (m_fireCooldown <= 0.0f) {
        m_orbitAngle += ORBIT_SPEED * deltaTime;
    }
    m_x = mechX + sinf(m_orbitAngle) * ORBIT_RADIUS;
    m_z = mechZ + cosf(m_orbitAngle) * ORBIT_RADIUS;
    m_y = mechBaseY + ORBIT_HEIGHT;
    syncVisual();
}

bool CompanionDrone::tryFire(ProjectileSystem& projectiles, float targetX, float targetZ,
                             float targetAimY, uint8_t droneTier) {
    if (droneTier == 0 || !m_active || m_fireCooldown > 0.0f) {
        return false;
    }

    const float range = droneRangeForTier(droneTier);
    const float dx = targetX - m_x;
    const float dz = targetZ - m_z;
    const float distSq = dx * dx + dz * dz;
    if (distSq > range * range) {
        return false;
    }

    const int damage = droneDamageForTier(droneTier);
    projectiles.fireDroneLaser(m_x, m_y, m_z, targetX, targetZ, targetAimY, damage);
    m_fireCooldown = droneCooldownForTier(droneTier);
    return true;
}

} // namespace Game
