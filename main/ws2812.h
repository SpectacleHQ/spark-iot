/**
 * @file ws2812.h
 * @brief WS2812 可寻址 LED 驱动接口
 *
 * 使用 ESP32-S3 RMT 外设生成 WS2812 时序信号，支持单颗 LED 的颜色控制。
 */

#pragma once

#include <stdint.h>

/**
 * @brief 初始化 WS2812 驱动
 *
 * 配置 RMT 发送通道（GPIO、时钟分辨率、内存块），使能通道。
 * 必须在调用 ws2812_set_color() 之前执行。
 */
void ws2812_init(void);

/**
 * @brief 设置 WS2812 LED 颜色
 *
 * 将 RGB 值编码为 WS2812 时序信号并通过 RMT 发送。
 * 数据格式为 GRB 顺序（WS2812 协议要求）。
 *
 * @param r 红色分量（0-255）
 * @param g 绿色分量（0-255）
 * @param b 蓝色分量（0-255）
 */
void ws2812_set_color(uint8_t r, uint8_t g, uint8_t b);
