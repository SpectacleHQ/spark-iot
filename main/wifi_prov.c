/**
 * @file wifi_prov.c
 * @brief WiFi 配网实现
 *
 * 使用 ESP-IDF network_provisioning 组件实现 BLE 配网。
 * 配网流程：
 * 1. 检查是否已配网（NVS 中有 WiFi 凭据）
 * 2. 已配网 → 直接连接 WiFi
 * 3. 未配网 → 启动 BLE 配网服务，等待手机 App 配置
 * 4. WiFi 获取 IP 后启动 MQTT 客户端
 */

#include "wifi_prov.h"
#include "config.h"
#include "mqtt_ctrl.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_idf_version.h"
#include "network_provisioning/manager.h"
#include "network_provisioning/scheme_ble.h"

static const char *TAG = "wifi_prov";

/**
 * @brief WiFi 事件处理回调
 *
 * 处理两个事件：
 * - WIFI_EVENT_STA_DISCONNECTED：断线自动重连
 * - IP_EVENT_STA_GOT_IP：获取 IP 后启动 MQTT
 *
 * @param arg 未使用
 * @param base 事件基
 * @param event_id 事件 ID
 * @param event_data 事件数据
 */
static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    if (base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "WiFi disconnected, reconnecting...");
        esp_wifi_connect();
    } else if (base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        mqtt_ctrl_start();
    }
}

/**
 * @brief 配网事件处理回调
 *
 * 监听配网过程中的关键事件（启动、完成等）。
 *
 * @param arg 未使用
 * @param base 事件基
 * @param event_id 事件 ID
 * @param event_data 事件数据
 */
static void prov_event_handler(void *arg, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    if (base == NETWORK_PROV_EVENT) {
        switch (event_id) {
            case NETWORK_PROV_START:
                ESP_LOGI(TAG, "Provisioning started");
                break;
            case NETWORK_PROV_DEINIT:
                ESP_LOGI(TAG, "Provisioning done, BLE released");
                break;
            default:
                break;
        }
    }
}

/**
 * @brief 初始化 WiFi 和配网流程
 *
 * 完整初始化步骤：
 * 1. 初始化 TCP/IP 协议栈和默认事件循环
 * 2. 创建默认 STA 网络接口
 * 3. 初始化 WiFi 驱动并注册事件处理
 * 4. 检查配网状态，决定直连还是进入 BLE 配网
 */
void wifi_prov_init(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_cfg);

    /* 注册 WiFi 断线和获取 IP 事件 */
    esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, wifi_event_handler, nullptr);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, nullptr);

    bool provisioned = false;
    network_prov_mgr_is_wifi_provisioned(&provisioned);

    if (provisioned) {
        /* 已配网：直接连接 */
        ESP_LOGI(TAG, "Already provisioned, connecting WiFi...");
        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_start();
        esp_wifi_connect();
    } else {
        /* 未配网：启动 BLE 配网 */
        ESP_LOGI(TAG, "Starting BLE provisioning...");
        esp_event_handler_register(NETWORK_PROV_EVENT, ESP_EVENT_ANY_ID, prov_event_handler, nullptr);

        network_prov_mgr_config_t prov_cfg = {
            .scheme = network_prov_scheme_ble,
            .scheme_event_handler = NETWORK_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM,
        };
        network_prov_mgr_init(prov_cfg);

        network_prov_mgr_start_provisioning(
            NETWORK_PROV_SECURITY_2,
            nullptr,
            PROV_DEVICE_NAME,
            nullptr
        );
    }
}
