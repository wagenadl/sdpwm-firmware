// led.h

#ifndef LED_H

#define LED_H

#include "pico/stdlib.h"
#include "pins.h"

void setup_led();
void blink(int n);
void ledon();
void ledoff();
void ledset(bool);

#endif
