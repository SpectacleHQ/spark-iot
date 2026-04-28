# Spark-IoT

ESP32-S3 (N16R8) IoT 控制器 — 通过 MQTT over WiFi 控制 WS2812 LED。

## 功能

- WS2812 可寻址 LED 控制（RMT 外设驱动）
- WiFi BLE 配网（基于 ESP-IDF `network_provisioning` 组件）
- MQTT JSON 指令控制
- LED 状态掉电保存（NVS）
- 三种灯效模式：静态、彩虹、呼吸
- 系统状态机 + LED 状态指示
- 重置按钮（长按 3 秒恢复出厂配网）

## 硬件

- ESP32-S3 N16R8（16MB Flash，8MB OPI PSRAM）
- WS2812 LED 接 GPIO 48
- 重置按钮接 GPIO 45（外部下拉，按下为高电平）

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

## 系统状态

设备启动后会经历以下状态，LED 指示灯会显示当前状态：

| 状态 | LED 颜色 | 动作 | 说明 |
|------|----------|------|------|
| 启动中 | 白色 | 快闪 | 系统初始化 |
| 检查配网 | 白色 | 常亮 | 检查是否已配网 |
| 未配网 | 蓝色 | 慢闪（1s） | 等待 BLE 配网 |
| BLE 启动中 | 蓝色 | 快闪（200ms） | BLE 配网服务启动 |
| 配网中 | 蓝色 | 呼吸灯 | 等待手机 App 配置 |
| 配网成功 | 绿色 | 快闪×3 | 配网完成 |
| WiFi 连接中 | 黄色 | 慢闪（500ms） | 连接路由器 |
| 已获取 IP | 绿色 | 常亮 | 网络就绪 |
| MQTT 连接中 | 青色 | 慢闪（500ms） | 连接 MQTT Broker |
| 正常运行 | — | 由 MQTT 控制 | 可接收指令 |
| 断线重连 | 橙色 | 慢闪（500ms） | WiFi/MQTT 断线 |
| 异常 | 红色 | 快闪（100ms） | 错误状态 |

## 重置按钮

长按 GPIO45 按钮 **3 秒**，LED 变红后设备自动重启，清除配网信息恢复出厂状态。
