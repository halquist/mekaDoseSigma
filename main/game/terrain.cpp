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

void Terrain::setMapConfig(const MapConfig* config) {
    s_mapConfig = config;
}

void Terrain::setChunkOrigin(float originX, float originZ) {
    s_chunkOriginX = originX;
    s_chunkOriginZ = originZ;
}

float Terrain::heightAt(float worldX, float worldZ) {
    const MapConfig& cfg = s_mapConfig ? *s_mapConfig : kDefaultMapConfig;
    return WorldGen::terrainHeight(worldX, worldZ, cfg);
}

bool Terrain::isInsideMesh(float worldX, float worldZ) {
    const int half = meshHalfExtent();
    const float lx = worldX - s_chunkOriginX;
    const float lz = worldZ - s_chunkOriginZ;
    return lx >= -half && lx <= half && lz >= -half && lz <= half;
}

float Terrain::meshHeightAt(float worldX, float worldZ) {
    const int half = meshHalfExtent();
    const float lx = worldX - s_chunkOriginX;
    const float lz = worldZ - s_chunkOriginZ;

    const float u = (lx + half) / static_cast<float>(MESH_CELL_SIZE);
    const float v = (lz + half) / static_cast<float>(MESH_CELL_SIZE);

    int i0 = static_cast<int>(floorf(u));
    int j0 = static_cast<int>(floorf(v));
    if (i0 < 0) i0 = 0;
    if (j0 < 0) j0 = 0;
    if (i0 >= MESH_CELLS) i0 = MESH_CELLS - 1;
    if (j0 >= MESH_CELLS) j0 = MESH_CELLS - 1;

    int i1 = i0 + 1;
    int j1 = j0 + 1;
    if (i1 > MESH_CELLS) i1 = MESH_CELLS;
    if (j1 > MESH_CELLS) j1 = MESH_CELLS;

    const float fu = u - static_cast<float>(i0);
    const float fv = v - static_cast<float>(j0);

    auto cornerHeight = [&](int ix, int iz) {
        const float wx = s_chunkOriginX + ix * MESH_CELL_SIZE - half;
        const float wz = s_chunkOriginZ + iz * MESH_CELL_SIZE - half;
        return heightAt(wx, wz);
    };

    const float h00 = cornerHeight(i0, j0);
    const float h10 = cornerHeight(i1, j0);
    const float h01 = cornerHeight(i0, j1);
    const float h11 = cornerHeight(i1, j1);

    const float h0 = h00 + (h10 - h00) * fu;
    const float h1 = h01 + (h11 - h01) * fu;
    return h0 + (h1 - h0) * fv;
}

float Terrain::hoverHeight(float worldX, float worldZ) {
    return heightAt(worldX, worldZ) + HOVER_CLEARANCE;
}

float Terrain::groundHeight(float worldX, float worldZ) {
    return heightAt(worldX, worldZ) + GROUND_CLEARANCE;
}

} // namespace Game
