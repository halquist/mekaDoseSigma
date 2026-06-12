#pragma once

#include "Jet.hpp"
#include "environment.hpp"
#include "map_config.hpp"
#include "types.hpp"
#include <cstdint>

namespace Game {

class World {
public:
    explicit World(Renderer::Scene& scene);

    void update(float centerX, float centerZ, float lookAheadDist,
                float deltaTime, float turnActivity);
    void resetAt(float originX, float originZ);
    void applyEnvironment(const EnvPalette& activePalette);

private:
    void createTerrain();
    void rebuildTerrain(float originX, float originZ);
    bool shouldRecentreTerrain(float centerX, float centerZ,
                               float lookAheadDist) const;
    Renderer::Material* terrainMaterialFor(float worldX, float worldZ,
                                           const MapConfig& cfg);

    Renderer::Scene& m_scene;

    Renderer::Material m_grassMats[2];
    Renderer::Material m_cityGroundMat;
    Renderer::Material m_cityRoadMat;
    Renderer::Material m_industrialGroundMat;

    Renderer::Object* m_terrain = nullptr;

    int m_chunkOriginX = INT32_MIN;
    int m_chunkOriginZ = INT32_MIN;
};

} // namespace Game
