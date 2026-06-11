#include "world.hpp"
#include "terrain.hpp"
#include "worldgen.hpp"
#include <climits>
#include <cmath>

namespace Game {

namespace {
constexpr float TERRAIN_EDGE_MARGIN = 90.0f;

Renderer::Material* grassMaterialForTheme(MapTheme theme,
                                          Renderer::Material* grassMats) {
    return theme == MapTheme::DESERT ? &grassMats[1] : &grassMats[0];
}
} // namespace

World::World(Renderer::Scene& scene)
    : m_scene(scene)
    , m_grassMats{Renderer::Material(Colors::GRASS), Renderer::Material(Colors::GRASS)}
{
    m_grassMats[0].shadingMode = Renderer::ShadingMode::FLAT;
    m_grassMats[1].shadingMode = Renderer::ShadingMode::FLAT;
    createTerrain();
    rebuildTerrain(0.0f, 0.0f);
}

void World::createTerrain() {
    m_terrain = new Renderer::Object();
    // Chase camera looks down at the ground; backface cull hides the tops.
    m_terrain->cullingMode = Renderer::CullingMode::NO_CULLING;
    m_terrain->zBias = -20;

    const int nx = Terrain::MESH_WIDTH_CELLS;
    const int nz = Terrain::MESH_DEPTH_CELLS;
    const int step = Terrain::MESH_CELL_SIZE;
    const int halfW = Terrain::meshHalfWidth();
    const int halfD = Terrain::meshHalfDepth();

    for (int iz = 0; iz <= nz; iz++) {
        for (int ix = 0; ix <= nx; ix++) {
            const int32_t lx = ix * step - halfW;
            const int32_t lz = iz * step - halfD;
            m_terrain->addVertex(
                {{lx, 0, lz}, {0, 0}, {0, FIXED_POINT_SCALE, 0}});
        }
    }

    for (int iz = 0; iz < nz; iz++) {
        for (int ix = 0; ix < nx; ix++) {
            const int i00 = iz * (nx + 1) + ix;
            const int i10 = i00 + 1;
            const int i01 = i00 + (nx + 1);
            const int i11 = i01 + 1;
            m_terrain->addFace(i00, i10, i11, i01, &m_grassMats[0]);
        }
    }

    m_scene.addObject(m_terrain);
}

void World::rebuildTerrain(float originX, float originZ) {
    const MapConfig& cfg = Terrain::mapConfig();
    const int nx = Terrain::MESH_WIDTH_CELLS;
    const int nz = Terrain::MESH_DEPTH_CELLS;
    const int step = Terrain::MESH_CELL_SIZE;
    const int halfW = Terrain::meshHalfWidth();
    const int halfD = Terrain::meshHalfDepth();
    const int snappedOriginX =
        static_cast<int>(lroundf(originX / static_cast<float>(step))) * step;
    const int snappedOriginZ =
        static_cast<int>(lroundf(originZ / static_cast<float>(step))) * step;

    if (snappedOriginX == m_chunkOriginX && snappedOriginZ == m_chunkOriginZ) {
        return;
    }

    int vi = 0;
    for (int iz = 0; iz <= nz; iz++) {
        for (int ix = 0; ix <= nx; ix++) {
            const int32_t lx = ix * step - halfW;
            const int32_t lz = iz * step - halfD;
            const float wx = static_cast<float>(snappedOriginX) + static_cast<float>(lx);
            const float wz = static_cast<float>(snappedOriginZ) + static_cast<float>(lz);
            m_terrain->vertices[vi].position.y =
                static_cast<int32_t>(Terrain::heightAt(wx, wz));
            vi++;
        }
    }

    int faceIndex = 0;
    for (int iz = 0; iz < nz; iz++) {
        for (int ix = 0; ix < nx; ix++) {
            const float centerLx =
                static_cast<float>(ix * step + step / 2 - halfW);
            const float centerLz =
                static_cast<float>(iz * step + step / 2 - halfD);
            const float wx = static_cast<float>(snappedOriginX) + centerLx;
            const float wz = static_cast<float>(snappedOriginZ) + centerLz;
            Renderer::Material* mat = grassMaterialForTheme(
                WorldGen::biomeAt(wx, wz, cfg), m_grassMats);
            m_terrain->triangles[faceIndex * 2].material = mat;
            m_terrain->triangles[faceIndex * 2 + 1].material = mat;
            faceIndex++;
        }
    }

    m_terrain->computeFlatNormals();
    m_terrain->calculateBoundingBox();
    m_terrain->setPosition(snappedOriginX, 0, snappedOriginZ);

    Terrain::setChunkOrigin(static_cast<float>(snappedOriginX),
                            static_cast<float>(snappedOriginZ));

    m_chunkOriginX = snappedOriginX;
    m_chunkOriginZ = snappedOriginZ;
}

bool World::shouldRecentreTerrain(float centerX, float centerZ,
                                   float lookAheadDist) const {
    if (m_chunkOriginX == INT32_MIN) {
        return true;
    }

    const float maxOffset = static_cast<float>(Terrain::meshHalfDepth()) -
                            lookAheadDist - TERRAIN_EDGE_MARGIN;
    if (maxOffset <= 0.0f) {
        return true;
    }

    const float dx = centerX - static_cast<float>(m_chunkOriginX);
    const float dz = centerZ - static_cast<float>(m_chunkOriginZ);

    return fabsf(dx) > maxOffset || fabsf(dz) > maxOffset;
}

void World::update(float centerX, float centerZ, float lookAheadDist,
                   float deltaTime, float turnActivity) {
    (void)deltaTime;
    (void)turnActivity;

    if (shouldRecentreTerrain(centerX, centerZ, lookAheadDist)) {
        rebuildTerrain(centerX, centerZ);
    }
}

void World::applyEnvironment(const EnvPalette& ruralPalette,
                             const EnvPalette& desertPalette) {
    m_grassMats[0].color = ruralPalette.grass;
    m_grassMats[1].color = desertPalette.grass;
}

} // namespace Game
