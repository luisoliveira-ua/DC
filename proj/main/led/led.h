#pragma once

#include <stdint.h>

void led_init(void);
void led_on(void);
void led_off(void);
uint8_t get_led_state(void);
void led_set_color(uint8_t red, uint8_t green, uint8_t blue);