/**
 * @file state_machine.h
 * @brief 系统状态机接口
 *
 * 管理设备生命周期中的所有状态转换，并通过 LED 指示当前状态。
 * 其他模块通过 state_set() 报告状态变化，通过 state_get() 查询当前状态。
 */

#pragma once

/**
 * @brief 系统状态枚举
 */
typedef enum {
    SYS_BOOTING,           /**< 系统启动中（白色快闪） */
    SYS_CHECKING_PROV,     /**< 检查配网状态（白色常亮） */
    SYS_NOT_PROVISIONED,   /**< 未配网，等待进入 BLE 配网（蓝色慢闪） */
    SYS_PROV_STARTING,     /**< BLE 配网服务启动中（蓝色快闪） */
    SYS_PROVISIONING,      /**< BLE 配网中，等待手机 App（蓝色呼吸） */
    SYS_PROV_SUCCESS,      /**< 配网成功（绿色快闪×3） */
    SYS_WIFI_CONNECTING,   /**< WiFi 连接中（黄色慢闪） */
    SYS_WIFI_GOT_IP,       /**< 已获取 IP（绿色常亮） */
    SYS_MQTT_CONNECTING,   /**< MQTT 连接中（青色慢闪） */
    SYS_RUNNING,           /**< 正常运行，LED 由 MQTT 控制 */
    SYS_RECONNECTING,      /**< 断线重连中（橙色慢闪） */
    SYS_ERROR,             /**< 异常状态（红色快闪） */
} sys_state_t;

/**
 * @brief 初始化状态机
 *
 * 配置重置按钮 GPIO，创建状态机任务（LED 指示 + 按钮检测）。
 */
void state_machine_init(void);

/**
 * @brief 设置系统状态
 *
 * 由 wifi_prov、mqtt_ctrl 等模块在事件回调中调用。
 *
 * @param new_state 目标状态
 */
void state_set(sys_state_t new_state);

/**
 * @brief 获取当前系统状态
 *
 * 由 led_mode 等模块查询以决定行为。
 *
 * @return 当前状态
 */
sys_state_t state_get(void);
