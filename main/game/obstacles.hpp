#pragma once

#include "Jet.hpp"
#include "environment.hpp"
#include "types.hpp"
#include "map_config.hpp"
#include "worldgen.hpp"
#include <cstdint>
#include <vector>

namespace Game {

class ObstacleField {
public:
    ObstacleField(Renderer::Scene& scene, const MapConfig& mapConfig);

    void update(float playerX, float playerZ, float playerAngleDeg);
    void resolveMovement(float& x, float& z, float newX, float newZ, float radius) const;
    bool isPositionBlocked(float x, float z, float radius) const;
    void reset();
    void applyEnvironment(const EnvPalette& activePalette,
                          const EnvPalette& ruralPalette,
                          const EnvPalette& desertPalette);

    static constexpr float PLAYER_RADIUS = 16.0f;

private:
    struct Slot {
        bool inUse = false;
        float x = 0;
        float z = 0;
        float radius = 0;
        float scale = 1.0f;
        uint32_t styleHash = 0;
        float surfaceY = 0.0f;
        MapTheme theme = MapTheme::RURAL;
        WorldGen::ObstacleKind kind = WorldGen::ObstacleKind::Tree;
        Renderer::Object* tree = nullptr;
        Renderer::Object* building = nullptr;
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
    int32_t slotRenderY(const Slot& slot) const;
    void updateSlotVisibility(Slot& slot, float playerX, float playerZ,
                              float forwardX, float forwardZ);
    void updateTreeVisibility(float playerX, float playerZ, float forwardX, float forwardZ);
    int findPlacedCell(int cellX, int cellZ) const;
    int allocSlot();
    void freePlacedCell(int placedIndex);
    void ensureCell(int cellX, int cellZ);
    void unloadDistantCells(int playerCellX, int playerCellZ);

    Renderer::Scene& m_scene;
    const MapConfig& m_mapConfig;
    Renderer::Material m_treeMats[2];
    Renderer::Material m_buildingMat;

    std::vector<Renderer::Object::Vertex> m_treeProto;
    std::vector<Renderer::Object::Vertex> m_buildingProto;

    static constexpr int MAX_SLOTS = 48;
    static constexpr int MAX_PLACED = 48;
    Slot m_slots[MAX_SLOTS];
    PlacedCell m_placed[MAX_PLACED];
    int m_placedCount = 0;
    int m_lastTerrainOriginX = INT32_MIN;
    int m_lastTerrainOriginZ = INT32_MIN;

    static constexpr int CELL_SIZE = 110;
    static constexpr int FORWARD_CELL_RADIUS = 4;
    static constexpr int REAR_CELL_RADIUS = 3;
    static constexpr int LATERAL_CELL_RADIUS = 3;
    static constexpr float BEHIND_RENDER_CULL = 90.0f;
    static constexpr int maxSpawnRadius() {
        return FORWARD_CELL_RADIUS * CELL_SIZE + CELL_SIZE / 2;
    }
    static constexpr int TREE_BASE = 24;
    static constexpr int TREE_HEIGHT = 38;
    static constexpr int BUILDING_WIDTH = 54;
    static constexpr int BUILDING_HEIGHT = 144;
    static constexpr int BUILDING_DEPTH = 54;
};

} // namespace Game
