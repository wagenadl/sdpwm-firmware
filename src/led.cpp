// led.cpp

#include "led.h"

void setup_led() {
  gpio_init(GPIO_LED);
  gpio_set_dir(GPIO_LED, GPIO_OUT);
  ledon();
}


void blink(int n) {
  for (int q=0; q<3; q++) {
    for (int k=0; k<n; k++) {
      ledon();
      sleep_us(100000);
      ledoff();
      sleep_us(100000);
    }
    sleep_us(500000);
  }
  sleep_us(1000000);
}

void ledset(bool x) {
  if (x)
    ledon();
  else
    ledoff();
}

void ledon() {
  // gpio_set_mask(1<<GPIO_LED);
}

void ledoff() {
  // gpio_clr_mask(1<<GPIO_LED);
}
