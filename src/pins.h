// pins.h

#ifndef PINS_H

#define PINS_H

constexpr int GPIO_PWM_AO0 = 6;
constexpr int GPIO_PWM_AOREF = 10;
constexpr int GPIO_LED = 25;
constexpr int PWM_SLICE_AO = 3; // for pin 6
constexpr int PWM_SLICE_LED = 4; // for pin 25 (the on-board LED)
constexpr int PWM_SLICE_REF = 5; // for pin 10

#endif
