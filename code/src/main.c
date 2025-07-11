#include "pico/stdlib.h"
#include "pins.h"
#include "leds.h"
#include "buttons.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "tusb.h"

uint32_t blink_time = 1000000;

void midi_rx();

int main() {
    set_sys_clock_khz(120000, true);
    led_init();
    butt_init();

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, 1);

    tusb_rhport_init_t dev_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO
    };
    tusb_init(BOARD_TUD_RHPORT, &dev_init);

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

        static uint64_t blink_timer = 0;
        if (blink_timer < prev_time) {
            blink_timer = prev_time + blink_time;
            gpio_put(PICO_DEFAULT_LED_PIN, !gpio_get_out_level(PICO_DEFAULT_LED_PIN));
        }

        // USB
        tud_task();
        midi_rx();

        static uint64_t test_time = 0;
        if (test_time < prev_time){
            test_time = prev_time + 1000000;
            if (tud_midi_mounted()){
                uint8_t buff[32];
                static bool state;
                if (state) {
                    buff[0] = 0x99; // Note on, channel 10
                } else {
                    buff[0] = 0x89; // Note off, channel 10
                }
                buff[1] = 0x20;
                buff[2] = 0x40;
                tud_midi_stream_write(0, buff, 3);
                state = !state;
            }
        }
    }
}

// TODO: do we need MIDI input? other than clearing buffer
void midi_rx(){
    uint32_t length;
    uint8_t buff[32];
    while ( tud_midi_available() ){
        length = tud_midi_stream_read(buff, 32);
    }
}


// Invoked when device is mounted
void tud_mount_cb(void){
	blink_time = 100000;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void){
	blink_time = 100000;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en){
	(void) remote_wakeup_en;
	blink_time = 1000000;
}

// Invoked when device is unmounted
void tud_umount_cb(void){
	blink_time = 1000000;
}
