#include "obstacles.hpp"
#include "scene_util.hpp"
#include "terrain.hpp"
#include "worldgen.hpp"
#include <cmath>

namespace Game {

ObstacleField::ObstacleField(Renderer::Scene& scene, const MapConfig& mapConfig)
    : m_scene(scene)
    , m_mapConfig(mapConfig)
    , m_treeMats{Renderer::Material(Colors::TREE_FOLIAGE),
                 Renderer::Material(Colors::TREE_FOLIAGE)}
    , m_buildingMat(Colors::CITY_BUILDING)
{
    m_treeMats[0].shadingMode = Renderer::ShadingMode::FLAT;
    m_treeMats[1].shadingMode = Renderer::ShadingMode::FLAT;
    m_buildingMat.shadingMode = Renderer::ShadingMode::FLAT;

    auto* treeProtoObj = Primitives::createPyramid(TREE_BASE, TREE_HEIGHT, &m_treeMats[0]);
    m_treeProto = treeProtoObj->vertices;
    delete treeProtoObj;

    auto* buildingProtoObj = Primitives::createCube(
        BUILDING_WIDTH, BUILDING_HEIGHT, BUILDING_DEPTH, &m_buildingMat);
    m_buildingProto = buildingProtoObj->vertices;
    delete buildingProtoObj;

    for (auto& slot : m_slots) {
        slot.tree = Primitives::createPyramid(TREE_BASE, TREE_HEIGHT, &m_treeMats[0]);
        slot.tree->cullingMode = Renderer::CullingMode::NO_CULLING;
        m_scene.addObject(slot.tree);
        stashSceneObject(slot.tree);

        slot.building = Primitives::createCube(
            BUILDING_WIDTH, BUILDING_HEIGHT, BUILDING_DEPTH, &m_buildingMat);
        slot.building->cullingMode = Renderer::CullingMode::NO_CULLING;
        // slot.building->zBias = 4;
        m_scene.addObject(slot.building);
        stashSceneObject(slot.building);
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

int32_t ObstacleField::slotRenderY(const Slot& slot) const {
    if (slot.kind == WorldGen::ObstacleKind::Building) {
        return static_cast<int32_t>(lroundf(
            slot.surfaceY + BUILDING_HEIGHT * slot.scale * 0.5f));
    }
    return static_cast<int32_t>(lroundf(slot.surfaceY));
}

void ObstacleField::hideSlot(Slot& slot) {
    slot.inUse = false;
    slot.styleHash = 0;
    stashSceneObject(slot.tree);
    stashSceneObject(slot.building);
}

void ObstacleField::placeSlot(Slot& slot, const WorldGen::ObstacleSpec& spec,
                              uint32_t styleHash) {
    const bool meshDirty = !slot.inUse || slot.styleHash != styleHash;
    const MapTheme theme = WorldGen::biomeAt(spec.x, spec.z, m_mapConfig);
    const bool themeChanged = !slot.inUse || slot.theme != theme;
    const bool kindChanged = !slot.inUse || slot.kind != spec.kind;

    slot.inUse = true;
    slot.x = spec.x;
    slot.z = spec.z;
    slot.scale = spec.scale;
    slot.styleHash = styleHash;
    slot.theme = theme;
    slot.kind = spec.kind;
    slot.radius = spec.collisionRadius;

    slot.surfaceY = Terrain::heightAt(spec.x, spec.z);

    Renderer::Object* activeObj = nullptr;
    if (spec.kind == WorldGen::ObstacleKind::Building) {
        if (meshDirty || kindChanged) {
            applyMeshScale(slot.building, m_buildingProto, spec.scale);
            for (auto& tri : slot.building->triangles) {
                tri.material = &m_buildingMat;
            }
        }
        stashSceneObject(slot.tree);
        activeObj = slot.building;
    } else {
        if (meshDirty || kindChanged) {
            applyMeshScale(slot.tree, m_treeProto, spec.scale);
        }
        if (meshDirty || themeChanged || kindChanged) {
            Renderer::Material* mat =
                theme == MapTheme::DESERT ? &m_treeMats[1] : &m_treeMats[0];
            for (auto& tri : slot.tree->triangles) {
                tri.material = mat;
            }
        }
        stashSceneObject(slot.building);
        activeObj = slot.tree;
    }

    showSceneObject(activeObj,
                    static_cast<int32_t>(lroundf(spec.x)),
                    slotRenderY(slot),
                    static_cast<int32_t>(lroundf(spec.z)));
    activeObj->setRotation(0, 0, 0);
}

void ObstacleField::refreshSlotHeights(Slot& slot) {
    if (!slot.inUse) return;

    WorldGen::ObstacleSpec spec;
    spec.present = true;
    spec.kind = slot.kind;
    spec.x = slot.x;
    spec.z = slot.z;
    spec.scale = slot.scale;
    spec.collisionRadius = slot.radius;
    placeSlot(slot, spec, slot.styleHash);
}

void ObstacleField::updateSlotVisibility(Slot& slot, float playerX, float playerZ,
                                         float forwardX, float forwardZ) {
    if (!slot.inUse) return;

    const float dx = slot.x - playerX;
    const float dz = slot.z - playerZ;
    const float along = dx * forwardX + dz * forwardZ;

    Renderer::Object* activeObj =
        slot.kind == WorldGen::ObstacleKind::Building ? slot.building : slot.tree;

    if (along < -BEHIND_RENDER_CULL) {
        stashSceneObject(activeObj);
    } else {
        showSceneObject(activeObj,
                        static_cast<int32_t>(lroundf(slot.x)),
                        slotRenderY(slot),
                        static_cast<int32_t>(lroundf(slot.z)));
    }
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

void ObstacleField::updateTreeVisibility(float playerX, float playerZ,
                                         float forwardX, float forwardZ) {
    for (auto& slot : m_slots) {
        updateSlotVisibility(slot, playerX, playerZ, forwardX, forwardZ);
    }
}

void ObstacleField::unloadDistantCells(int playerCellX, int playerCellZ) {
    for (int i = m_placedCount - 1; i >= 0; i--) {
        const int dx = m_placed[i].cellX - playerCellX;
        const int dz = m_placed[i].cellZ - playerCellZ;
        if (dx > LATERAL_CELL_RADIUS + 1 || dx < -(LATERAL_CELL_RADIUS + 1) ||
            dz > FORWARD_CELL_RADIUS + 1 || dz < -(REAR_CELL_RADIUS + 1)) {
            freePlacedCell(i);
        }
    }
}

void ObstacleField::update(float playerX, float playerZ, float playerAngleDeg) {
    const int playerCellX = static_cast<int>(floorf(playerX / CELL_SIZE));
    const int playerCellZ = static_cast<int>(floorf(playerZ / CELL_SIZE));

    for (int dz = -REAR_CELL_RADIUS; dz <= FORWARD_CELL_RADIUS; dz++) {
        for (int dx = -LATERAL_CELL_RADIUS; dx <= LATERAL_CELL_RADIUS; dx++) {
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

    const float rad = playerAngleDeg * static_cast<float>(M_PI) / 180.0f;
    updateTreeVisibility(playerX, playerZ, sinf(rad), cosf(rad));
}

bool ObstacleField::isPositionBlocked(float x, float z, float radius) const {
    for (const auto& slot : m_slots) {
        if (!slot.inUse) continue;

        const float dx = x - slot.x;
        const float dz = z - slot.z;
        const float minDist = radius + slot.radius;
        if (dx * dx + dz * dz < minDist * minDist) {
            return true;
        }
    }
    return false;
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

void ObstacleField::applyEnvironment(const EnvPalette& activePalette,
                                     const EnvPalette& ruralPalette,
                                     const EnvPalette& desertPalette) {
    m_treeMats[0].color = ruralPalette.treeFoliage;
    m_treeMats[1].color = desertPalette.treeFoliage;
    m_buildingMat.color = activePalette.treeFoliage;
}

} // namespace Game
