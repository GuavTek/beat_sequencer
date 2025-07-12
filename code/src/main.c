#include "pico/stdlib.h"
#include "pins.h"
#include "leds.h"
#include "buttons.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "tusb.h"

#define NUM_CHANNELS 4

uint32_t blink_time = 1000000;

int16_t current_bpm = 120;
uint64_t current_uspb = 60000000/120; // µs per beat
uint8_t current_playing = 0;
bool steps[NUM_CHANNELS][16];
uint8_t current_idx[NUM_CHANNELS];
uint8_t max_idx[NUM_CHANNELS];
bool step_state;

void midi_rx();

// TODO: sequencer
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

    // Initialize array
    for (uint8_t i = 0; i < NUM_CHANNELS; i++){
        max_idx[i] = 15;
    }
    

    while (true) {
        static uint64_t step_time = 0;
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

            // TODO: velocity multiplier
            // Has rotary encoder changed?
            if (renc_increment != 0){
                current_bpm += renc_increment;
                if (current_bpm < 10){
                    current_bpm = 10;
                }
                current_uspb = 60000000 / current_bpm;
                renc_increment = 0;
            }
            
            // Check buttons
            static bool prev_butt[80];
            for (uint8_t i = 0; i < 80; i++){
                if(butts[i] && !prev_butt[i]){
                    // posedge
                    if (i == 7){
                        // Play/stop button
                        current_playing = !current_playing;
                        //step_time = 0;
                        //step_state = 0;
                        if (!current_playing){
                            // Clear step led
                            for (uint8_t j = 0; j < 4; j++){
                                uint8_t idx = current_idx[j];
                                led_set(j+2, idx, steps[j][idx]*64, 0, 0);
                                if (idx == 0){
                                    idx = max_idx[j];
                                } else {
                                    idx--;
                                }
                                led_set(j+2, idx, steps[j][idx]*64, 0, 0);
                            }
                            for (uint8_t j = 0; j < NUM_CHANNELS; j++){
                                // Send note offs
                                if (tud_midi_mounted()){
                                    uint8_t buff[3];
                                    buff[0] = 0x89;
                                    buff[1] = 0x20+j;
                                    buff[2] = 0x0;
                                    tud_midi_stream_write(0, buff, 3);
                                }
                                // Reset step index
                                current_idx[j] = 0;
                            }
                        }
                    } else if (i > 15){
                        // Step section
                        uint8_t r;
                        uint8_t c;
                        r = (i >> 4)-1;
                        c = i & 0b1111;
                        if (butts[15]){
                            led_set(r+2, max_idx[r]+1, 0, 0, 0);
                            max_idx[r] = c;
                            led_set(r+2, c+1, 0, 0, 64);
                            for (uint8_t j = c+2; j < 16; j++){
                                steps[r][j] = 0;
                                led_set(r+2, j, 0, 0, 0);
                            }
                            if (current_idx[r] > c){
                                current_idx[r] = 0;
                            }
                        } else if (max_idx[r] >= c) {
                            steps[r][c] = !steps[r][c];
                            led_set(r+2, c, steps[r][c] * 64, 0, 0);
                        }
                    }
                }
                prev_butt[i] = butts[i];
            }
        }

        if (step_time < prev_time){
            step_time = prev_time + current_uspb;
            step_state = !step_state;
            if (step_state){
                if (current_playing){
                    led_set(0,7,0,64,0);
                    current_playing = 3;
                    // Update LED matrix
                    for(uint8_t i = 0; i < 4; i++){
                        uint8_t idx = current_idx[i];
                        bool active = steps[i][idx];
                        if(active){
                            led_set(i+2, idx, 0, 63, 63);
                        } else {
                            led_set(i+2, idx, 63, 63, 63);
                        }
                        // Reset the previous led
                        if (idx == 0){
                            if (max_idx[i] == 0){
                                // Only one step
                                continue;
                            }
                            idx = max_idx[i];
                        } else {
                            idx--;
                        }
                        led_set(i+2, idx, steps[i][idx]*64, 0, 0);
                    }
                    // Send midi note on
                    if (tud_midi_mounted()){
                        uint8_t buff[3*NUM_CHANNELS];
                        uint8_t len = 0;
                        for (uint8_t i = 0; i < NUM_CHANNELS; i++){
                            if (steps[i][current_idx[i]]){
                                buff[len++] = 0x99;
                                buff[len++] = 0x20+i;
                                buff[len++] = 0x40;
                            }
                            tud_midi_stream_write(0,buff, len);
                        }
                    }
                } else {
                    led_set(0,7,64,0,0);
                }
            } else {
                led_set(0,7,0,0,0);
                if (current_playing > 1){
                    if (tud_midi_mounted()){
                        uint8_t buff[3*NUM_CHANNELS];
                        uint8_t len = 0;
                        for (uint8_t i = 0; i < NUM_CHANNELS; i++){
                            if(steps[i][current_idx[i]]){
                                buff[len++] = 0x89;
                                buff[len++] = 0x20+i;
                                buff[len++] = 0x0;
                            }
                        }
                        tud_midi_stream_write(0, buff, len);
                    }
                    for (uint8_t i = 0; i < NUM_CHANNELS; i++){                        
                        current_idx[i]++;
                        if (current_idx[i] > max_idx[i]){
                            current_idx[i] = 0;
                        }
                    }
                }
            }
        }

        static uint64_t blink_timer = 0;
        if (blink_timer < prev_time) {
            blink_timer = prev_time + blink_time;
            gpio_put(PICO_DEFAULT_LED_PIN, !gpio_get_out_level(PICO_DEFAULT_LED_PIN));
        }

        // USB
        tud_task();
        midi_rx();

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
