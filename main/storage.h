/**
 * @file storage.h
 * @brief NVS 持久化存储接口
 *
 * 提供 LED 状态（颜色 + 模式）的保存和加载功能，
 * 数据存储在 NVS 的 "led" 命名空间中。
 */

#pragma once

#include "config.h"

/**
 * @brief 初始化 NVS Flash 存储
 *
 * 调用 nvs_flash_init()，若分区异常则擦除后重新初始化。
 * 必须在其他 storage 函数之前调用。
 */
void storage_init(void);

/**
 * @brief 保存 LED 状态到 NVS
 *
 * 将 RGB 颜色以 blob（3 字节）形式存储，模式以 u8 形式存储。
 * 使用 "led" 命名空间，键名 "color" 和 "mode"。
 *
 * @param r 红色分量
 * @param g 绿色分量
 * @param b 蓝色分量
 * @param mode 灯效模式
 */
void storage_save_led(uint8_t r, uint8_t g, uint8_t b, led_mode_t mode);

/**
 * @brief 从 NVS 加载 LED 状态
 *
 * 若 NVS 中无数据或读取失败，则使用默认值（白色静态）。
 *
 * @param r 输出：红色分量（默认 255）
 * @param g 输出：绿色分量（默认 255）
 * @param b 输出：蓝色分量（默认 255）
 * @param mode 输出：灯效模式（默认 MODE_STATIC）
 */
void storage_load_led(uint8_t *r, uint8_t *g, uint8_t *b, led_mode_t *mode);
