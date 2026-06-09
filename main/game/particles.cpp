#include "particles.hpp"
#include "terrain.hpp"
#include <cmath>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Game {

ParticleSystem::ParticleSystem(Renderer::Scene& scene)
    : m_scene(scene)
    , m_cyanMat(Colors::SPARK_ORANGE)
    , m_orangeMat(Colors::SPARK_ORANGE)
    , m_pinkMat(Colors::SPARK_RUST)
{
    m_cyanMat.shadingMode = Renderer::ShadingMode::UNLIT;
    m_orangeMat.shadingMode = Renderer::ShadingMode::UNLIT;
    m_pinkMat.shadingMode = Renderer::ShadingMode::UNLIT;

    for (auto& p : m_particles) {
        p.obj = nullptr;
        p.active = false;
    }
}

void ParticleSystem::spawnParticle(float x, float z, float speed, float life,
                                   Renderer::Material* mat) {
    Particle* slot = nullptr;
    for (auto& p : m_particles) {
        if (!p.active) {
            slot = &p;
            break;
        }
    }
    if (!slot) return;

    if (!slot->obj) {
        slot->obj = Primitives::createCube(5, 5, 5, mat);
        m_scene.addObject(slot->obj);
    } else {
        for (auto& tri : slot->obj->triangles) {
            tri.material = mat;
        }
    }

    float angle = (rand() % 360) * M_PI / 180.0f;
    float elevation = ((rand() % 50) + 35) * M_PI / 180.0f;

    slot->x = x;
    slot->y = Terrain::hoverHeight(x, z);
    slot->z = z;
    slot->vx = cosf(angle) * cosf(elevation) * speed;
    slot->vy = sinf(elevation) * speed;
    slot->vz = sinf(angle) * cosf(elevation) * speed;
    slot->life = life;
    slot->active = true;

    slot->obj->setPosition((int16_t)slot->x, (int16_t)slot->y, (int16_t)slot->z);
}

void ParticleSystem::spawnHitEffect(float x, float z) {
    for (int i = 0; i < 4; i++) {
        spawnParticle(x, z, 70 + (rand() % 30), 0.25f, &m_cyanMat);
    }
}

void ParticleSystem::spawnDeathEffect(float x, float z) {
    for (int i = 0; i < 6; i++) {
        spawnParticle(x, z, 90 + (rand() % 40), 0.5f, &m_orangeMat);
    }
    for (int i = 0; i < 6; i++) {
        spawnParticle(x, z, 90 + (rand() % 40), 0.5f, &m_pinkMat);
    }
}

void ParticleSystem::update(float deltaTime) {
    for (auto& p : m_particles) {
        if (!p.active) continue;

        p.x += p.vx * deltaTime;
        p.y += p.vy * deltaTime;
        p.z += p.vz * deltaTime;
        p.vy -= 180.0f * deltaTime;
        p.life -= deltaTime;

        if (p.life <= 0 || p.y < -10) {
            p.active = false;
            if (p.obj) {
                p.obj->setPosition(0, -1000, 0);
            }
        } else {
            p.obj->setPosition((int16_t)p.x, (int16_t)p.y, (int16_t)p.z);
        }
    }
}

void ParticleSystem::reset() {
    for (auto& p : m_particles) {
        p.active = false;
        if (p.obj) {
            p.obj->setPosition(0, -1000, 0);
        }
    }
}

} // namespace Game
