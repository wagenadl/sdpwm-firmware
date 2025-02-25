

  
void __not_in_flash_func(filler_sdm)() {
    int32_t sigma = sigma0;
    int32_t x = value;
    int32_t y = y0;
    
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
    while (--now) {
      sigma += x - y;
      if (sigma > 0) {
        *fillptr++ = K | (K<<16);
        y = 32768;
      } else {
        *fillptr++ = 0;
        y = -32768;
      }
    }
    sigma0 = sigma;
    y0 = y;
  }

void __not_in_flash_func(filler_sdpwm)() {
    /* Let's check the math:
       
       import numpy as np
       import matplotlib.pyplot as plt
       plt.ion()

       logK = 5
       K = 32
       values = np.arange(-32767, 32768)
       pwm = (values + 32768) >> (16 - logK)
       pwm[pwm < 0] = 0
       pwm[pwm > K - 1] = K - 1
       absY = 1 << (15 - logK)
       xx = (values + 32768) - (pwm << (16 - logK)) - absY

       plt.clf()
       plt.plot(values, pwm)
       plt.plot(values, xx / (1 << 15 - 2*logK))
       print(absY, np.min(xx), np.max(xx))
     */
    pwm = (value + 32768) >> (16 - logK);
    if (pwm < 0)
      pwm = 0;
    if (pwm > K - 1)
      pwm = K - 1;

    int32_t absY = 1 << (15 - logK);
    int32_t x = (value + 32768) - (pwm << (16 - logK)) - absY;

    int32_t sigma = sigma0;
    int32_t y = y0;
    
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
    while (--now) {
      sigma += x - y;
      if (sigma > 0) {
        *fillptr++ = (pwm + 1) | ((pwm + 1) << 16);
        y = absY;
      } else {
        *fillptr++ = pwm | (pwm << 16);
        y = -absY;
      }
    }
    sigma0 = sigma;
    y0 = y;
  }
