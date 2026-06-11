#include "parallel_scene_render.hpp"

namespace Game {

namespace {

constexpr uint32_t kWorkerStackWords = 8192;
constexpr UBaseType_t kWorkerPriority = configMAX_PRIORITIES - 2;
constexpr int kWorkerCore = 1;

void finishFrame(Renderer::Scene& scene)
{
    scene.drawSprites();
    scene.advanceFrameCounter();
}

} // namespace

bool ParallelSceneRenderer::init()
{
    if (m_worker) {
        return true;
    }

    const BaseType_t ok = xTaskCreatePinnedToCore(
        workerTask, "jet_band", kWorkerStackWords, this,
        kWorkerPriority, &m_worker, kWorkerCore);
    return ok == pdPASS;
}

void ParallelSceneRenderer::shutdown()
{
    if (!m_worker) {
        return;
    }

    vTaskDelete(m_worker);
    m_worker = nullptr;
    m_waiter = nullptr;
    m_scene = nullptr;
}

void ParallelSceneRenderer::workerTask(void* arg)
{
    auto* self = static_cast<ParallelSceneRenderer*>(arg);

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (self->m_scene) {
            self->m_scene->rasterizeBand(self->m_bandYMin, self->m_bandYMax);
        }

        if (self->m_waiter) {
            xTaskNotifyGive(self->m_waiter);
        }
    }
}

void ParallelSceneRenderer::render(Renderer::Scene& scene, int screenHeight)
{
    if (!scene.getCamera()) {
        return;
    }

    scene.prepareFrame();

    if (!m_worker) {
        scene.rasterizeBand(0, screenHeight);
        finishFrame(scene);
        return;
    }

    const int yMid = screenHeight / 2;

    m_scene = &scene;
    m_bandYMin = yMid;
    m_bandYMax = screenHeight;
    m_waiter = xTaskGetCurrentTaskHandle();

    xTaskNotifyGive(m_worker);
    scene.rasterizeBand(0, yMid);
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    m_scene = nullptr;
    m_waiter = nullptr;

    finishFrame(scene);
}

} // namespace Game
