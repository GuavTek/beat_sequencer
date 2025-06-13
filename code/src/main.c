#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "ws2812.h"
#include "pins.h"

#ifndef LED_DELAY_MS
#define LED_DELAY_MS 250
#endif

// Perform initialisation
int pico_led_init(void) {
#if defined(PICO_DEFAULT_LED_PIN)
    // A device like Pico that uses a GPIO for the LED will define PICO_DEFAULT_LED_PIN
    // so we can use normal GPIO functionality to turn the led on and off
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return PICO_OK;
#endif
}

// Turn the led on or off
void pico_set_led(bool led_on) {
#if defined(PICO_DEFAULT_LED_PIN)
    // Just set the GPIO on or off
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);
#endif
}

int main() {
    int rc = pico_led_init();
    hard_assert(rc == PICO_OK);
    ws2812_init(pio0, PIXEL, 800000);
    while (true) {
        static uint8_t temp;
        pico_set_led(true);
        sleep_ms(LED_DELAY_MS);
        put_pixel_rgb(temp, 0, 0);
        temp += 10;
        pico_set_led(false);
        sleep_ms(LED_DELAY_MS);
    }
}
