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

const MapConfig& Terrain::mapConfig() {
    return s_mapConfig ? *s_mapConfig : kDefaultMapConfig;
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

float Terrain::groundHeight(float worldX, float worldZ) {
    return heightAt(worldX, worldZ) + GROUND_CLEARANCE;
}

} // namespace Game
