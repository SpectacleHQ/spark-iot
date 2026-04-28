/**
 * @file config.h
 * @brief 项目全局配置 — 硬件引脚、网络参数、数据结构定义
 *
 * 集中管理所有可配置项，避免硬编码散落在各模块中。
 */

#pragma once

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/* ── 硬件配置 ─────────────────────────────────────────── */

/** WS2812 LED 数据引脚 */
#define LED_GPIO          48

/** RMT 外设时钟分辨率（10MHz，对应 100ns/tick） */
#define RMT_RESOLUTION_HZ 10000000

/** 重置按钮引脚（外部下拉，按下为高电平） */
#define BTN_GPIO          45

/* ── 配网配置 ─────────────────────────────────────────── */

/** BLE 配网时广播的设备名称 */
#define PROV_DEVICE_NAME  "Spark-IoT"

/* ── MQTT 配置（从 secrets.h 注入，勿在此硬编码）────── */
#include "secrets.h"

/* ── LED 模式枚举 ──────────────────────────────────────── */

/**
 * @brief LED 灯效模式
 */
typedef enum {
    MODE_STATIC = 0, /**< 静态常亮 */
    MODE_RAINBOW, /**< 彩虹渐变 */
    MODE_BREATH, /**< 呼吸灯 */
    MODE_MAX, /**< 模式数量上限（哨兵值） */
} led_mode_t;

/* ── LED 指令结构体 ────────────────────────────────────── */

/**
 * @brief LED 控制指令，通过 FreeRTOS 队列在模块间传递
 */
typedef struct {
    uint8_t r, g, b; /**< 目标颜色（RGB 各 0-255） */
    led_mode_t mode; /**< 目标灯效模式 */
} led_cmd_t;

/* ── 全局指令队列 ──────────────────────────────────────── */

/** LED 指令队列，由 app_main 创建，mqtt_ctrl 写入，led_task 读取 */
extern QueueHandle_t led_cmd_queue;
