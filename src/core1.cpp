#include <stdint.h>
#include <stdlib.h>
#include <hardware/pwm.h>
#include <hardware/dma.h>
#include <pico/multicore.h>
#include <hardware/gpio.h>

#include "usb.h"
#include "pins.h"
#include "ringbuffer.h"
#include "led.h"
#include "core1.h"

volatile int test = 0xc0ffee;
volatile int test2 = 0xc0ffee;

namespace Core1 {
  //////////////////////////////////////////////////////////////////////
  // Constants
  constexpr int32_t KILO = 1000;
  constexpr int32_t MEGA = 1000 * KILO;
  constexpr int32_t FCLOCK_HZ = 125 * MEGA; // base clock rate of PWM
  constexpr int32_t logK_MIN = 4; // i.e., K=16, i.e., 96/16 = 6 MHz mod freq
  constexpr int32_t logK_MAX = 12; // i.e., K=4096, i.e., 23.4 kHz mod freq
  constexpr int32_t DMABUFSIZE = 1024; // Note that this is less than max(K)

  //////////////////////////////////////////////////////////////////////
  // PWM period and baseline reference
  int32_t logK = 0; // number of bits of pwm to use
  int32_t K; // highest level to support
  int32_t KZERO; // baseline PWM value
  int32_t REF_NOM;
  int32_t REF_DENOM;

  void set_logK(int lk) {
    if (lk < logK_MIN)
      lk = logK_MIN;
    logK = lk;
    K = 1 << logK;
    KZERO = 1 << (logK - 1);
    pwm_set_wrap(PWM_SLICE_AO, K - 1);
    pwm_set_gpio_level(GPIO_PWM_AO0, KZERO);    
    pwm_set_gpio_level(GPIO_PWM_AO1, KZERO);    
  }

  void set_offset(int nom, int denom) {
    REF_NOM = nom;
    REF_DENOM = denom;
    pwm_set_wrap(PWM_SLICE_REF, REF_DENOM - 1);
    pwm_set_gpio_level(GPIO_PWM_AOREF, REF_NOM);
  }

  void init_pwm() {
    gpio_set_function(GPIO_PWM_AO0, GPIO_FUNC_PWM);
    gpio_set_function(GPIO_PWM_AO1, GPIO_FUNC_PWM);
    pwm_set_output_polarity(PWM_SLICE_AO, true, true);
    gpio_set_function(GPIO_PWM_AOREF, GPIO_FUNC_PWM);
    pwm_set_clkdiv_int_frac(PWM_SLICE_AO, 1, 0);
    pwm_set_clkdiv_int_frac(PWM_SLICE_REF, 1, 0);

    set_offset(17, 40);
    set_logK(5);
  
    pwm_set_enabled(PWM_SLICE_AO,  true);
    pwm_set_enabled(PWM_SLICE_REF, true);
  }

  
  //////////////////////////////////////////////////////////////////////
  // DMA buffer
  uint32_t *dmabuf1, *dmabuf2;
  int nextbuf; // cycles 0/1/0/1/etc
  volatile bool refill;

  void reset_dmabuf() {
    refill = false;
    nextbuf = 0;
    for (int n=0; n<2*DMABUFSIZE; n++)
      dmabuf1[n] = KZERO; // + (KZERO << 16);
  }
  
  void init_dmabuf() {
    dmabuf1 = (uint32_t*)malloc(4*2*DMABUFSIZE * 3);
    dmabuf2 = dmabuf1 + DMABUFSIZE;
    reset_dmabuf();
  }
  
  //////////////////////////////////////////////////////////////////////
  // DMA channels

  int dmachan1;
  int dmachan2;
  uint32_t dmamask1;
  uint32_t dmamask2;

  void init_dmachannels() {
    dmachan1 = dma_claim_unused_channel(true);
    dmachan2 = dma_claim_unused_channel(true);  
    dmamask1 = 1u<<dmachan1;
    dmamask2 = 1u<<dmachan2;
  }

  //////////////////////////////////////////////////////////////////////
  // DMA and interrupt handling

  void __not_in_flash_func(dma_isr)() {
    test = sio_hw->cpuid;
    if (dma_channel_get_irq1_status(dmachan1)) {
      dma_channel_set_read_addr(dmachan1, dmabuf1, false);
      dma_channel_acknowledge_irq1(dmachan1);
      refill = true;
    }
    if (dma_channel_get_irq1_status(dmachan2)) {
      dma_channel_set_read_addr(dmachan2, dmabuf2, false);
      dma_channel_acknowledge_irq1(dmachan2);
      refill = true;
    }
  }

  void setup_dma1(uint32_t chn, uint32_t othchn,
                  uint32_t *buf, uint32_t nwords,
                  bool start) {  
    dma_channel_config cfg = dma_channel_get_default_config(chn);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_write_increment(&cfg, false);
    int dreq = PWM_DREQ_NUM(PWM_SLICE_AO);
    io_rw_32 *dst = &pwm_hw->slice[PWM_SLICE_AO].cc;
    channel_config_set_dreq(&cfg, dreq);
    channel_config_set_chain_to(&cfg, othchn);
    dma_channel_configure(chn, &cfg, dst, buf, nwords, start);
  }

