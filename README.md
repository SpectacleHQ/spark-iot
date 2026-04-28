# Spark-IoT

ESP32-S3 (N16R8) IoT controller — WS2812 LED controlled via MQTT over WiFi.

## Features

- WS2812 addressable LED control via RMT peripheral
- WiFi provisioning via BLE (using ESP-IDF `network_provisioning`)
- MQTT control with JSON commands
- LED state persistence across reboots (NVS)
- Three LED modes: static, rainbow, breathing

## Hardware

- ESP32-S3 N16R8 (16MB flash, 8MB OPI PSRAM)
- WS2812 LED on GPIO 48

## Build

Requires ESP-IDF v6.0 environment.

```bash
idf.py build
idf.py -p COM5 flash monitor
```

## MQTT Protocol

Subscribe to `led-spark/set` to send commands:

```json
{"r": 255, "g": 0, "b": 128, "mode": 1}
```

| Mode | Value |
|------|-------|
| Static | 0 |
| Rainbow | 1 |
| Breathing | 2 |

Status published to `led-spark/status` (retained): `"online"` / `"offline"`.
