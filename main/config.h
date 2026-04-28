#pragma once

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/* ── 硬件 ─────────────────────────────────────────── */
#define LED_GPIO          48
#define RMT_RESOLUTION_HZ 10000000

/* ── 配网 ─────────────────────────────────────────── */
#define PROV_DEVICE_NAME  "Spark-IoT"

/* ── MQTT ─────────────────────────────────────────── */
#define MQTT_BROKER_URI   "mqtt://broker.emqx.io:1883"  // TODO: 改成你自己的 broker
#define MQTT_TOPIC_SET    "spark-iot/set"
#define MQTT_TOPIC_STATUS "spark-iot/status"

/* ── LED 模式 ─────────────────────────────────────── */
typedef enum {
    MODE_STATIC = 0,
    MODE_RAINBOW,
    MODE_BREATH,
    MODE_MAX,
} led_mode_t;

/* ── LED 指令（通过队列传递）──────────────────────── */
typedef struct {
    uint8_t r, g, b;
    led_mode_t mode;
} led_cmd_t;

/* ── 全局指令队列 ─────────────────────────────────── */
extern QueueHandle_t led_cmd_queue;