  void init_dmairq() {
    irq_add_shared_handler(DMA_IRQ_1, &dma_isr,
                           PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    irq_set_enabled(DMA_IRQ_1, true);
    dma_channel_set_irq1_enabled(dmachan1, true);
    dma_channel_set_irq1_enabled(dmachan2, true);
    setup_dma1(dmachan2, dmachan1, dmabuf1, DMABUFSIZE, false);
    setup_dma1(dmachan1, dmachan2, dmabuf2, DMABUFSIZE, true);
  }

  //////////////////////////////////////////////////////////////////////
  // main code
  int dblbuf_logK = 5; // i.e., 3 MHz
  int dblbuf_samplerepeats = 64; // in terms of PWM periods, i.e., "48" kHz
  int dblbuf_blockrepeats = 1; // how many times each sample is used
  ModulationMode dblbuf_mode = ModulationMode::PWM;

  int32_t value;
  int32_t kremaining = 0;
  int32_t fremaining = 0;
  uint32_t *fillptr = 0;


  int32_t pwm;
  
  void __not_in_flash_func(filler_pwm)() {
    /* We want value=0 to map to K/2 and we want half a step
       to exist for k=0 and k=K. Let's check our math:

       import numpy as np
       import matplotlib.pyplot as plt
       plt.ion()

       K = 32
       values = np.arange(-32767, 32768)
       pwm = ((values + 32768) * K + 32768) >> 16

       plt.clf()
       plt.plot(values, pwm);
    */
    
    pwm = ((value + 32768) * K + 32768) >> 16;
    if (pwm < 0)
      pwm = 0;
    if (pwm > K)
      pwm = K;

    int32_t now;
    if (kremaining < fremaining) {
      now = kremaining;
      kremaining = 0;
      fremaining -= now;
    } else {
      now = fremaining;
      fremaining = 0;
      kremaining -= now;
    }
    if (now <= 0)
      return;
    now ++;
    while (--now) 
      *fillptr++ = pwm | (pwm << 16);
  }

  int32_t sigma0 = 0;
  int32_t y0 = 0;

#include "core1a.cpp"
  //void filler_sdm() { }
  //void filler_sdpwm() { }
  
  void (*filler_current)() = 0;

  void __not_in_flash_func(core1_main)() {
    init_dmabuf();
    init_dmachannels();
    multicore_fifo_push_blocking(0);
    init_dmairq();

    ModulationMode mode = ModulationMode::PWM;
    filler_current = &filler_pwm;

    //    test = 0;
    value = 0; // mid range
    kremaining = 0;
    fremaining = 0;
    fillptr = 0;

    bool led = false;
    ledoff();

    int irep = 0;
    int dvalue = 0;
    int value1 = 0;
    while (true) {
      test += 10;
      test2 += 1;
      if (kremaining == 0) {
        Ringbuffer::lock();
        if (Ringbuffer::empty()) {
          /*
            value -= 3;
            if (value < -10000)
            value = 10000;
          */
        } else {
          //if (irep<=0) {
            value = *Ringbuffer::readptr();
            Ringbuffer::readoffset ++;
            //irep = dblbuf_blockrepeats;
            //dvalue = (value1 - value) / irep;
            //}
            //value += dvalue;
            //irep --;
        }
        Ringbuffer::unlock();
        kremaining = dblbuf_samplerepeats;
        if (dblbuf_logK != logK) {
          set_logK(dblbuf_logK);
          y0 = sigma0 = 0;
        }
      }

      if (refill) {
        fremaining = DMABUFSIZE;
        fillptr = (nextbuf) ? dmabuf2 : dmabuf1;
        nextbuf = !nextbuf;
        refill = false;
      }

      if (dblbuf_mode != mode) {
        mode = dblbuf_mode;
        switch (mode) {
        case ModulationMode::PWM: filler_current = &filler_pwm; break;
        case ModulationMode::SDM: filler_current = &filler_sdm; break;
        case ModulationMode::SDPWM: filler_current = &filler_sdpwm; break;
        }
        y0 = sigma0 = 0;
      }
      
      filler_current();
    }
    blink(2);
  }
  
  //////////////////////////////////////////////////////////////////////
  // Public interface

  void setPeriod(int fd_period_pwmclock) {
    dblbuf_samplerepeats = fd_period_pwmclock;
  }
  
  void setOver(int reps) {
    dblbuf_blockrepeats = reps;
  }

  void setPWMClock(int logK) {
    dblbuf_logK = logK;
  }

  void setMode(ModulationMode m) {
    dblbuf_mode = m;
  }

  void start() {
    sleep_ms(5);
    multicore_reset_core1();
    multicore_fifo_pop_blocking(); // returns a 0
    USB::sendtext("hello");
    init_pwm();
    if (dblbuf_logK != logK)
      set_logK(dblbuf_logK);
    USB::reportint("test", test);
    sleep_ms(5);
    multicore_launch_core1(&core1_main);
    multicore_fifo_pop_blocking(); // returns a 0
    USB::reportint("dma1", int32_t(dmabuf1));
    USB::reportint("dma2", int32_t(dmabuf2));
    USB::reportint("test", test);
    sleep_ms(5);
  }
}
