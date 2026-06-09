#pragma once

#include "Jet.hpp"
#include "types.hpp"
#include "map_config.hpp"
#include "worldgen.hpp"
#include <cstdint>
#include <vector>

namespace Game {

class ObstacleField {
public:
    ObstacleField(Renderer::Scene& scene, const MapConfig& mapConfig);

    void update(float playerX, float playerZ);
    void resolveMovement(float& x, float& z, float newX, float newZ, float radius) const;
    void reset();

    static constexpr float PLAYER_RADIUS = 16.0f;

private:
    struct Slot {
        bool inUse = false;
        float x = 0;
        float z = 0;
        float radius = 0;
        float scale = 1.0f;
        uint32_t styleHash = 0;
        Renderer::Object* tree = nullptr;
    };

    struct PlacedCell {
        int16_t cellX = 0;
        int16_t cellZ = 0;
        int8_t slotIndex = -1;
    };

    void hideSlot(Slot& slot);
    void applyMeshScale(Renderer::Object* obj,
                        const std::vector<Renderer::Object::Vertex>& proto,
                        float scale);
    void placeSlot(Slot& slot, const WorldGen::ObstacleSpec& spec, uint32_t styleHash);
    void refreshSlotHeights(Slot& slot);
    int findPlacedCell(int cellX, int cellZ) const;
    int allocSlot();
    void freePlacedCell(int placedIndex);
    void ensureCell(int cellX, int cellZ);
    void unloadDistantCells(int playerCellX, int playerCellZ);

    Renderer::Scene& m_scene;
    const MapConfig& m_mapConfig;
    Renderer::Material m_treeMat;

    std::vector<Renderer::Object::Vertex> m_treeProto;

    static constexpr int MAX_SLOTS = 36;
    static constexpr int MAX_PLACED = 36;
    Slot m_slots[MAX_SLOTS];
    PlacedCell m_placed[MAX_PLACED];
    int m_placedCount = 0;
    int m_lastTerrainOriginX = INT32_MIN;
    int m_lastTerrainOriginZ = INT32_MIN;

    static constexpr int CELL_SIZE = 110;
    /// Keep maxSpawnRadius() below Terrain::meshHalfExtent() (14 cells → 490).
    static constexpr int CELL_RADIUS = 3;
    static constexpr int maxSpawnRadius() {
        return CELL_RADIUS * CELL_SIZE + CELL_SIZE / 2;
    }
    static constexpr int TREE_BASE = 24;
    static constexpr int TREE_HEIGHT = 38;
};

} // namespace Game
