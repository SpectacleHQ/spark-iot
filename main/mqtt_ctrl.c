/**
 * @file mqtt_ctrl.c
 * @brief MQTT 控制实现
 *
 * MQTT 指令协议（spark-iot/set）：
 *   JSON 格式，字段均可选：{"r":255,"g":0,"b":128,"mode":1}
 *
 * 状态上报（spark-iot/status）：
 *   上线时发布 "online"，断线时通过 LWT 发布 "offline"（retained）
 *
 * 接收到的指令通过解析后发送到 led_cmd_queue。
 */

#include "mqtt_ctrl.h"
#include "config.h"
#include "state_machine.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>

static const char *TAG = "mqtt_ctrl";

/** MQTT 客户端句柄 */
static esp_mqtt_client_handle_t client = nullptr;

/**
 * @brief MQTT 事件处理回调
 *
 * 处理以下事件：
 * - CONNECTED：订阅控制主题，发布上线状态
 * - DATA：解析 JSON 指令并发送到 LED 指令队列
 * - DISCONNECTED：记录断线日志
 *
 * JSON 解析使用简单的字符串搜索（非完整 JSON 解析器），
 * 仅提取 "r"、"g"、"b"、"mode" 四个字段。
 *
 * @param arg 未使用
 * @param base 事件基
 * @param event_id 事件 ID
 * @param event_data MQTT 事件数据
 */
static void mqtt_event_handler(void *arg, esp_event_base_t base,
                               int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t) event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Connected");
            esp_mqtt_client_subscribe(client, MQTT_TOPIC_SET, 1);
            state_set(SYS_RUNNING);
            break;

        case MQTT_EVENT_DATA: {
            /* 将消息拷贝到本地缓冲区并确保 null 结尾 */
            char buf[128] = {0};
            int len = event->data_len < 127 ? event->data_len : 127;
            memcpy(buf, event->data, len);

            /*
             * 解析下发指令，格式：
             * {"LED_COLOR":[255,0,128],"LightStatus":true}
             */
            led_cmd_t cmd = {.mode = MODE_STATIC};
            char *p;

            /* 解析 LED_COLOR 数组 [r,g,b] */
            if ((p = strstr(buf, "LED_COLOR")) != nullptr) {
                char *arr = strchr(p, '[');
                if (arr != nullptr) {
                    char *end;
                    long r = strtol(arr + 1, &end, 10);
                    long g = (*end == ',') ? strtol(end + 1, &end, 10) : 0;
                    long b = (*end == ',') ? strtol(end + 1, &end, 10) : 0;
                    if (r >= 0 && r <= 255) cmd.r = (uint8_t) r;
                    if (g >= 0 && g <= 255) cmd.g = (uint8_t) g;
                    if (b >= 0 && b <= 255) cmd.b = (uint8_t) b;
                }
            }

            /* 解析 LightStatus（true=保持颜色，false=关闭） */
            if ((p = strstr(buf, "LightStatus")) != nullptr) {
                char *val = strchr(p, ':');
                if (val != nullptr && strstr(val, "false") == val + 1) {
                    cmd.r = 0;
                    cmd.g = 0;
                    cmd.b = 0;
                }
            }

            xQueueSend(led_cmd_queue, &cmd, 0);
            ESP_LOGI(TAG, "Cmd: r=%d g=%d b=%d mode=%d", cmd.r, cmd.g, cmd.b, cmd.mode);

            /* 上报当前状态 */
            {
                char resp[96];
                snprintf(resp, sizeof(resp),
                         "{\"LED_COLOR\":[%d,%d,%d],\"LightStatus\":%s}",
                         cmd.r, cmd.g, cmd.b,
                         (cmd.r || cmd.g || cmd.b) ? "true" : "false");
                esp_mqtt_client_publish(client, MQTT_TOPIC_STATUS, resp, 0, 1, 0);
            }
            break;
        }

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "Disconnected");
            state_set(SYS_RECONNECTING);
            break;

        default:
            break;
    }
}

/**
 * @brief 启动 MQTT 客户端
 *
 * 配置项：
 * - Broker 地址：MQTT_BROKER_URI
 * - LWT 主题：spark-iot/status，消息 "offline"，QoS 1，retained
 *
 * 注册事件处理后启动连接。
 */
void mqtt_ctrl_start(void) {
    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .credentials.client_id = MQTT_CLIENT_ID,
        .credentials.username = MQTT_USERNAME,
        .credentials.authentication.password = MQTT_PASSWORD,
        .session.last_will = {
            .topic = MQTT_TOPIC_STATUS,
            .msg = "offline",
            .msg_len = 6,
            .qos = 1,
            .retain = true,
        },
    };
    client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, nullptr);
    esp_mqtt_client_start(client);
}
