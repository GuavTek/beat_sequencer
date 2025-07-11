#include "pico/stdlib.h"
#include "pins.h"
#include "leds.h"
#include "buttons.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

int32_t renc_increment;

int main() {
    led_init();
    butt_init();

    // Initialize rotary encoder
    gpio_init(RENC_A);
    gpio_init(RENC_B);
    while (true) {
        static uint64_t prev_time;
        for (uint8_t i = 0; i < 80; i++){
            uint8_t r;
            uint8_t c;
            if (i < 16){
                r = i >> 3;
                c = i & 0b111;
            } else {
                r = (i >> 4)+1;
                c = i & 0b1111;
            }
            if (butts[i]){
                led_set(r, c, 64, 0, 0);
            } else {
                led_set(r, c, 0, 0, 0);
            }
        }
        if (time_us_64() < prev_time){
            // We have passed end of time?!
        }
        prev_time = time_us_64();
        
        // Leds: 30ms
        static uint64_t led_time = 0;
        if (led_time < prev_time){
            led_time = prev_time + 30000;
            if (renc_increment > 0){
                renc_increment = 0;
                led_set(2,2,64,0,0);
            }
            if (renc_increment < 0){
                renc_increment = 0;
                led_set(2,2,0,0,64);
            }
            led_update();
        }

        // Buttons: 1ms
        static uint64_t butt_time = 0;
        if (butt_time < prev_time){
            butt_time = prev_time + 1000;
            butt_update();
        }
    }
}
