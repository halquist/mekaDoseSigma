#pragma once

#include "Jet.hpp"
#include "types.hpp"
#include <array>

namespace Game {

class ParticleSystem {
public:
    explicit ParticleSystem(Renderer::Scene& scene);

    void spawnHitEffect(float x, float y, float z,
                        uint16_t color = Colors::SPARK_ORANGE);
    void spawnDeathEffect(float x, float y, float z);

    void update(float deltaTime);
    void reset();

private:
    struct Particle {
        Renderer::Object* obj = nullptr;
        Renderer::Material mat;
        float x, y, z;
        float vx, vy, vz;
        float life;
        bool active = false;
    };

    void spawnParticle(float x, float y, float z, float speed, float life,
                       uint16_t color);

    Renderer::Scene& m_scene;

    static constexpr int MAX_PARTICLES = 20;
    std::array<Particle, MAX_PARTICLES> m_particles;
};

} // namespace Game
