#pragma once

#include "Jet.hpp"
#include "environment.hpp"
#include "types.hpp"
#include <cstdint>

namespace Game {

class World {
public:
    explicit World(Renderer::Scene& scene);

    void update(float centerX, float centerZ, float lookAheadDist,
                float deltaTime, float turnActivity);
    void resetAt(float originX, float originZ);
    void applyEnvironment(const EnvPalette& ruralPalette,
                          const EnvPalette& desertPalette);

private:
    void createTerrain();
    void rebuildTerrain(float originX, float originZ);
    bool shouldRecentreTerrain(float centerX, float centerZ,
                               float lookAheadDist) const;

    Renderer::Scene& m_scene;

    Renderer::Material m_grassMats[2];

    Renderer::Object* m_terrain = nullptr;

    int m_chunkOriginX = INT32_MIN;
    int m_chunkOriginZ = INT32_MIN;
};

} // namespace Game
