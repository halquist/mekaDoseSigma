#include "world.hpp"
#include "terrain.hpp"
#include <climits>
#include <cmath>

namespace Game {

World::World(Renderer::Scene& scene)
    : m_scene(scene)
    , m_grassMats{Renderer::Material(Colors::GRASS), Renderer::Material(Colors::GRASS)}
    , m_cityGroundMat(Colors::CITY_GROUND)
    , m_cityRoadMat(Colors::CITY_ROAD)
    , m_industrialGroundMat(Colors::INDUSTRIAL_GROUND)
{
    m_grassMats[0].shadingMode = Renderer::ShadingMode::GOURAUD;
    m_grassMats[1].shadingMode = Renderer::ShadingMode::GOURAUD;
    m_cityGroundMat.shadingMode = Renderer::ShadingMode::GOURAUD;
    m_cityRoadMat.shadingMode = Renderer::ShadingMode::GOURAUD;
    m_industrialGroundMat.shadingMode = Renderer::ShadingMode::GOURAUD;
    createTerrain();
    rebuildTerrain(0.0f, 0.0f);
}

void World::createTerrain() {
    m_terrain = new Renderer::Object();
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

Renderer::Material* World::terrainMaterialFor(float worldX, float worldZ,
                                              const MapConfig& cfg) {
    switch (cfg.theme) {
    case MapTheme::CITY:
        (void)worldX;
        (void)worldZ;
        return &m_cityGroundMat;
    case MapTheme::INDUSTRIAL:
        return &m_industrialGroundMat;
    case MapTheme::DESERT:
        return &m_grassMats[1];
    case MapTheme::RURAL:
    default:
        return &m_grassMats[0];
    }
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
            Renderer::Material* mat = terrainMaterialFor(wx, wz, cfg);
            m_terrain->triangles[faceIndex * 2].material = mat;
            m_terrain->triangles[faceIndex * 2 + 1].material = mat;
            faceIndex++;
        }
    }

    computeTerrainSmoothNormals(snappedOriginX, snappedOriginZ);
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
                            lookAheadDist - 90.0f;
    if (maxOffset <= 0.0f) {
        return true;
    }

    const float dx = centerX - static_cast<float>(m_chunkOriginX);
    const float dz = centerZ - static_cast<float>(m_chunkOriginZ);

    return fabsf(dx) > maxOffset || fabsf(dz) > maxOffset;
}

void World::resetAt(float originX, float originZ) {
    m_chunkOriginX = INT32_MIN;
    m_chunkOriginZ = INT32_MIN;
    rebuildTerrain(originX, originZ);
}

void World::update(float centerX, float centerZ, float lookAheadDist,
                   float deltaTime, float turnActivity) {
    (void)deltaTime;
    (void)turnActivity;

    if (shouldRecentreTerrain(centerX, centerZ, lookAheadDist)) {
        rebuildTerrain(centerX, centerZ);
    }
}

void World::computeTerrainSmoothNormals(int snappedOriginX, int snappedOriginZ) {
    const int nx = Terrain::MESH_WIDTH_CELLS;
    const int nz = Terrain::MESH_DEPTH_CELLS;
    const int step = Terrain::MESH_CELL_SIZE;
    const int halfW = Terrain::meshHalfWidth();
    const int halfD = Terrain::meshHalfDepth();

    for (int iz = 0; iz <= nz; iz++) {
        for (int ix = 0; ix <= nx; ix++) {
            const int vi = iz * (nx + 1) + ix;

            // Fetch heights of the 4 cardinal neighbors; reuse already-computed
            // vertex Y values for in-mesh neighbors, sample outside the mesh
            // for boundary vertices (only ~60 boundary verts per rebuild).
            float hL, hR, hB, hF;

            if (ix > 0) {
                hL = static_cast<float>(
                    m_terrain->vertices[iz * (nx + 1) + (ix - 1)].position.y);
            } else {
                const float lx = static_cast<float>((ix - 1) * step - halfW);
                const float lz = static_cast<float>(iz * step - halfD);
                hL = Terrain::heightAt(static_cast<float>(snappedOriginX) + lx,
                                       static_cast<float>(snappedOriginZ) + lz);
            }

            if (ix < nx) {
                hR = static_cast<float>(
                    m_terrain->vertices[iz * (nx + 1) + (ix + 1)].position.y);
            } else {
                const float lx = static_cast<float>((ix + 1) * step - halfW);
                const float lz = static_cast<float>(iz * step - halfD);
                hR = Terrain::heightAt(static_cast<float>(snappedOriginX) + lx,
                                       static_cast<float>(snappedOriginZ) + lz);
            }

            if (iz > 0) {
                hB = static_cast<float>(
                    m_terrain->vertices[(iz - 1) * (nx + 1) + ix].position.y);
            } else {
                const float lx = static_cast<float>(ix * step - halfW);
                const float lz = static_cast<float>((iz - 1) * step - halfD);
                hB = Terrain::heightAt(static_cast<float>(snappedOriginX) + lx,
                                       static_cast<float>(snappedOriginZ) + lz);
            }

            if (iz < nz) {
                hF = static_cast<float>(
                    m_terrain->vertices[(iz + 1) * (nx + 1) + ix].position.y);
            } else {
                const float lx = static_cast<float>(ix * step - halfW);
                const float lz = static_cast<float>((iz + 1) * step - halfD);
                hF = Terrain::heightAt(static_cast<float>(snappedOriginX) + lx,
                                       static_cast<float>(snappedOriginZ) + lz);
            }

            // Central-difference height-map normal:
            //   right tangent = (2*step, hR-hL, 0)
            //   forward tangent = (0, hF-hB, 2*step)
            //   N = forward × right (gives upward-facing normal)
            const float fnx = -(hR - hL);
            const float fny = static_cast<float>(2 * step);
            const float fnz = -(hF - hB);

            const float len = sqrtf(fnx * fnx + fny * fny + fnz * fnz);
            const float scale = static_cast<float>(FIXED_POINT_SCALE) / len;

            m_terrain->vertices[vi].normal.x = static_cast<int32_t>(fnx * scale);
            m_terrain->vertices[vi].normal.y = static_cast<int32_t>(fny * scale);
            m_terrain->vertices[vi].normal.z = static_cast<int32_t>(fnz * scale);
        }
    }
}

void World::applyEnvironment(const EnvPalette& activePalette) {
    m_grassMats[0].color = activePalette.grass;
    m_grassMats[1].color = activePalette.grass;
    m_cityGroundMat.color = activePalette.grass;
    m_industrialGroundMat.color = activePalette.grass;
}

} // namespace Game
