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

bool paused = false;
bool binmode = false;
uint32_t pauseoffset = 0;

void playbinary(int16_t const *dat, int len) {
  Ringbuffer::lock();
  for (int k=0; k<len; k++) {
    while (Ringbuffer::full()) {
      Ringbuffer::unlock();
      sleep_us(1000);
      Ringbuffer::lock();
    }
    *Ringbuffer::writeptr() = dat[k];
    Ringbuffer::writeoffset ++;    
  }
  Ringbuffer::unlock();
}

void handlebinary(int16_t const *dat, int len) {
  for (int k=0; k<len; k++) {
    if (dat[k]==0xffff8080) {
      if (k)
        playbinary(dat, k);
      binmode = false;
      int n = 2 * (len - 1 - k);
      if (n > 0) {
        memcpy(USB::inbuffer, (uint8_t const *)(dat + k + 1), n);
        USB::inidx = n;
      }
      return;
    }
  } 
  playbinary(dat, len);
}

void parse_and_execute(char *input) {
  char c = *input;
  if (c=='-' || (c>='0' && c<='9')) {
    Ringbuffer::lock();
    while (Ringbuffer::full()) {
      Ringbuffer::unlock();
      if (paused) {
        USB::sendtext("Overrun");
        return;
      }
      sleep_us(1000);
      Ringbuffer::lock();
    } 

    if (paused) {
      *Ringbuffer::accessptr(pauseoffset++) = atoi(input);
    } else {	
      *Ringbuffer::writeptr() = atoi(input);
      Ringbuffer::writeoffset ++;
    }
    Ringbuffer::unlock();
    return;
  }
  
  char *cmd = strtok(input, " ");
  USB::sendtext(cmd);
  char *arg1p = strtok(0, " ");
  int arg1 = arg1p ? atoi(arg1p) : 0;
  if (strcmp(cmd, "bin")==0) {
    binmode = true;
  } else if (strcmp(cmd, "pause")==0) {
    paused = true;
    pauseoffset = Ringbuffer::writeoffset;
  } else if (strcmp(cmd, "go")==0) {
    paused = false;
    Ringbuffer::writeoffset = pauseoffset;
  } else if (strcmp(cmd, "pwm")==0)
    Core1::setMode(Core1::ModulationMode::PWM);
  else if (strcmp(cmd, "sdm")==0)
    Core1::setMode(Core1::ModulationMode::SDM);
  else if (strcmp(cmd, "sdpwm")==0)
    Core1::setMode(Core1::ModulationMode::SDPWM);
  else if (strcmp(cmd, "logk")==0) // log pwm period in units of base clock
    Core1::setPWMClock(arg1);
  else if (strcmp(cmd, "period")==0) // sample period in units of pwm period
    Core1::setPeriod(arg1);
  else if (strcmp(cmd, "over")==0) // oversampling factor
    Core1::setOver(arg1);
  else 
    USB::sendtext("Command?");
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
  int16_t binbuffer[32];
  paused = false;
  uint32_t t0 = time_us_32() >> 10;
  while (true) {
    Reset::reset_on_bootsel();
    USB::poll();
    if (USB::connected()) {
      if (!connected) {
        binmode = false;
	paused = false;
        connected = true;
        USB::sendtext("sdpwm 1.0 - Daniel Wagenaar 2025");
      }
      if (binmode) {
        bool err;
        USB::receivebinary((uint8_t*)binbuffer, 64, false, err);
        handlebinary(binbuffer, 32);
      } else {
        char *input = USB::receivetext();
        uint32_t t = time_us_32() >> 10;
        if (input) {
          parse_and_execute(input);
          t0 = t;
        } else if (t - t0 > 100) {
          // report();
          t0 = t;
        }
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
