#include "config.h"
#include "storage.h"
#include "ws2812.h"
#include "led_mode.h"
#include "wifi_prov.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"

static const char *TAG = "spark-iot";

QueueHandle_t led_cmd_queue;

void app_main(void)
{
    ESP_LOGI(TAG, "Spark-IoT starting...");

    storage_init();

    led_cmd_queue = xQueueCreate(4, sizeof(led_cmd_t));

    ws2812_init();
    ws2812_set_color(0, 0, 0);

    wifi_prov_init();

    led_mode_start();
}
