#include "storage.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static const char *TAG = "storage";

void storage_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition issue, erasing...");
        nvs_flash_erase();
        nvs_flash_init();
    }
}

void storage_save_led(uint8_t r, uint8_t g, uint8_t b, led_mode_t mode)
{
    nvs_handle_t h;
    if (nvs_open("led", NVS_READWRITE, &h) == ESP_OK) {
        uint8_t rgb[3] = {r, g, b};
        nvs_set_blob(h, "color", rgb, 3);
        nvs_set_u8(h, "mode", (uint8_t)mode);
        nvs_commit(h);
        nvs_close(h);
    }
}

void storage_load_led(uint8_t *r, uint8_t *g, uint8_t *b, led_mode_t *mode)
{
    nvs_handle_t h;
    *r = 255; *g = 255; *b = 255; *mode = MODE_STATIC;
    if (nvs_open("led", NVS_READONLY, &h) == ESP_OK) {
        uint8_t rgb[3];
        size_t len = 3;
        if (nvs_get_blob(h, "color", rgb, &len) == ESP_OK) {
            *r = rgb[0]; *g = rgb[1]; *b = rgb[2];
        }
        uint8_t m;
        if (nvs_get_u8(h, "mode", &m) == ESP_OK && m < MODE_MAX) {
            *mode = (led_mode_t)m;
        }
        nvs_close(h);
    }
}
