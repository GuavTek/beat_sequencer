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
int8_t steps[NUM_CHANNELS][16];
uint8_t current_idx[NUM_CHANNELS];
uint8_t max_idx[NUM_CHANNELS];
uint8_t velocities[4] = {0, 42, 84, 127};
bool step_state;

void clamp_uint8(uint8_t* target, int32_t delta);
void midi_rx();
void led_set_step(uint8_t row, uint8_t col, int8_t vel);
void led_set_end(uint8_t row, uint8_t col);
void renc_change(int32_t delta);

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

    // Initialize arrays
    for (uint8_t i = 0; i < NUM_CHANNELS; i++){
        max_idx[i] = 15;
        for (uint8_t j = 0; j < 16; j++){
            steps[i][j] = -1;
        }
    }

    led_set_step(0, 0, velocities[0]);
    led_set_step(0, 1, velocities[1]);
    led_set_step(1, 0, velocities[2]);
    led_set_step(1, 1, velocities[3]);

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

            // Has rotary encoder changed?
            static uint32_t renc_mult;
            static bool renc_dir;
            if (renc_increment != 0){
                if (renc_dir == (renc_increment > 0)){
                    renc_mult += 0x200;
                } else {
                    // Direction changed, reset mult
                    renc_mult = 0x200;
                    renc_dir = renc_increment > 0;
                }
                renc_change(renc_increment * (renc_mult >> 9));
                renc_increment = 0;
            }
            if (renc_mult > 0){
                renc_mult -= 1 + (renc_mult >> 9);
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
                                led_set_step(j+2, idx, steps[j][idx]);
                                if (idx == 0){
                                    idx = max_idx[j];
                                } else {
                                    idx--;
                                }
                                led_set_step(j+2, idx, steps[j][idx]);
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
                            // Holding button 15 selects sequence end
                            // Clear the previous end LED
                            led_set_step(r+2, max_idx[r]+1, steps[r][max_idx[r]+1]);
                            // Set new end step
                            max_idx[r] = c;
                            led_set_end(r+2, c+1);
                            // TODO? maybe it should complete the previous length
                            // Reset pointer if it has passed the new end step
                            if (current_idx[r] > c){
                                current_idx[r] = 0;
                            }
                            // Reset LEDs after new end step
                            for (uint8_t i = c+2; i < 16; i++){
                                led_set_step(r+2, i, steps[r][i]);
                            }
                        } else {
                            // Set new velocity on step?
                            if (butts[0]) {
                                steps[r][c] = velocities[0];
                            } else if (butts[1]) {
                                steps[r][c] = velocities[1];
                            } else if (butts[8]) {
                                steps[r][c] = velocities[2];
                            } else if (butts[9]) {
                                steps[r][c] = velocities[3];
                            } else {
                                // Default toggling previous velocity on or off
                                steps[r][c] = -steps[r][c]-1;
                            }
                            led_set_step(r+2, c, steps[r][c]);
                        }
                    }
                }
                prev_butt[i] = butts[i];
            }
        }

        if (step_time < prev_time){
            step_time = prev_time + current_uspb;
            step_state = !step_state;
            // Flash end LED if that step is enabled
            static uint8_t div_step;
            div_step += 1;
            div_step &= 0xf;
            if (div_step == 0){
                for (uint8_t i = 0; i < 4; i++){
                    uint8_t c = max_idx[i]+1;
                    if (steps[i][c] < 0) continue;
                    led_set_step(i+2, c, steps[i][c]);
                }
            } else if (div_step == 0x8){
                for (uint8_t i = 0; i < 4; i++){
                    uint8_t c = max_idx[i]+1;
                    led_set_end(i+2, c);
                }
            }
            if (step_state){
                if (current_playing){
                    led_set(0,7,0,64,0);
                    current_playing = 3;
                    // Update LED matrix
                    for(uint8_t i = 0; i < 4; i++){
                        uint8_t idx = current_idx[i];
                        bool active = steps[i][idx] >= 0;
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
                        led_set_step(i+2, idx, steps[i][idx]);
                    }
                    // Send midi note on
                    if (tud_midi_mounted()){
                        uint8_t buff[3*NUM_CHANNELS];
                        uint8_t len = 0;
                        for (uint8_t i = 0; i < NUM_CHANNELS; i++){
                            if (steps[i][current_idx[i]] >= 0){
                                buff[len++] = 0x99;     // Note on
                                buff[len++] = 0x20+i;   // key
                                buff[len++] = steps[i][current_idx[i]] & 0x7f;     // velocity
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
                            if(steps[i][current_idx[i]] >= 0){
                                buff[len++] = 0x89;     // Note off
                                buff[len++] = 0x20+i;   // key
                                buff[len++] = 0x0;      // velocity
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

void clamp_uint8(uint8_t* target, int32_t delta){
    if (*target + delta > 255) {
        *target = 255;
    } else if (*target + delta < 0) {
        *target = 0;
    } else {
        *target += delta;
    }
}

void led_set_step(uint8_t row, uint8_t col, int8_t vel){
    bool active = vel >= 0;
    if (active){
        led_set(row, col, 64, vel, 0);
    } else {
        led_set(row, col, 0, 0, 0);
    }
}


void led_set_end(uint8_t row, uint8_t col){
    led_set(row, col, 0, 0, 64);
}

void renc_change(int32_t delta){
    if (butts[0]) {
        // Change velocity preset
        clamp_uint8(&velocities[0], delta);
        velocities[0] &= 0x7f;
        led_set_step(0, 0, velocities[0]);
    } else if (butts[1]) {
        // Change velocity preset
        clamp_uint8(&velocities[1], delta);
        velocities[1] &= 0x7f;
        led_set_step(0, 1, velocities[1]);
    } else if (butts[8]) {
        // Change velocity preset
        clamp_uint8(&velocities[2], delta);
        velocities[2] &= 0x7f;
        led_set_step(1, 0, velocities[2]);
    } else if (butts[9]) {
        // Change velocity preset
        clamp_uint8(&velocities[3], delta);
        velocities[3] &= 0x7f;
        led_set_step(1, 1, velocities[3]);
    } else {
        // Default to changing tempo
        current_bpm += delta;
        if (current_bpm < 10){
            current_bpm = 10;
        }
        current_uspb = 60000000 / current_bpm;
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
