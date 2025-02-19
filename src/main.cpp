#include "pico/stdlib.h"
#include <hardware/clocks.h>
#include <stdlib.h>
#include "hardware/pwm.h"
#include "hardware/dma.h"

#include <stdio.h>
#include <string.h>

#include "pins.h"
#include "reset.h"
#include "usb.h"
#include "core1.h"
#include "ringbuffer.h"
#include "led.h"

extern volatile int test;
extern volatile int test2;

void report() {
  Ringbuffer::lock();
  int t = test;
  int t2 = test2;
  int f = Core1::fremaining;
  int k = Core1::kremaining;
  int val = Core1::value;
  int pwm = Core1::pwm;
  int32_t fp = (int32_t)Core1::fillptr;
  int w = Ringbuffer::writeoffset;
  int r = Ringbuffer::readoffset;
  int32_t wp = (int32_t)Ringbuffer::writeptr();
  int32_t rp =  (int32_t)Ringbuffer::readptr();
  Ringbuffer::unlock();
  USB::reportint("cpuid", sio_hw->cpuid);
  USB::reportint("test", t);
  USB::reportint("test2", t2);
  USB::reportint("f", f);
  USB::reportint("k", k);
  USB::reportint("value", val);
  USB::reportint("pwm", pwm);
  USB::reportint("fill", fp);
  USB::reportint("write", w);
  USB::reportint("read", r);
  USB::reportint("writeptr", wp);
  USB::reportint("readptr", rp);
}  

void parse_and_execute(char *input) {
  char *cmd = strtok(input, " ");
  USB::sendtext(cmd);
  char *arg1p = strtok(0, " ");
  int arg1 = arg1p ? atoi(arg1p) : 0;
  if (strcmp(cmd, "pwm")==0)
    Core1::setMode(Core1::ModulationMode::PWM);
  else if (strcmp(cmd, "sdm")==0)
    Core1::setMode(Core1::ModulationMode::SDM);
  else if (strcmp(cmd, "sdpwm")==0)
    Core1::setMode(Core1::ModulationMode::SDPWM);
  else if (strcmp(cmd, "logk")==0) 
    Core1::setPWMClock(arg1);
  else if (strcmp(cmd, "period")==0) 
    Core1::setPeriod(arg1);
  else if ((*cmd >= '0' && *cmd <='9') || *cmd=='-') {
    Ringbuffer::lock();
    if (Ringbuffer::full()) {
      Ringbuffer::unlock();
      USB::sendtext("Overrun");
    } else {
      *Ringbuffer::writeptr() = atoi(cmd);
      Ringbuffer::writeoffset ++;
      Ringbuffer::unlock();
    }
  } else {
    USB::sendtext("Command?");
  } 
  report();
}


int main() {
  //  set_sys_clock_pll(1440000000, 4, 4);
  
  setup_led();
  ledoff();
  USB::initialize();
  while (!USB::connected()) {
    Reset::reset_on_bootsel();
    USB::poll();
  }

  Ringbuffer::initialize();
  Core1::setPWMClock(5);
  Core1::setPeriod(62); // 48 kHz, approx
  Core1::setMode(Core1::ModulationMode::PWM);
  USB::sendtext("connected");
  USB::poll();
  USB::sendtext("starting");
  Core1::start();
  USB::sendtext("started");

  bool connected = false;

  uint32_t t0 = time_us_32() >> 10;
  while (true) {
    Reset::reset_on_bootsel();
    USB::poll();
    if (USB::connected()) {
      if (!connected) {
        connected = true;
        USB::sendtext("sdpwm 1.0 - Daniel Wagenaar 2025");
      }
      char *input = USB::receivetext();
      uint32_t t = time_us_32() >> 10;
      if (input) {
        parse_and_execute(input);
        t0 = t;
      } else if (t - t0 > 100) {
        report();
        t0 = t;
      }
    } else {
      if (connected) {
        Core1::setPWMClock(5);
        Core1::setPeriod(62);
        Core1::setMode(Core1::ModulationMode::PWM);
      }
    }
  }
  return 0;
}
