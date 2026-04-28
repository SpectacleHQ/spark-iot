/**
 * @file state_machine.c
 * @brief 系统状态机实现
 *
 * 职责：
 * 1. 根据当前状态驱动 LED 指示灯（非 RUNNING 状态时）
 * 2. 检测重置按钮（GPIO45 长按 3s 清除配网并重启）
 * 3. 在 WIFI_GOT_IP 状态时自动触发 MQTT 连接
 */

#include "state_machine.h"
#include "config.h"
#include "ws2812.h"
#include "mqtt_ctrl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "esp_system.h"

static const char *TAG = "state_machine";

/** 当前系统状态，volatile 保证多任务可见性 */
static volatile sys_state_t current_state = SYS_BOOTING;

/** 重置按钮长按计时（单位：任务循环次数，50ms/次） */
#define BTN_LONG_PRESS_TICKS  (3000 / 50)  /* 3 秒 */

/**
 * @brief 设置系统状态
 */
void state_set(sys_state_t new_state)
{
    if (current_state != new_state) {
        ESP_LOGI(TAG, "State: %d -> %d", current_state, new_state);
        current_state = new_state;
    }
}

/**
 * @brief 获取当前系统状态
 */
sys_state_t state_get(void)
{
    return current_state;
}

/**
 * @brief 闪烁控制
 *
 * 根据当前 tick 和周期计算 LED 是否该亮。
 *
 * @param tick 当前计时 tick
 * @param period_ms 闪烁周期（毫秒）
 * @param task_period_ms 任务循环周期（毫秒）
 * @return true 表示亮，false 表示灭
 */
static bool blink_pattern(uint32_t tick, uint32_t period_ms, uint32_t task_period_ms)
{
    return (tick % (period_ms / task_period_ms)) < (period_ms / task_period_ms / 2);
}

/**
 * @brief 呼吸灯效果
 *
 * 根据 tick 计算亮度（三角波），返回 0-255。
 *
 * @param tick 当前计时 tick
 * @param period_ms 呼吸周期（毫秒）
 * @param task_period_ms 任务循环周期（毫秒）
 * @return 亮度值 0-255
 */
static uint8_t breath_pattern(uint32_t tick, uint32_t period_ms, uint32_t task_period_ms)
{
    uint32_t ticks_per_period = period_ms / task_period_ms;
    uint32_t pos = tick % ticks_per_period;
    uint32_t half = ticks_per_period / 2;
    if (pos < half) {
        return (uint8_t)(pos * 255 / half);
    } else {
        return (uint8_t)((ticks_per_period - pos) * 255 / half);
    }
}

/**
 * @brief 根据当前状态更新 LED 指示
 *
 * @param tick 当前 tick 计数
 */
