#pragma once

#include "map_config.hpp"

namespace Game {

class Terrain {
public:
    static void setMapConfig(const MapConfig* config);
    static void setChunkOrigin(float originX, float originZ);

    static float chunkOriginX() { return s_chunkOriginX; }
    static float chunkOriginZ() { return s_chunkOriginZ; }

    static float heightAt(float worldX, float worldZ);
    static float meshHeightAt(float worldX, float worldZ);
    static bool isInsideMesh(float worldX, float worldZ);
    static float hoverHeight(float worldX, float worldZ);
    static float groundHeight(float worldX, float worldZ);

    /// Must cover obstacle spawn radius (see ObstacleField::maxSpawnRadius()).
    static constexpr int MESH_CELLS = 14;
    static constexpr int MESH_CELL_SIZE = 70;
    static constexpr int meshHalfExtent() { return MESH_CELLS * MESH_CELL_SIZE / 2; }

    static constexpr float HOVER_CLEARANCE = 22.0f;
    static constexpr float GROUND_CLEARANCE = 3.0f;

private:
    static float s_chunkOriginX;
    static float s_chunkOriginZ;
};

} // namespace Game
