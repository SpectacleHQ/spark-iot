#include "mqtt_ctrl.h"
#include "config.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "mqtt_ctrl";
static esp_mqtt_client_handle_t client = NULL;

static void mqtt_event_handler(void *arg, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Connected");
            esp_mqtt_client_subscribe(client, MQTT_TOPIC_SET, 1);
            esp_mqtt_client_publish(client, MQTT_TOPIC_STATUS, "online", 0, 1, 1);
            break;

        case MQTT_EVENT_DATA: {
            char buf[128] = {0};
            int len = event->data_len < 127 ? event->data_len : 127;
            memcpy(buf, event->data, len);

            led_cmd_t cmd = {.mode = MODE_STATIC};
            char *p;
            if ((p = strstr(buf, "\"r\"")) != NULL) cmd.r = atoi(strchr(p, ':') + 1);
            if ((p = strstr(buf, "\"g\"")) != NULL) cmd.g = atoi(strchr(p, ':') + 1);
            if ((p = strstr(buf, "\"b\"")) != NULL) cmd.b = atoi(strchr(p, ':') + 1);
            if ((p = strstr(buf, "\"mode\"")) != NULL) {
                int m = atoi(strchr(p, ':') + 1);
                if (m >= 0 && m < MODE_MAX) cmd.mode = (led_mode_t)m;
            }

            xQueueSend(led_cmd_queue, &cmd, 0);
            ESP_LOGI(TAG, "Cmd: r=%d g=%d b=%d mode=%d", cmd.r, cmd.g, cmd.b, cmd.mode);
            break;
        }

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Disconnected");
            break;

        default:
            break;
    }
}

void mqtt_ctrl_start(void)
{
    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .session.last_will = {
            .topic = MQTT_TOPIC_STATUS,
            .msg = "offline",
            .msg_len = 6,
            .qos = 1,
            .retain = true,
        },
    };
    client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}
