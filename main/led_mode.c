/**
 * @file led_mode.c
 * @brief LED 灯效任务实现
 *
 * 支持三种灯效模式：
 * - 静态模式（MODE_STATIC）：固定颜色常亮
 * - 彩虹模式（MODE_RAINBOW）：HSV 色相循环，固定饱和度和亮度
 * - 呼吸模式（MODE_BREATH）：亮度线性渐变，到边界反转方向
 *
 * 任务从 led_cmd_queue 接收指令，收到新指令后立即保存到 NVS。
 */

#include "led_mode.h"
#include "ws2812.h"
#include "storage.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "led_mode";

/**
 * @brief HSV 转 RGB 颜色空间
 *
 * 使用整数运算实现 HSV 到 RGB 的转换，避免浮点开销。
 *
 * @param h 色相（0-359）
 * @param s 饱和度（0-255）
 * @param v 明度（0-255）
 * @param r 输出：红色分量
 * @param g 输出：绿色分量
 * @param b 输出：蓝色分量
 */
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

/**
 * @brief LED 灯效主任务
 *
 * 任务启动时从 NVS 加载上次保存的颜色和模式，然后进入主循环。
 * 每次循环：
 * 1. 非阻塞检查指令队列，有新指令则更新状态并保存 NVS
 * 2. 根据当前模式执行对应的灯效动画
 * 3. 延时等待下一帧
 *
 * @param arg 未使用
 */
static void led_task(void *arg)
{
    led_cmd_t cmd = {.r = 255, .g = 255, .b = 255, .mode = MODE_STATIC};

    /* 从 NVS 恢复上次的 LED 状态 */
    storage_load_led(&cmd.r, &cmd.g, &cmd.b, &cmd.mode);
    ESP_LOGI(TAG, "Loaded state: r=%d g=%d b=%d mode=%d", cmd.r, cmd.g, cmd.b, cmd.mode);

    uint16_t hue = 0;        /**< 彩虹模式当前色相 */
    uint8_t brightness = 0;  /**< 呼吸模式当前亮度 */
    int8_t breath_dir = 5;   /**< 呼吸模式亮度变化方向和步长 */

    while (1) {
        /* 非阻塞读取指令队列 */
        led_cmd_t new_cmd;
        if (xQueueReceive(led_cmd_queue, &new_cmd, 0) == pdTRUE) {
            cmd = new_cmd;
            storage_save_led(cmd.r, cmd.g, cmd.b, cmd.mode);
            /* 重置动画状态 */
            hue = 0;
            brightness = 0;
            breath_dir = 5;
        }

        switch (cmd.mode) {
            case MODE_STATIC:
                /* 静态模式：直接设置颜色，低频刷新即可 */
                ws2812_set_color(cmd.r, cmd.g, cmd.b);
                vTaskDelay(pdMS_TO_TICKS(100));
                break;

            case MODE_RAINBOW: {
                /* 彩虹模式：色相循环，固定饱和度 255，亮度 128 */
                uint8_t r, g, b;
                hsv_to_rgb(hue, 255, 128, &r, &g, &b);
                ws2812_set_color(r, g, b);
                hue = (hue + 5) % 360;
                vTaskDelay(pdMS_TO_TICKS(30));
                break;
            }

            case MODE_BREATH: {
                /* 呼吸模式：亮度线性渐变，按比例缩放目标颜色 */
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
                /* 未知模式：熄灭 LED */
                ws2812_set_color(0, 0, 0);
                vTaskDelay(pdMS_TO_TICKS(100));
                break;
        }
    }
}

/**
 * @brief 创建并启动 LED 灯效任务
 */
void led_mode_start(void)
{
    xTaskCreate(led_task, "led_task", 4096, NULL, 5, NULL);
}
