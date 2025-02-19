// core1.h

#ifndef CORE1_H

#define CORE1_H

namespace Core1 {
  extern int32_t kremaining;
  extern int32_t fremaining;
  extern uint32_t *fillptr;
  extern int32_t value;
  extern int32_t pwm;
  
  enum class ModulationMode {
    PWM,
    SDM,
    SDPWM,
  };

  void setPeriod(int fs_period_pwmclock);
  void setPWMClock(int logK);
  void setMode(ModulationMode m);
  void start();
}

#endif
