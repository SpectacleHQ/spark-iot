# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32-S3 (N16R8) IoT controller firmware. A WS2812 LED is controlled via MQTT over WiFi. WiFi credentials are provisioned via BLE using the ESP-IDF `network_provisioning` component. LED state (color + mode) persists across reboots via NVS.

## Build Commands

Requires ESP-IDF v6.0 environment to be sourced first (`export.bat` on Windows).

```bash
idf.py build                    # Build the project
idf.py -p <COM_PORT> flash      # Flash to device
idf.py -p <COM_PORT> monitor    # Serial monitor (115200 baud)
idf.py -p <COM_PORT> flash monitor  # Flash + monitor combined
idf.py menuconfig               # Kconfig menu for sdkconfig
idf.py fullclean                # Clean build artifacts
```

## Architecture

- **Framework:** ESP-IDF v6.0 (CMake-based, provides FreeRTOS, drivers, networking, etc.)
- **Target:** ESP32-S3 (Xtensa LX7 dual-core, 240MHz, 16MB flash, 8MB OPI PSRAM) — N16R8 variant
- **Entry point:** `app_main()` in `main/spark-iot.c` — not standard `main()`
- **Build output:** `cmake-build-debug-esp-idf/` (CLion-managed)

### Data Flow

```
MQTT broker ──mqtt_ctrl──→ led_cmd_queue (FreeRTOS) ──→ led_task ──→ ws2812 (RMT)
                                                                  └→ storage (NVS)
BLE provisioning ──→ wifi_prov ──→ WiFi STA ──→ mqtt_ctrl_start()
```

The system boots, loads saved LED state from NVS, then either connects to WiFi (if provisioned) or starts BLE provisioning. Once WiFi connects, MQTT starts and listens for commands. Commands arrive as JSON on `spark-iot/set`, get parsed into `led_cmd_t`, and are sent through a FreeRTOS queue to the LED task. The LED task runs three modes: static color, rainbow cycle, and breathing effect. State changes are persisted to NVS.

### Modules

- `config.h` — All hardware pins (LED GPIO48, BTN GPIO45), MQTT topics/broker, mode enum, command struct, global queue declaration
- `ws2812.c` — WS2812 driver using RMT peripheral (10MHz tick, manual bit-bang encoding)
- `led_mode.c` — FreeRTOS task that consumes the command queue, runs LED animation modes, and persists state (only active in SYS_RUNNING)
- `storage.c` — NVS wrapper for saving/loading RGB + mode
- `wifi_prov.c` — WiFi STA setup + BLE-based network provisioning, reports state via state_set()
- `mqtt_ctrl.c` — MQTT client that subscribes to `spark-iot/set`, publishes LWT on `spark-iot/status`, reports connection state via state_set()
- `state_machine.c` — System state machine: LED status indicators for each state, reset button (GPIO45 long-press 3s) detection

### MQTT Command Protocol

Topic: `spark-iot/set` — JSON with optional fields:
```json
{"r": 255, "g": 0, "b": 128, "mode": 1}
```
Mode values: 0=static, 1=rainbow, 2=breath. Missing fields use defaults from last saved state.

Topic: `spark-iot/status` — `"online"` / `"offline"` (LWT retained).

## Conventions

- Language is C (not C++)
- Source files go in `main/` and must be registered in `main/CMakeLists.txt` `SRCS`
- Hardware config is managed through `sdkconfig`/Kconfig, not hardcoded constants
- The project uses CLion as its IDE
- External dependencies are declared in `main/idf_component.yml` and installed to `managed_components/`
- Cross-module communication uses FreeRTOS queues (`led_cmd_queue` is the single global queue)
