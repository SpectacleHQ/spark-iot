/**
 * @file mqtt_ctrl.h
 * @brief MQTT 控制接口
 *
 * 负责 MQTT 连接管理、指令订阅和状态上报。
 * WiFi 连接成功后由 wifi_prov 调用 mqtt_ctrl_start() 启动。
 */

#pragma once

/**
 * @brief 启动 MQTT 客户端
 *
 * 连接到配置的 Broker，订阅控制主题，发布上线状态。
 * 设置 LWT（遗嘱消息），断线时自动发布 "offline"。
 */
void mqtt_ctrl_start(void);
