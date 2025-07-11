#include "pico/stdlib.h"
#include "pins.h"
#include "leds.h"
#include "buttons.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"


int main() {
    led_init();
    butt_init();

    while (true) {
        static uint64_t prev_time = 0;
        if (time_us_64() < prev_time){
            // We have passed end of time?!
        }
        prev_time = time_us_64();
        
        // Leds: 30ms
        static uint64_t led_time = 0;
        if (led_time < prev_time){
            led_time = prev_time + 30000;
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
