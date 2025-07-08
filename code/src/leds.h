#ifndef LEDS_H_
#define LEDS_H_

#include "pico/stdlib.h"

typedef union {
    struct {
        uint8_t g;
        uint8_t r;
        uint8_t b;
    };
    uint32_t data;
} pixel_t;

extern pixel_t leds[80];

int led_get_idx(uint8_t row, uint8_t col);

void led_set(uint8_t row, uint8_t col, uint8_t r, uint8_t g, uint8_t b);

void led_update();

void led_init();

#endif // LEDS_H_
