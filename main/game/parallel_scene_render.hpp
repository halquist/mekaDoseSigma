#pragma once

#include "Scene.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace Game {

/// Splits Jet scene rasterization across ESP32-S3 cores via prepareFrame/rasterizeBand.
class ParallelSceneRenderer {
public:
    bool init();
    void shutdown();
    void render(Renderer::Scene& scene, int screenHeight);

private:
    static void workerTask(void* arg);

    TaskHandle_t m_worker = nullptr;
    TaskHandle_t m_waiter = nullptr;
    Renderer::Scene* m_scene = nullptr;
    int m_bandYMin = 0;
    int m_bandYMax = 0;
};

} // namespace Game
