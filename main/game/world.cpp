#include "world.hpp"
#include "terrain.hpp"
#include <climits>

namespace Game {

World::World(Renderer::Scene& scene)
    : m_scene(scene)
    , m_grassMat(Colors::GRASS)
{
    m_grassMat.shadingMode = Renderer::ShadingMode::GOURAUD;
    createTerrain();
    rebuildTerrain(0.0f, 0.0f);
}

void World::createTerrain() {
    m_terrain = new Renderer::Object();
    m_terrain->cullingMode = Renderer::CullingMode::NO_CULLING;
    m_terrain->zBias = -20;

    const int n = Terrain::MESH_CELLS;
    const int step = Terrain::MESH_CELL_SIZE;
    const int half = Terrain::meshHalfExtent();

    for (int iz = 0; iz <= n; iz++) {
        for (int ix = 0; ix <= n; ix++) {
            const int32_t lx = ix * step - half;
            const int32_t lz = iz * step - half;
            m_terrain->addVertex(
                {{lx, 0, lz}, {0, 0}, {0, FIXED_POINT_SCALE, 0}});
        }
    }

    for (int iz = 0; iz < n; iz++) {
        for (int ix = 0; ix < n; ix++) {
            const int i00 = iz * (n + 1) + ix;
            const int i10 = i00 + 1;
            const int i01 = i00 + (n + 1);
            const int i11 = i01 + 1;
            m_terrain->addFace(i00, i10, i11, i01, &m_grassMat);
        }
    }

    m_scene.addObject(m_terrain);
}

void World::rebuildTerrain(float originX, float originZ) {
    const int n = Terrain::MESH_CELLS;
    const int step = Terrain::MESH_CELL_SIZE;
    const int half = Terrain::meshHalfExtent();

    int vi = 0;
    for (int iz = 0; iz <= n; iz++) {
        for (int ix = 0; ix <= n; ix++) {
            const int32_t lx = ix * step - half;
            const int32_t lz = iz * step - half;
            const float wx = originX + lx;
            const float wz = originZ + lz;
            m_terrain->vertices[vi].position.y =
                (int32_t)Terrain::heightAt(wx, wz);
            vi++;
        }
    }

    m_terrain->computeFlatNormals();
    m_terrain->calculateBoundingBox();
    m_terrain->setPosition((int16_t)originX, 0, (int16_t)originZ);

    Terrain::setChunkOrigin(originX, originZ);

    m_chunkOriginX = (int)originX;
    m_chunkOriginZ = (int)originZ;
}

void World::update(float centerX, float centerZ) {
    const int originX = (int)centerX;
    const int originZ = (int)centerZ;

    if (originX != m_chunkOriginX || originZ != m_chunkOriginZ) {
        rebuildTerrain((float)originX, (float)originZ);
    }
}

} // namespace Game
