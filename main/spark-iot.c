/**
 * @file spark-iot.c
 * @brief 应用入口 — 初始化各子系统并启动主循环
 *
 * 启动顺序：NVS 存储 → 指令队列 → WS2812 驱动 → 状态机 → WiFi/BLE 配网 → LED 任务
 */

#include "config.h"
#include "storage.h"
#include "ws2812.h"
#include "led_mode.h"
#include "wifi_prov.h"
#include "state_machine.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"

static const char *TAG = "spark-iot";

/** LED 指令队列，深度 4，元素类型 led_cmd_t */
QueueHandle_t led_cmd_queue;

/**
 * @brief 应用主入口（ESP-IDF 替代标准 main）
 *
 * 按顺序完成以下初始化：
 * 1. NVS 存储初始化（用于持久化 LED 状态）
 * 2. 创建 LED 指令队列
 * 3. 初始化 WS2812 驱动并熄灭 LED
 * 4. 启动状态机（LED 指示 + 重置按钮检测）
 * 5. 启动 WiFi 配网（已配网则直连，否则进入 BLE 配网）
 * 6. 启动 LED 模式任务
 */
void app_main(void) {
    ESP_LOGI(TAG, "Spark-IoT starting...");

    storage_init();

    led_cmd_queue = xQueueCreate(4, sizeof(led_cmd_t));

    ws2812_init();
    ws2812_set_color(0, 0, 0);

    state_machine_init();
    wifi_prov_init();

    led_mode_start();
}
