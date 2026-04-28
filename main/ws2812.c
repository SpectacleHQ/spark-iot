#include "ws2812.h"
#include "config.h"
#include "driver/rmt_tx.h"

/* WS2812 时序 (10MHz tick) */
#define T0H  4
#define T0L  8
#define T1H  8
#define T1L  4
#define RESET 50

static rmt_channel_handle_t channel = NULL;
static rmt_encoder_handle_t encoder = NULL;

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

void ws2812_set_color(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t data[3] = {g, r, b};
    rmt_symbol_word_t symbols[25];
    int idx = 0;

    for (int i = 0; i < 3; i++) {
        for (int bit = 7; bit >= 0; bit--) {
            if (data[i] & (1 << bit)) {
                symbols[idx] = (rmt_symbol_word_t){
                    .level0 = 1, .duration0 = T1H,
                    .level1 = 0, .duration1 = T1L,
                };
            } else {
                symbols[idx] = (rmt_symbol_word_t){
                    .level0 = 1, .duration0 = T0H,
                    .level1 = 0, .duration1 = T0L,
                };
            }
            idx++;
        }
    }
    symbols[idx] = (rmt_symbol_word_t){
        .level0 = 0, .duration0 = RESET,
        .level1 = 0, .duration1 = 0,
    };

    rmt_transmit_config_t tx_cfg = {};
    rmt_transmit(channel, encoder, symbols, sizeof(symbols), &tx_cfg);
    rmt_tx_wait_all_done(channel, 1000);
}
