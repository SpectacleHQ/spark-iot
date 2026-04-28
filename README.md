# Spark-IoT

ESP32-S3 (N16R8) IoT 控制器 — 通过 MQTT over WiFi 控制 WS2812 LED。

## 功能

- WS2812 可寻址 LED 控制（RMT 外设驱动）
- WiFi BLE 配网（基于 ESP-IDF `network_provisioning` 组件）
- MQTT JSON 指令控制
- LED 状态掉电保存（NVS）
- 三种灯效模式：静态、彩虹、呼吸

## 硬件

- ESP32-S3 N16R8（16MB Flash，8MB OPI PSRAM）
- WS2812 LED 接 GPIO 48

## 构建

需要 ESP-IDF v6.0 环境。

```bash
idf.py build
idf.py -p COM5 flash monitor
```

## MQTT 协议

订阅 `spark-iot/set` 发送控制指令：

```json
{"r": 255, "g": 0, "b": 128, "mode": 1}
```

| 模式 | 值 |
|------|----|
| 静态 | 0 |
| 彩虹 | 1 |
| 呼吸 | 2 |

状态发布到 `spark-iot/status`（retained）：`"online"` / `"offline"`。