static void update_led_indicator(uint32_t tick)
{
    const uint32_t task_ms = 50;  /* 任务循环周期 50ms */

    switch (current_state) {
        case SYS_BOOTING:
            /* 白色快闪：200ms 周期 */
            if (blink_pattern(tick, 200, task_ms)) {
                ws2812_set_color(255, 255, 255);
            } else {
                ws2812_set_color(0, 0, 0);
            }
            break;

        case SYS_CHECKING_PROV:
            /* 白色常亮 */
            ws2812_set_color(255, 255, 255);
            break;

        case SYS_NOT_PROVISIONED:
            /* 蓝色慢闪：1000ms 周期 */
            if (blink_pattern(tick, 1000, task_ms)) {
                ws2812_set_color(0, 0, 255);
            } else {
                ws2812_set_color(0, 0, 0);
            }
            break;

        case SYS_PROV_STARTING:
            /* 蓝色快闪：200ms 周期 */
            if (blink_pattern(tick, 200, task_ms)) {
                ws2812_set_color(0, 0, 255);
            } else {
                ws2812_set_color(0, 0, 0);
            }
            break;

        case SYS_PROVISIONING:
            /* 蓝色呼吸：3000ms 周期 */
            ws2812_set_color(0, 0, breath_pattern(tick, 3000, task_ms));
            break;

        case SYS_PROV_SUCCESS:
            /* 绿色快闪：200ms 周期 */
            if (blink_pattern(tick, 200, task_ms)) {
                ws2812_set_color(0, 255, 0);
            } else {
                ws2812_set_color(0, 0, 0);
            }
            break;

        case SYS_WIFI_CONNECTING:
            /* 黄色慢闪：500ms 周期 */
            if (blink_pattern(tick, 500, task_ms)) {
                ws2812_set_color(255, 255, 0);
            } else {
                ws2812_set_color(0, 0, 0);
            }
            break;

        case SYS_WIFI_GOT_IP:
            /* 绿色常亮 */
            ws2812_set_color(0, 255, 0);
            break;

        case SYS_MQTT_CONNECTING:
            /* 青色慢闪：500ms 周期 */
            if (blink_pattern(tick, 500, task_ms)) {
                ws2812_set_color(0, 255, 255);
            } else {
                ws2812_set_color(0, 0, 0);
            }
            break;

        case SYS_RUNNING:
            /* 正常运行：不控制 LED，交给 led_mode */
            break;

        case SYS_RECONNECTING:
            /* 橙色慢闪：500ms 周期 */
            if (blink_pattern(tick, 500, task_ms)) {
                ws2812_set_color(255, 128, 0);
            } else {
                ws2812_set_color(0, 0, 0);
            }
            break;

        case SYS_ERROR:
            /* 红色快闪：100ms 周期 */
            if (blink_pattern(tick, 100, task_ms)) {
                ws2812_set_color(255, 0, 0);
            } else {
                ws2812_set_color(0, 0, 0);
            }
            break;
    }
}

/**
 * @brief 检测重置按钮
 *
 * GPIO45 外部下拉，按下为高电平。
 * 持续按住超过 3 秒 → 清除配网信息并重启。
 */
static void check_reset_button(void)
{
    static uint32_t press_ticks = 0;

    if (gpio_get_level(BTN_GPIO) == 1) {
        press_ticks++;
        if (press_ticks >= BTN_LONG_PRESS_TICKS) {
            ESP_LOGW(TAG, "Reset button held 3s, erasing NVS and rebooting...");
            ws2812_set_color(255, 0, 0);  /* 红色提示 */
            vTaskDelay(pdMS_TO_TICKS(500));
            nvs_flash_erase();  /* 清除所有 NVS 数据（含 WiFi 凭据） */
            esp_restart();
        }
    } else {
        press_ticks = 0;
    }
}

/**
 * @brief 状态机主任务
 *
 * 50ms 循环：
 * 1. 更新 LED 指示（非 RUNNING 状态）
 * 2. 检测重置按钮
 * 3. 处理状态过渡（如 WIFI_GOT_IP → MQTT_CONNECTING）
 */
static void state_machine_task(void *arg)
{
    uint32_t tick = 0;

    /* 配置重置按钮 GPIO（输入，无上下拉，外部已下拉） */
    gpio_config_t btn_cfg = {
        .pin_bit_mask = (1ULL << BTN_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&btn_cfg);

    while (1) {
        /* 处理自动状态过渡 */
        if (current_state == SYS_WIFI_GOT_IP) {
            /* 获取 IP 后自动启动 MQTT */
            state_set(SYS_MQTT_CONNECTING);
            mqtt_ctrl_start();
        }

        /* 更新 LED 指示（RUNNING 状态由 led_mode 控制） */
        if (current_state != SYS_RUNNING) {
            update_led_indicator(tick);
        }

        /* 检测重置按钮 */
        check_reset_button();

        tick++;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/**
 * @brief 初始化状态机
 */
void state_machine_init(void)
{
    state_set(SYS_BOOTING);
    xTaskCreate(state_machine_task, "state_machine", 4096, nullptr, 6, nullptr);
}
