/**
 * @file led_mode.h
 * @brief LED 灯效任务接口
 *
 * 启动一个 FreeRTOS 任务，从指令队列接收控制命令，
 * 执行对应的灯效动画，并将状态持久化到 NVS。
 */

#pragma once

#include "config.h"

/**
 * @brief 启动 LED 灯效任务
 *
 * 创建一个 FreeRTOS 任务（栈 4096 字节，优先级 5），
 * 任务内部循环执行灯效动画并监听指令队列。
 */
void led_mode_start(void);
