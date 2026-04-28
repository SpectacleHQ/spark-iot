#pragma once

#include "config.h"

void storage_init(void);
void storage_save_led(uint8_t r, uint8_t g, uint8_t b, led_mode_t mode);
void storage_load_led(uint8_t *r, uint8_t *g, uint8_t *b, led_mode_t *mode);
