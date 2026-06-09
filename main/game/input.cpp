#include "input.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "touch";

namespace Game {

bool TouchInput_t::init() {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = { .clk_speed = 400000 },
        .clk_flags = 0,
    };

    esp_err_t err = i2c_param_config(I2C_NUM_0, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C config failed: %d", err);
        return false;
    }

    err = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C install failed: %d", err);
        return false;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << TOUCH_INT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "CHSC6X touch input initialized");
    return true;
}

bool TouchInput_t::isPressed() {
    if (gpio_get_level(TOUCH_INT) != 0) {
        vTaskDelay(1);
        if (gpio_get_level(TOUCH_INT) != 0) {
            return false;
        }
    }
    return true;
}

bool TouchInput_t::readBytes(uint8_t* data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (CHSC6X_ADDR << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(cmd);
    return ret == ESP_OK;
}

TouchInput TouchInput_t::read() {
    TouchInput result = { false, 0, 0 };

    if (!isPressed()) {
        return result;
    }

    uint8_t data[CHSC6X_READ_LEN];
    if (!readBytes(data, CHSC6X_READ_LEN)) {
        return result;
    }

    if (data[0] == 0x01) {
        result.touched = true;
        // Rotate touch coordinates 90° CW to match display rotation
        result.x = data[4];
        result.y = 239 - data[2];
    }

    return result;
}

} // namespace Game
