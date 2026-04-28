#include "led_mode.h"
#include "ws2812.h"
#include "storage.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "led_mode";

static void hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    uint8_t region = h / 60;
    uint8_t remainder = (h % 60) * 255 / 60;
    uint8_t p = (uint16_t)v * (255 - s) / 255;
    uint8_t q = (uint16_t)v * (255 - (uint16_t)s * remainder / 255) / 255;
    uint8_t t = (uint16_t)v * (255 - (uint16_t)s * (255 - remainder) / 255) / 255;

    switch (region) {
        case 0:  *r = v; *g = t; *b = p; break;
        case 1:  *r = q; *g = v; *b = p; break;
        case 2:  *r = p; *g = v; *b = t; break;
        case 3:  *r = p; *g = q; *b = v; break;
        case 4:  *r = t; *g = p; *b = v; break;
        default: *r = v; *g = p; *b = q; break;
    }
}

static void led_task(void *arg)
{
    led_cmd_t cmd = {.r = 255, .g = 255, .b = 255, .mode = MODE_STATIC};
    storage_load_led(&cmd.r, &cmd.g, &cmd.b, &cmd.mode);
    ESP_LOGI(TAG, "Loaded state: r=%d g=%d b=%d mode=%d", cmd.r, cmd.g, cmd.b, cmd.mode);

    uint16_t hue = 0;
    uint8_t brightness = 0;
    int8_t breath_dir = 5;

    while (1) {
        led_cmd_t new_cmd;
        if (xQueueReceive(led_cmd_queue, &new_cmd, 0) == pdTRUE) {
            cmd = new_cmd;
            storage_save_led(cmd.r, cmd.g, cmd.b, cmd.mode);
            hue = 0;
            brightness = 0;
            breath_dir = 5;
        }

        switch (cmd.mode) {
            case MODE_STATIC:
                ws2812_set_color(cmd.r, cmd.g, cmd.b);
                vTaskDelay(pdMS_TO_TICKS(100));
                break;

            case MODE_RAINBOW: {
                uint8_t r, g, b;
                hsv_to_rgb(hue, 255, 128, &r, &g, &b);
                ws2812_set_color(r, g, b);
                hue = (hue + 5) % 360;
                vTaskDelay(pdMS_TO_TICKS(30));
                break;
            }

            case MODE_BREATH: {
                uint8_t r = (uint16_t)cmd.r * brightness / 255;
                uint8_t g = (uint16_t)cmd.g * brightness / 255;
                uint8_t b = (uint16_t)cmd.b * brightness / 255;
                ws2812_set_color(r, g, b);
                brightness += breath_dir;
                if (brightness >= 255 || brightness <= 0) breath_dir = -breath_dir;
                vTaskDelay(pdMS_TO_TICKS(15));
                break;
            }

            default:
                ws2812_set_color(0, 0, 0);
                vTaskDelay(pdMS_TO_TICKS(100));
                break;
        }
    }
}

void led_mode_start(void)
{
    xTaskCreate(led_task, "led_task", 4096, NULL, 5, NULL);
}
