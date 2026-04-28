/**
 * @file ws2812.c
 * @brief WS2812 驱动实现 — 基于 RMT 外设的时序编码
 *
 * RMT 时钟配置为 10MHz（100ns/tick），通过高低电平持续时间
 * 编码 WS2812 的 T0H/T0L/T1H/T1L 信号。
 */

#include "ws2812.h"
#include "config.h"
#include "driver/rmt_tx.h"

/* ── WS2812 时序参数（单位：tick，10MHz 下 1tick=100ns）── */
#define T0H   4   /**< 0 码高电平：400ns */
#define T0L   8   /**< 0 码低电平：800ns */
#define T1H   8   /**< 1 码高电平：800ns */
#define T1L   4   /**< 1 码低电平：400ns */
#define RESET 50  /**< 复位信号：>50μs（50 ticks × 100ns = 5μs，实际由连续发送保证） */

/** RMT 发送通道句柄 */
static rmt_channel_handle_t channel = NULL;

/** RMT 拷贝编码器句柄（将 symbol 数组转为 RMT 信号） */
static rmt_encoder_handle_t encoder = NULL;

/**
 * @brief 初始化 RMT 发送通道和编码器
 *
 * 配置参数：
 * - GPIO: LED_GPIO（默认 48）
 * - 时钟源: RMT_CLK_SRC_DEFAULT
 * - 分辨率: 10MHz
 * - 内存块: 64 symbol
 * - 发送队列深度: 1
 */
void ws2812_init(void)
{
    rmt_tx_channel_config_t tx_cfg = {
        .gpio_num = LED_GPIO,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = RMT_RESOLUTION_HZ,
        .mem_block_symbols = 64,
        .trans_queue_depth = 1,
    };
    rmt_new_tx_channel(&tx_cfg, &channel);

    rmt_copy_encoder_config_t enc_cfg = {};
    rmt_new_copy_encoder(&enc_cfg, &encoder);

    rmt_enable(channel);
}

/**
 * @brief 设置 LED 颜色并发送
 *
 * 将 RGB 转为 GRB 字节序（WS2812 协议），逐 bit 编码为 RMT symbol，
 * 末尾附加复位信号，然后通过 RMT 发送并等待完成。
 *
 * @param r 红色分量（0-255）
 * @param g 绿色分量（0-255）
 * @param b 蓝色分量（0-255）
 */
void ws2812_set_color(uint8_t r, uint8_t g, uint8_t b)
{
    /* WS2812 数据顺序为 GRB */
    uint8_t data[3] = {g, r, b};
    rmt_symbol_word_t symbols[25];  /* 24 bit 数据 + 1 个复位 */
    int idx = 0;

    /* 将每个字节的 8 个 bit 编码为 RMT symbol */
    for (int i = 0; i < 3; i++) {
        for (int bit = 7; bit >= 0; bit--) {
            if (data[i] & (1 << bit)) {
                /* 1 码：高 800ns，低 400ns */
                symbols[idx] = (rmt_symbol_word_t){
                    .level0 = 1, .duration0 = T1H,
                    .level1 = 0, .duration1 = T1L,
                };
            } else {
                /* 0 码：高 400ns，低 800ns */
                symbols[idx] = (rmt_symbol_word_t){
                    .level0 = 1, .duration0 = T0H,
                    .level1 = 0, .duration1 = T0L,
                };
            }
            idx++;
        }
    }

    /* 复位信号：低电平 >50μs */
    symbols[idx] = (rmt_symbol_word_t){
        .level0 = 0, .duration0 = RESET,
        .level1 = 0, .duration1 = 0,
    };

    rmt_transmit_config_t tx_cfg = {};
    rmt_transmit(channel, encoder, symbols, sizeof(symbols), &tx_cfg);
    rmt_tx_wait_all_done(channel, 1000);
}
