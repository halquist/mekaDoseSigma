/**
 * mekaDoseSigma — M0 engine port
 * Seeed XIAO ESP32-S3 + 240×240 round GC9A01 display + CHSC6X touch
 */

#include <cstdio>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_gc9a01.h"
#include "esp_heap_caps.h"

#include "game/game.hpp"

static const char* TAG = "mekadose";

#define LCD_WIDTH  240
#define LCD_HEIGHT 240

#define PIN_LCD_CS    GPIO_NUM_2
#define PIN_LCD_DC    GPIO_NUM_4
#define PIN_LCD_BL    GPIO_NUM_43
#define PIN_LCD_SCK   GPIO_NUM_7
#define PIN_LCD_MOSI  GPIO_NUM_9

#define LCD_SPI_HOST SPI2_HOST
#define LCD_PIXEL_CLOCK_HZ (80 * 1000 * 1000)

static uint16_t* framebuffer = nullptr;
static esp_lcd_panel_handle_t panel_handle = nullptr;
static esp_lcd_panel_io_handle_t io_handle = nullptr;
static volatile bool transfer_done = true;

static bool IRAM_ATTR lcd_trans_done_cb(esp_lcd_panel_io_handle_t panel_io,
                                        esp_lcd_panel_io_event_data_t* edata,
                                        void* user_ctx)
{
    transfer_done = true;
    return false;
}

static void init_display(void)
{
    ESP_LOGI(TAG, "Initializing display...");

    gpio_config_t bl_gpio_config = {
        .pin_bit_mask = 1ULL << PIN_LCD_BL,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&bl_gpio_config));
    gpio_set_level(PIN_LCD_BL, 1);

    spi_bus_config_t bus_config = {
        .mosi_io_num = PIN_LCD_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = PIN_LCD_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = PIN_LCD_CS,
        .dc_gpio_num = PIN_LCD_DC,
        .spi_mode = 0,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .trans_queue_depth = 10,
        .on_color_trans_done = lcd_trans_done_cb,
        .user_ctx = nullptr,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .flags = {
            .dc_low_on_data = 0,
            .octal_mode = 0,
            .quad_mode = 0,
            .sio_mode = 0,
            .lsb_first = 0,
            .cs_high_active = 0,
        },
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST, &io_config, &io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = -1,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(TAG, "Display initialized");
}

static void push_framebuffer(void)
{
    transfer_done = false;
    esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_WIDTH, LCD_HEIGHT, framebuffer);
    while (!transfer_done) {
        taskYIELD();
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "=== mekaDoseSigma M0 ===");

    size_t fb_size = LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t);
    framebuffer = (uint16_t*)heap_caps_malloc(fb_size, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!framebuffer) {
        framebuffer = (uint16_t*)heap_caps_malloc(fb_size, MALLOC_CAP_SPIRAM);
    }
    if (!framebuffer) {
        ESP_LOGE(TAG, "Failed to allocate framebuffer!");
        return;
    }
    memset(framebuffer, 0, fb_size);

    init_display();

    Game::MekaGame game(framebuffer, LCD_WIDTH, LCD_HEIGHT);
    if (!game.init()) {
        ESP_LOGE(TAG, "Failed to initialize game!");
        return;
    }

    ESP_LOGI(TAG, "M2 combat loop — menu tap to start, destroy objectives to upgrade");

    int64_t lastTime = esp_timer_get_time();
    int frameCount = 0;
    int64_t fpsTimer = lastTime;

    while (true) {
        int64_t now = esp_timer_get_time();
        float deltaTime = (now - lastTime) / 1000000.0f;
        lastTime = now;

        if (deltaTime > 0.1f) {
            deltaTime = 0.1f;
        }

        game.update(deltaTime);
        game.render();
        push_framebuffer();

        frameCount++;

        if (now - fpsTimer >= 2000000) {
            float fps = frameCount * 1000000.0f / (now - fpsTimer);
            ESP_LOGI(TAG, "FPS: %.1f", fps);
            frameCount = 0;
            fpsTimer = now;
        }

        vTaskDelay(1);
    }
}
