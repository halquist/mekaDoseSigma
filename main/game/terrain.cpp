#include "terrain.hpp"
#include "worldgen.hpp"
#include <cmath>

namespace Game {

namespace {
const MapConfig* s_mapConfig = nullptr;
const MapConfig kDefaultMapConfig;
} // namespace

float Terrain::s_chunkOriginX = 0.0f;
float Terrain::s_chunkOriginZ = 0.0f;
const int32_t* Terrain::s_vertexYCache = nullptr;
int Terrain::s_cacheOriginX = 0;
int Terrain::s_cacheOriginZ = 0;

void Terrain::setMapConfig(const MapConfig* config) {
    s_mapConfig = config;
}

const MapConfig& Terrain::mapConfig() {
    return s_mapConfig ? *s_mapConfig : kDefaultMapConfig;
}

void Terrain::setChunkOrigin(float originX, float originZ) {
    s_chunkOriginX = originX;
    s_chunkOriginZ = originZ;
}

void Terrain::setMeshCache(const int32_t* vertexYCache,
                           int snappedOriginX, int snappedOriginZ) {
    s_vertexYCache = vertexYCache;
    s_cacheOriginX = snappedOriginX;
    s_cacheOriginZ = snappedOriginZ;
}

float Terrain::heightAtFast(float worldX, float worldZ) {
    if (!s_vertexYCache) {
        return heightAt(worldX, worldZ);
    }

    constexpr int step = MESH_CELL_SIZE;
    constexpr int halfW = meshHalfWidth();
    constexpr int halfD = meshHalfDepth();
    constexpr int nx = MESH_WIDTH_CELLS;
    constexpr int nz = MESH_DEPTH_CELLS;

    // Local coordinates relative to cached chunk origin
    const float lx = worldX - static_cast<float>(s_cacheOriginX);
    const float lz = worldZ - static_cast<float>(s_cacheOriginZ);

    // Convert to grid space [0 .. nx] x [0 .. nz]
    const float gx = (lx + static_cast<float>(halfW)) / static_cast<float>(step);
    const float gz = (lz + static_cast<float>(halfD)) / static_cast<float>(step);

    // If outside cache bounds, fall back to procedural
    if (gx < 0.0f || gx > static_cast<float>(nx) ||
        gz < 0.0f || gz > static_cast<float>(nz)) {
        return heightAt(worldX, worldZ);
    }

    const int ix = static_cast<int>(gx);
    const int iz = static_cast<int>(gz);
    const int ix1 = (ix < nx) ? ix + 1 : ix;
    const int iz1 = (iz < nz) ? iz + 1 : iz;

    const float tx = gx - static_cast<float>(ix);
    const float tz = gz - static_cast<float>(iz);

    // Bilinear interpolation from the 4 surrounding vertex Y values
    const float h00 = static_cast<float>(s_vertexYCache[iz  * (nx + 1) + ix ]);
    const float h10 = static_cast<float>(s_vertexYCache[iz  * (nx + 1) + ix1]);
    const float h01 = static_cast<float>(s_vertexYCache[iz1 * (nx + 1) + ix ]);
    const float h11 = static_cast<float>(s_vertexYCache[iz1 * (nx + 1) + ix1]);

    const float h0 = h00 + (h10 - h00) * tx;
    const float h1 = h01 + (h11 - h01) * tx;
    return h0 + (h1 - h0) * tz;
}

float Terrain::heightAt(float worldX, float worldZ) {
    const MapConfig& cfg = s_mapConfig ? *s_mapConfig : kDefaultMapConfig;
    return WorldGen::terrainHeight(worldX, worldZ, cfg);
}

bool Terrain::isInsideMesh(float worldX, float worldZ) {
    const float lx = worldX - s_chunkOriginX;
    const float lz = worldZ - s_chunkOriginZ;
    const float halfW = static_cast<float>(meshHalfWidth());
    const float halfD = static_cast<float>(meshHalfDepth());
    return lx >= -halfW && lx <= halfW && lz >= -halfD && lz <= halfD;
}

float Terrain::meshHeightAt(float worldX, float worldZ) {
    return heightAt(worldX, worldZ);
}

float Terrain::hoverHeight(float worldX, float worldZ) {
    return heightAt(worldX, worldZ) + HOVER_CLEARANCE;
}

float Terrain::hoverHeightFast(float worldX, float worldZ) {
    return heightAtFast(worldX, worldZ) + HOVER_CLEARANCE;
}

float Terrain::groundHeight(float worldX, float worldZ) {
    return heightAt(worldX, worldZ) + GROUND_CLEARANCE;
}

float Terrain::groundHeightFast(float worldX, float worldZ) {
    return heightAtFast(worldX, worldZ) + GROUND_CLEARANCE;
}

} // namespace Game
