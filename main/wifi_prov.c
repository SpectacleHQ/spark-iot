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

void wifi_prov_init(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_cfg);
    esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);

    bool provisioned = false;
    network_prov_mgr_is_wifi_provisioned(&provisioned);

    if (provisioned) {
        ESP_LOGI(TAG, "Already provisioned, connecting WiFi...");
        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_start();
        esp_wifi_connect();
    } else {
        ESP_LOGI(TAG, "Starting BLE provisioning...");
        esp_event_handler_register(NETWORK_PROV_EVENT, ESP_EVENT_ANY_ID, prov_event_handler, NULL);

        network_prov_mgr_config_t prov_cfg = {
            .scheme = network_prov_scheme_ble,
            .scheme_event_handler = NETWORK_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM,
        };
        network_prov_mgr_init(prov_cfg);

        network_prov_mgr_start_provisioning(
            NETWORK_PROV_SECURITY_2,
            NULL,
            PROV_DEVICE_NAME,
            NULL
        );
    }
}
