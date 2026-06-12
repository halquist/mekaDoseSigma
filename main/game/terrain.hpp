#pragma once

#include "map_config.hpp"
#include <cstdint>

namespace Game {

class Terrain {
public:
    static void setMapConfig(const MapConfig* config);
    static const MapConfig& mapConfig();
    static void setChunkOrigin(float originX, float originZ);

    static float chunkOriginX() { return s_chunkOriginX; }
    static float chunkOriginZ() { return s_chunkOriginZ; }

    /// Register the terrain mesh vertex Y cache so heightAtFast can do
    /// bilinear interpolation without calling WorldGen::terrainHeight.
    /// @p vertexYCache  pointer to the flat vertex Y array (size (NZ+1)*(NX+1))
    /// @p snappedOriginX/Z  world origin that was used when the cache was built
    static void setMeshCache(const int32_t* vertexYCache,
                             int snappedOriginX, int snappedOriginZ);

    /// Fast height via bilinear interpolation from the mesh vertex cache.
    /// Falls back to heightAt() when the query is outside the cached chunk.
    static float heightAtFast(float worldX, float worldZ);

    static float heightAt(float worldX, float worldZ);
    static float meshHeightAt(float worldX, float worldZ);
    static bool isInsideMesh(float worldX, float worldZ);
    static float hoverHeight(float worldX, float worldZ);
    static float hoverHeightFast(float worldX, float worldZ);
    static float groundHeight(float worldX, float worldZ);
    static float groundHeightFast(float worldX, float worldZ);

    static constexpr int MESH_CELL_SIZE = 84;
    static constexpr int MESH_WIDTH_CELLS = 14;
    static constexpr int MESH_DEPTH_CELLS = 16;

    static constexpr int meshHalfWidth() {
        return MESH_WIDTH_CELLS * MESH_CELL_SIZE / 2;
    }
    static constexpr int meshHalfDepth() {
        return MESH_DEPTH_CELLS * MESH_CELL_SIZE / 2;
    }
    static constexpr int meshHalfExtent() { return meshHalfWidth(); }

    static constexpr float HOVER_CLEARANCE = 22.0f;
    static constexpr float GROUND_CLEARANCE = 3.0f;

private:
    static float s_chunkOriginX;
    static float s_chunkOriginZ;

    // Mesh vertex Y cache for heightAtFast.
    static const int32_t* s_vertexYCache;
    static int s_cacheOriginX;
    static int s_cacheOriginZ;
};

} // namespace Game
