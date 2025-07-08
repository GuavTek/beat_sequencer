#include "pico/stdlib.h"
#include "pins.h"
#include "leds.h"

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 250
#endif

int main() {
    led_init();
    while (true) {
        static uint8_t temp;
        for (uint8_t i = 0; i < 16; i++){
            uint8_t sel;
            sel = (temp+i)%3;
            if (sel == 0){
                led_set(0, i, 64, 0, 0);
                led_set(1, i, 64, 0, 0);
                led_set(2, i, 64, 0, 0);
                led_set(3, i, 64, 0, 0);
                led_set(4, i, 64, 0, 0);
                led_set(5, i, 64, 0, 0);
            } else if (sel == 1) {
                led_set(0, i, 0, 64, 0);
                led_set(1, i, 0, 64, 0);
                led_set(2, i, 0, 64, 0);
                led_set(3, i, 0, 64, 0);
                led_set(4, i, 0, 64, 0);
                led_set(5, i, 0, 64, 0);
            } else if (sel == 2) {
                led_set(0, i, 0, 0, 64);
                led_set(1, i, 0, 0, 64);
                led_set(2, i, 0, 0, 64);
                led_set(3, i, 0, 0, 64);
                led_set(4, i, 0, 0, 64);
                led_set(5, i, 0, 0, 64);
            }
        }
        temp -= 1;
        led_update();
        sleep_ms(LED_DELAY_MS);
    }
}
