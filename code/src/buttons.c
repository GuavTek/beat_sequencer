#include "buttons.h"
#include "pins.h"

bool butts[80];
uint8_t current_col = 0;
int8_t renc_increment;

uint8_t columns[] = {
    PM_C1,
    PM_C2,
    PM_C3,
    PM_C4,
    PM_C5,
    PM_C6,
    PM_C7,
    PM_C8
};

uint8_t rows[] = {
    PM_R1,
    PM_R2,
    PM_R3,
    PM_R4,
    PM_R5,
    PM_R6,
    PM_R7,
    PM_R8,
    PM_R9,
    PM_R10
};

// Set column low to select, set as input to deselect
void butt_col_select(uint8_t col){
    // Set all columns as input to deselect
    gpio_set_dir_masked(
        (1 << PM_C1)|
        (1 << PM_C2)|
        (1 << PM_C3)|
        (1 << PM_C4)|
        (1 << PM_C5)|
        (1 << PM_C6)|
        (1 << PM_C7)|
        (1 << PM_C8)
        ,0
    );
    if (col >= 8){
        return;
    }
    // Set the selected column as output and set it low
    gpio_set_dir(columns[col], 1);
    gpio_put(columns[col], 0);
}

/*
R10
R9
R1  R5
R2  R6
R3  R7
R4  R8
C1 C2 C3 C4....
*/
// Read rows for state
void butt_update(){
    butts[16+current_col] = !gpio_get(PM_R1);
    butts[32+current_col] = !gpio_get(PM_R2);
    butts[48+current_col] = !gpio_get(PM_R3);
    butts[64+current_col] = !gpio_get(PM_R4);
    butts[24+current_col] = !gpio_get(PM_R5);
    butts[40+current_col] = !gpio_get(PM_R6);
    butts[56+current_col] = !gpio_get(PM_R7);
    butts[72+current_col] = !gpio_get(PM_R8);
    butts[8+current_col] = !gpio_get(PM_R9);
    butts[0+current_col] = !gpio_get(PM_R10);
    // Select next column
    current_col++;
    current_col &= 0b111;
    butt_col_select(current_col);

    // Rotary encoder
    static bool renc_state;
    static uint8_t renc_debounce_a;
    static uint8_t renc_debounce_b;
    renc_debounce_a = (renc_debounce_a << 1) | gpio_get(RENC_A);
    renc_debounce_b = (renc_debounce_b << 1) | gpio_get(RENC_B);
    renc_debounce_a &= 0xf;
    renc_debounce_b &= 0xf;
    if ((renc_debounce_a == 0xf) && !renc_state){
        // Positive edge
        if (renc_debounce_b == 0xf){
            renc_state = 1;
            renc_increment += 1;
        } else if (renc_debounce_b == 0) {
            renc_state = 1;
            renc_increment -= 1;
        }
    } else if ((renc_debounce_a == 0) && renc_state){
        // Negative edge
        renc_state = 0;
    }
}

void butt_init(){
    gpio_init(PM_R1);
    gpio_init(PM_R2);
    gpio_init(PM_R3);
    gpio_init(PM_R4);
    gpio_init(PM_R5);
    gpio_init(PM_R6);
    gpio_init(PM_R7);
    gpio_init(PM_R8);
    gpio_init(PM_R9);
    gpio_init(PM_R10);
    gpio_init(PM_C1);
    gpio_init(PM_C2);
    gpio_init(PM_C3);
    gpio_init(PM_C4);
    gpio_init(PM_C5);
    gpio_init(PM_C6);
    gpio_init(PM_C7);
    gpio_init(PM_C8);
    butt_col_select(current_col);
    // Initialize rotary encoder
    gpio_init(RENC_A);
    gpio_init(RENC_B);
}

