#pragma once

#include "Jet.hpp"
#include "environment.hpp"
#include "map_config.hpp"
#include "terrain.hpp"
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
    void computeTerrainSmoothNormals(int snappedOriginX, int snappedOriginZ);
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

    // Flat cache of mesh vertex Y values (int32_t) updated every rebuildTerrain.
    // Layout: row-major [iz * (NX+1) + ix], same order as m_terrain->vertices.
    static constexpr int kCacheSize =
        (Terrain::MESH_DEPTH_CELLS + 1) * (Terrain::MESH_WIDTH_CELLS + 1);
    int32_t m_terrainVertexY[kCacheSize] = {};

    int m_chunkOriginX = INT32_MIN;
    int m_chunkOriginZ = INT32_MIN;
};

} // namespace Game
