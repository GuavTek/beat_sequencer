#ifndef BUTTONS_H_
#define BUTTONS_H_

#include "pico/stdlib.h"

extern bool butts[80];
extern int8_t renc_increment;

void butt_update();

void butt_init();

#endif // BUTTONS_H_
