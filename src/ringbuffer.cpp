// ringbuffer.cpp

#include "ringbuffer.h"
#include <stdlib.h>

namespace Ringbuffer {
  volatile uint32_t writeoffset;
  volatile uint32_t readoffset;
  int16_t *buffer;
  mutex_t mtx;
  
  void initialize() {
    mutex_init(&mtx);
    buffer = (int16_t*)malloc(bufsize * 2 * 3);
    reset();
  }

  void reset() {
    readoffset = writeoffset = 0;
  }
};

void func1() {
}

void func2() {
}
