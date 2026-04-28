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
#include "state_machine.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include <string.h>
#include "network_provisioning/manager.h"
#include "network_provisioning/scheme_ble.h"
#include "protocomm_security.h"

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
        state_set(SYS_RECONNECTING);
        esp_wifi_connect();
    } else if (base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        state_set(SYS_WIFI_GOT_IP);
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
                state_set(SYS_PROVISIONING);
                break;
            case NETWORK_PROV_DEINIT:
                ESP_LOGI(TAG, "Provisioning done, BLE released");
                state_set(SYS_PROV_SUCCESS);
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
        state_set(SYS_WIFI_CONNECTING);
        ESP_LOGI(TAG, "Already provisioned, connecting WiFi...");
        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_start();
        esp_wifi_connect();
    } else {
        /* 未配网：启动 BLE 配网 */
        state_set(SYS_NOT_PROVISIONED);
        ESP_LOGI(TAG, "Starting BLE provisioning...");
        esp_event_handler_register(NETWORK_PROV_EVENT, ESP_EVENT_ANY_ID, prov_event_handler, nullptr);

        network_prov_mgr_config_t prov_cfg = {
            .scheme = network_prov_scheme_ble,
            .scheme_event_handler = NETWORK_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM,
        };
        network_prov_mgr_init(prov_cfg);
        state_set(SYS_PROV_STARTING);

        /* Security 2：SRP6a (3072-bit) 握手 + AES-256-GCM 加密 */
        static const char sec_salt[] = "\xe4\xf4\xee\xb6\xf4\x4a\x27\xa4\x1d\x5c\xb2\xcd\xdc\xaa\x46\x46";
        static const char sec_verifier[] = "\x92\x0d\xad\xf8\x62\x01\xbc\x60\x5a\x94\xe3\xfb\xfd\x09\x24\x80"
            "\x06\x64\xf2\xc8\x3e\x27\x87\x15\xdb\x11\x5d\xbd\x40\x89\x97\x76"
            "\x75\x5c\x9d\xe3\x31\xdd\xf4\x41\xa2\x75\xbb\xc2\xcb\xe8\x33\x1c"
            "\xd5\xd3\x85\xe3\xbb\xfb\x37\xa8\xbb\x9b\xa6\xdb\xfb\xd4\x65\xe2"
            "\x54\x9f\x97\x3e\x3b\x02\x32\x77\xd1\x54\x06\xec\x0e\x60\x39\x0f"
            "\x60\xe7\xfc\x0b\x3f\x58\x62\xa8\x50\xee\xa1\xf9\x44\x1e\xa9\x7f"
            "\x6b\x1e\xd5\xb2\x30\x50\xe4\xcd\xeb\x15\xe4\x8e\x7c\x2c\xfa\x75"
            "\xad\x65\xac\xab\x78\x29\x23\x65\xcb\x0b\x73\xf9\x47\x0f\xd3\xc5"
            "\x2f\x7e\xf5\xc2\x76\xfc\x19\xa6\x80\xfd\xd0\x64\x8f\xaa\xd0\x60"
            "\xe8\x46\xf4\x90\x52\x6b\xed\x01\x8b\x70\x28\x7b\x08\xf7\x55\x01"
            "\xce\x90\xd5\xfe\xdc\xce\xde\xb6\x4c\xc8\xa8\xe6\x68\x49\xc9\xb0"
            "\xf4\x06\x86\xa8\x2d\x0c\x17\x77\x93\xed\x33\xc2\x84\xe1\xab\x38"
            "\x6e\xbc\x5c\x47\xd9\x27\xfe\x0b\x7b\x50\xd4\xfa\x9f\xc4\x4b\x71"
            "\x98\xf8\x26\x9e\x9d\x0b\x80\xa5\xbd\xc2\x8a\xbf\x17\xb3\xa9\x08"
            "\x21\x62\xda\xfe\xf1\x7a\x50\xf2\x76\x1a\x2c\xd2\x45\x20\xbd\xdb"
            "\x24\x00\xed\x78\xfc\xa1\x22\x64\xae\xb2\x8c\x6a\xfc\x6e\xd0\x5c"
            "\xee\x75\x72\xa0\x8f\x17\x8d\xfb\x81\xd2\x27\x53\xdf\xef\xb6\xa1"
            "\xf4\xdb\x5f\xd7\xaa\x2f\xb0\xb7\xca\xf7\x07\xda\x47\x3f\xc3\xdc"
            "\xdf\xf2\x02\xfe\xf8\x67\x68\xa3\x46\x57\x69\x70\x2b\x0c\x00\xa0"
            "\xb1\x3e\xba\xbf\xee\x2e\xb6\x70\x8e\x42\xb9\x4d\xfd\xac\x24\xd7"
            "\x5e\x99\x66\x61\x83\x67\xdc\x83\x42\x10\xcd\x4a\x64\x93\x3b\xf4"
            "\x1a\x86\x96\x6d\x6a\x55\x2c\xa8\x5e\x71\xdf\x11\xf8\xdd\x74\x79"
            "\x7c\x29\xfc\xf1\x8c\x3b\x1d\x8e\x55\x44\xde\x7b\x45\x66\x5c\x86"
            "\xfb\x16\x93\x1a\xd2\x5f\x25\xad\xeb\x45\x89\x8e\xcc\x01\x90\x62";

        protocomm_security2_params_t sec_params = {
            .salt = sec_salt,
            .salt_len = sizeof(sec_salt) - 1,
            .verifier = sec_verifier,
            .verifier_len = sizeof(sec_verifier) - 1,
        };

        network_prov_mgr_start_provisioning(
            NETWORK_PROV_SECURITY_2,
            &sec_params,
            PROV_DEVICE_NAME,
            nullptr
        );
    }
}
