// ringbuffer.h

#ifndef RINGBUFFER_H

#define RINGBUFFER_H

#include <stdint.h>
#include <pico/mutex.h>

namespace Ringbuffer {
  extern volatile uint32_t writeoffset;
  extern volatile uint32_t readoffset;
  extern int16_t *buffer;
  constexpr int32_t logbufsize = 8;
  constexpr uint32_t bufsize = 1 << logbufsize; // samples
  constexpr uint32_t mask = bufsize - 1;
  extern mutex_t mtx;
  void initialize();
  void reset();
  inline void lock() { mutex_enter_blocking(&mtx); }
  inline void unlock() { mutex_exit(&mtx); }
  inline int16_t const *readptr() {
    return buffer + (readoffset & mask);
  }
  inline int16_t *writeptr() {
    return buffer + (writeoffset & mask);
  }
  inline bool full() {
    return writeoffset - readoffset >= bufsize;
  }
  inline bool nearlyfull(uint32_t margin) {
    return writeoffset - readoffset > bufsize - margin;
  }
  inline bool empty() {
    return writeoffset == readoffset;
  }
  inline bool nearlyempty(uint32_t margin) {
    return writeoffset - readoffset < margin;
  }

};


#endif
