/**
 * @file wifi_prov.h
 * @brief WiFi 配网接口
 *
 * 管理 WiFi STA 连接和 BLE 配网流程。
 * 首次使用需通过 BLE 配网（使用 ESP-IDF 手机 App），
 * 配网成功后 WiFi 凭据保存在 NVS 中，后续启动自动连接。
 */

#pragma once

/**
 * @brief 初始化 WiFi 并启动配网流程
 *
 * 行为逻辑：
 * - 若已配网：直接以 STA 模式连接 WiFi
 * - 若未配网：启动 BLE 配网服务，等待手机 App 下发凭据
 *
 * WiFi 连接成功（获取 IP）后自动调用 mqtt_ctrl_start()。
 */
void wifi_prov_init(void);
