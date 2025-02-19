// pwmdma.h

#ifndef PWMDMA_H

#define PWMDMA_H

namespace PWMDMA {
  uint16_t *buffer1;
  uint16_t *buffer2;
  constexpr int maxbuflen = 2048;
  int buflen = maxbuflen;
  void initialize();

};

#endif
