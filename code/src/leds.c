#include "leds.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "ws2812.h"
#include "pins.h"

pixel_t leds[80];
uint8_t led_channel;

int led_get_idx(uint8_t row, uint8_t col){
    uint8_t idx;
    // row 0, 3, 5 go backward
    if (row < 2){
        if (col >= 8) {
            return -1;
        }
        idx = 8;
        if (row == 0) {
            idx -= col+1;
        } else {
            idx += col;
        }
    } else {
        if (col >= 16) {
            return -1;
        }
        // 2 -> 16, 3 or 4 -> 48, 5 -> 80
        idx = ((row-1)|1) * 16;
        if (row & 0b0001) {
            idx -= col+1;
        } else {
            idx += col;
        }
    }
    return idx;
}

void led_set(uint8_t row, uint8_t col, uint8_t r, uint8_t g, uint8_t b){
    int8_t idx;
    idx = led_get_idx(row, col);
    if (idx >= 0){
        leds[idx].r = r;
        leds[idx].g = g;
        leds[idx].b = b;
    }
}

void led_update(){
    dma_channel_set_read_addr(led_channel, &leds[0], 1);
}

void led_init(){
    ws2812_init(pio0, PIXEL, 800000);
    // Figure out which sm the ws2812 library claimed
    uint8_t led_sm;
    for (uint8_t i = 0; i < 4; i++){
        if (pio_sm_is_claimed(pio0, i)){
            led_sm = i;
            break;
        }
    }
    // Set up DMA for the LED array
    led_channel = dma_claim_unused_channel(1);
    dma_channel_config config;
    config = dma_channel_get_default_config(led_channel);
    channel_config_set_read_increment(&config, 1);
    channel_config_set_write_increment(&config, 0);
    channel_config_set_dreq(&config, pio_get_dreq(pio0, led_sm, 1));
    channel_config_set_transfer_data_size(&config, DMA_SIZE_32);
    channel_config_set_bswap(&config, 1);
    channel_config_set_irq_quiet(&config, 1);
    channel_config_set_enable(&config, 1);
    dma_channel_configure(led_channel, &config, &pio0->txf[led_sm], NULL, 80, 0);
    // Clear leds
    for (uint8_t i = 0; i < 80; i++){
        leds[i].data = 0;
    }
    led_update();
}
