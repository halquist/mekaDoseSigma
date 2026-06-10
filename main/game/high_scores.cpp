#include "high_scores.hpp"
#include "nvs_flash.h"
#include "nvs.h"
#include <cstdio>

namespace Game {
namespace HighScores {

namespace {
constexpr const char* kNamespace = "mekadose";
constexpr const char* kKeyPrefix = "hs";

void sortDescending(int* values, int count) {
    for (int i = 0; i < count - 1; ++i) {
        for (int j = i + 1; j < count; ++j) {
            if (values[j] > values[i]) {
                const int tmp = values[i];
                values[i] = values[j];
                values[j] = tmp;
            }
        }
    }
}
} // namespace

bool init() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err == ESP_OK;
}

void load(int outScores[kTopCount]) {
    for (int i = 0; i < kTopCount; ++i) {
        outScores[i] = 0;
    }

    nvs_handle_t handle = 0;
    if (nvs_open(kNamespace, NVS_READONLY, &handle) != ESP_OK) {
        return;
    }

    for (int i = 0; i < kTopCount; ++i) {
        char key[8];
        snprintf(key, sizeof(key), "%s%d", kKeyPrefix, i);
        int32_t value = 0;
        if (nvs_get_i32(handle, key, &value) == ESP_OK && value > 0) {
            outScores[i] = static_cast<int>(value);
        }
    }

    nvs_close(handle);
}

static void save(const int scores[kTopCount]) {
    nvs_handle_t handle = 0;
    if (nvs_open(kNamespace, NVS_READWRITE, &handle) != ESP_OK) {
        return;
    }

    for (int i = 0; i < kTopCount; ++i) {
        char key[8];
        snprintf(key, sizeof(key), "%s%d", kKeyPrefix, i);
        nvs_set_i32(handle, key, static_cast<int32_t>(scores[i]));
    }

    nvs_commit(handle);
    nvs_close(handle);
}

bool tryAddScore(int score) {
    if (score <= 0) {
        return false;
    }

    int scores[kTopCount];
    load(scores);

    if (scores[kTopCount - 1] >= score && scores[kTopCount - 1] > 0) {
        return false;
    }

    int merged[kTopCount + 1];
    for (int i = 0; i < kTopCount; ++i) {
        merged[i] = scores[i];
    }
    merged[kTopCount] = score;
    sortDescending(merged, kTopCount + 1);
    for (int i = 0; i < kTopCount; ++i) {
        scores[i] = merged[i];
    }
    save(scores);
    return score >= scores[kTopCount - 1];
}

int best() {
    int scores[kTopCount];
    load(scores);
    return scores[0];
}

} // namespace HighScores
} // namespace Game
