#include "obstacles.hpp"
#include "terrain.hpp"
#include "worldgen.hpp"
#include <cmath>

namespace Game {

ObstacleField::ObstacleField(Renderer::Scene& scene, const MapConfig& mapConfig)
    : m_scene(scene)
    , m_mapConfig(mapConfig)
    , m_treeMat(Colors::TREE_FOLIAGE)
{
    m_treeMat.shadingMode = Renderer::ShadingMode::GOURAUD;

    auto* treeProtoObj = Primitives::createPyramid(TREE_BASE, TREE_HEIGHT, &m_treeMat);
    m_treeProto = treeProtoObj->vertices;
    delete treeProtoObj;

    for (auto& slot : m_slots) {
        slot.tree = Primitives::createPyramid(TREE_BASE, TREE_HEIGHT, &m_treeMat);
        slot.tree->cullingMode = Renderer::CullingMode::NO_CULLING;
        m_scene.addObject(slot.tree);
        hideSlot(slot);
    }
}

void ObstacleField::applyMeshScale(Renderer::Object* obj,
                                   const std::vector<Renderer::Object::Vertex>& proto,
                                   float scale) {
    if (!obj) return;
    obj->vertices = proto;
    for (auto& vert : obj->vertices) {
        vert.position.x = static_cast<int32_t>(vert.position.x * scale);
        vert.position.y = static_cast<int32_t>(vert.position.y * scale);
        vert.position.z = static_cast<int32_t>(vert.position.z * scale);
    }
    obj->computeFlatNormals();
    obj->calculateBoundingBox();
}

void ObstacleField::hideSlot(Slot& slot) {
    slot.inUse = false;
    slot.styleHash = 0;
    if (slot.tree) slot.tree->setPosition(0, -1000, 0);
}

void ObstacleField::placeSlot(Slot& slot, const WorldGen::ObstacleSpec& spec,
                              uint32_t styleHash) {
    const bool meshDirty = !slot.inUse || slot.styleHash != styleHash;

    slot.inUse = true;
    slot.x = spec.x;
    slot.z = spec.z;
    slot.scale = spec.scale;
    slot.styleHash = styleHash;
    slot.radius = 12.0f * spec.scale;

    if (!Terrain::isInsideMesh(spec.x, spec.z)) {
        if (slot.tree) slot.tree->setPosition(0, -1000, 0);
        return;
    }

    const float surfaceY = Terrain::meshHeightAt(spec.x, spec.z);

    if (meshDirty) {
        applyMeshScale(slot.tree, m_treeProto, spec.scale);
    }
    slot.tree->setPosition((int16_t)spec.x, (int16_t)surfaceY, (int16_t)spec.z);
    slot.tree->setRotation(0, 0, 0);
}

void ObstacleField::refreshSlotHeights(Slot& slot) {
    if (!slot.inUse) return;

    WorldGen::ObstacleSpec spec;
    spec.present = true;
    spec.x = slot.x;
    spec.z = slot.z;
    spec.scale = slot.scale;
    placeSlot(slot, spec, slot.styleHash);
}

int ObstacleField::findPlacedCell(int cellX, int cellZ) const {
    for (int i = 0; i < m_placedCount; i++) {
        if (m_placed[i].cellX == cellX && m_placed[i].cellZ == cellZ) {
            return i;
        }
    }
    return -1;
}

int ObstacleField::allocSlot() {
    for (int i = 0; i < MAX_SLOTS; i++) {
        if (!m_slots[i].inUse) {
            return i;
        }
    }
    return -1;
}

void ObstacleField::freePlacedCell(int placedIndex) {
    if (placedIndex < 0 || placedIndex >= m_placedCount) return;
    const int slotIndex = m_placed[placedIndex].slotIndex;
    if (slotIndex >= 0 && slotIndex < MAX_SLOTS) {
        hideSlot(m_slots[slotIndex]);
    }
    m_placed[placedIndex] = m_placed[m_placedCount - 1];
    m_placedCount--;
}

void ObstacleField::ensureCell(int cellX, int cellZ) {
    if (findPlacedCell(cellX, cellZ) >= 0) {
        return;
    }

    const WorldGen::ObstacleSpec spec =
        WorldGen::sampleObstacle(cellX, cellZ, m_mapConfig);
    if (!spec.present) {
        return;
    }

    const int slotIndex = allocSlot();
    if (slotIndex < 0 || m_placedCount >= MAX_PLACED) {
        return;
    }

    const uint32_t styleHash = WorldGen::hash(cellX, cellZ, m_mapConfig.worldSeed);
    placeSlot(m_slots[slotIndex], spec, styleHash);

    m_placed[m_placedCount].cellX = static_cast<int16_t>(cellX);
    m_placed[m_placedCount].cellZ = static_cast<int16_t>(cellZ);
    m_placed[m_placedCount].slotIndex = static_cast<int8_t>(slotIndex);
    m_placedCount++;
}

void ObstacleField::unloadDistantCells(int playerCellX, int playerCellZ) {
    for (int i = m_placedCount - 1; i >= 0; i--) {
        const int dx = m_placed[i].cellX - playerCellX;
        const int dz = m_placed[i].cellZ - playerCellZ;
        if (dx > CELL_RADIUS + 1 || dx < -(CELL_RADIUS + 1) ||
            dz > CELL_RADIUS + 1 || dz < -(CELL_RADIUS + 1)) {
            freePlacedCell(i);
        }
    }
}

void ObstacleField::update(float playerX, float playerZ) {
    const int playerCellX = static_cast<int>(floorf(playerX / CELL_SIZE));
    const int playerCellZ = static_cast<int>(floorf(playerZ / CELL_SIZE));

    for (int dz = -CELL_RADIUS; dz <= CELL_RADIUS; dz++) {
        for (int dx = -CELL_RADIUS; dx <= CELL_RADIUS; dx++) {
            ensureCell(playerCellX + dx, playerCellZ + dz);
        }
    }

    unloadDistantCells(playerCellX, playerCellZ);

    const int terrainOx = static_cast<int>(Terrain::chunkOriginX());
    const int terrainOz = static_cast<int>(Terrain::chunkOriginZ());
    const bool terrainMoved =
        terrainOx != m_lastTerrainOriginX || terrainOz != m_lastTerrainOriginZ;
    const bool firstUpdate = m_lastTerrainOriginX == INT32_MIN;
    if (terrainMoved || firstUpdate) {
        m_lastTerrainOriginX = terrainOx;
        m_lastTerrainOriginZ = terrainOz;
        for (int i = 0; i < m_placedCount; i++) {
            const int slotIndex = m_placed[i].slotIndex;
            if (slotIndex >= 0) {
                refreshSlotHeights(m_slots[slotIndex]);
            }
        }
    }
}

void ObstacleField::resolveMovement(float& x, float& z,
                                    float newX, float newZ, float radius) const {
    x = newX;
    z = newZ;

    for (int pass = 0; pass < 3; pass++) {
        for (const auto& slot : m_slots) {
            if (!slot.inUse) continue;

            float dx = x - slot.x;
            float dz = z - slot.z;
            float distSq = dx * dx + dz * dz;
            const float minDist = radius + slot.radius;

            if (distSq >= minDist * minDist) continue;

            float dist = sqrtf(distSq);
            if (dist < 0.001f) {
                dx = 1.0f;
                dz = 0.0f;
                dist = 1.0f;
            }

            const float push = minDist - dist;
            x += (dx / dist) * push;
            z += (dz / dist) * push;
        }
    }
}

void ObstacleField::reset() {
    m_lastTerrainOriginX = INT32_MIN;
    m_lastTerrainOriginZ = INT32_MIN;
    for (int i = m_placedCount - 1; i >= 0; i--) {
        freePlacedCell(i);
    }
    m_placedCount = 0;
}

} // namespace Game
