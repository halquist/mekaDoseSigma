#pragma once

#include "Jet.hpp"
#include "types.hpp"
#include <cstdint>

namespace Game {

class World {
public:
    explicit World(Renderer::Scene& scene);

    void update(float centerX, float centerZ);

private:
    void createTerrain();
    void rebuildTerrain(float originX, float originZ);

    Renderer::Scene& m_scene;

    Renderer::Material m_grassMat;

    Renderer::Object* m_terrain = nullptr;

    int m_chunkOriginX = INT32_MIN;
    int m_chunkOriginZ = INT32_MIN;
};

} // namespace Game
