

  
void __not_in_flash_func(filler_sdm)() {
  int32_t x = value;
  int32_t sigma = sigma0 + x;
    
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
  uint32_t K1 = K | (K<<16);
  int32_t dsigma1 = x - 32768;
  int32_t dsigma2 = x + 32768;
  now ++;
  while (--now) {
    if (sigma > 0) {
      sigma += dsigma1;
      *fillptr = K1;
    } else {
      sigma += dsigma2;
      *fillptr = 0;
    }
    fillptr++;
  };
  sigma0 = sigma - x;
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

    int32_t sigma = sigma0 + x;
    int32_t dsigma1 = x - absY;
    int32_t dsigma2 = x + absY;
    //    int32_t y = y0;
    
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
    uint32_t pwm1 = (pwm + 1) | ((pwm + 1) << 16);
    uint32_t pwm0 = pwm | (pwm << 16);
    now ++;
    while (--now) {
      //      sigma += x; // - y;
      if (sigma > 0) {
        sigma += dsigma1;
        *fillptr = pwm1;
        //        y = absY;
      } else {
        sigma += dsigma2;
        *fillptr = pwm0;
        //        y = -absY;
      }
      fillptr ++;
    }
    sigma0 = sigma - x;
    //    y0 = y;
  }
