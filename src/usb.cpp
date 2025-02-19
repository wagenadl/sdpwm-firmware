// usb.cpp

#include <string.h>
#include "usb.h"
#include "reset.h"
#include <tusb.h>


constexpr bool VERBOSE = true;

namespace USB {
  char inbuffer[BLOCKSIZE];
  int inidx = 0;
  char outbuffer[BLOCKSIZE];

  void initialize() {
    //    board_init();
    tud_init(0);
    //    if (board_init_after_tusb) 
    //      board_init_after_tusb();
  }
  
  void busywait() {
    poll();
    Reset::reset_on_bootsel();
  }
  
  void sendoutbuffer(int len) {
    outbuffer[len++] = '\n';
    while (tud_cdc_write_available() < len) {
      busywait();
      if (!connected())
        return;
    }
    tud_cdc_write(outbuffer, len);
    tud_cdc_write_flush();
  }
  
  void sendtext(char const *src) {
    if (!VERBOSE)
      if (src[0] == '~')
        return;
    if (!connected())
      return;
    int len = strnlen(src, BLOCKSIZE - 1);
    memcpy(outbuffer, src, len);
    sendoutbuffer(len);
    poll();
  }
  
  inline char hexdigit(int x) {
    return x < 10 ? '0' + x : 'a' + x - 10;
  }

  void sendhex(uint32_t x) {
    for (int k=0; k<8; k++) {
      outbuffer[7-k] = hexdigit(x & 15);
      x >>= 4;
    }
    sendoutbuffer(8);
  }

  char *receivetext() {
    if (!connected())
      return 0;
    for (int k=0; k<inidx; k++) {
      if (inbuffer[k] < 32) {
        // Preexisting newline.
        // This is possible if readbinarychunk put it there.
        // Abuse the outbuffer for temporary space
        memcpy(outbuffer, inbuffer, k);
        outbuffer[k] = 0;
        // copy rest to beginning of inbuffer
        memmove(inbuffer, inbuffer + k + 1, inidx - k - 1);
        inidx -= k + 1;
        // copy line to end of inbuffer and return
        // (this is guaranteed to fit)
        // of course, result is only valid until next call,
        // but that is always true
        memcpy(inbuffer + inidx, outbuffer, k+1);
        return inbuffer + inidx;
      }
    }
    poll();
    if (!tud_cdc_available())
      return 0;
    while (inidx < BLOCKSIZE) {
      int n = tud_cdc_read(inbuffer + inidx, 1);
      if (n <= 0)
        return 0;
      if (inbuffer[inidx++] < 32) {
        inbuffer[inidx-1] = 0;
        inidx = 0;
        return inbuffer;
      }
    }
    inidx = 0; // drop overly long lines
    return 0;
  }

  bool receivebinary(uint8_t *dest, int nbytes, bool checkascii,
                     bool &err) {
    poll();
    bool first = true;
    while (nbytes > 0) {
      if (!connected())
        return false;
      if (!tud_cdc_available())
        busywait();
      int maxnow = nbytes > BLOCKSIZE ? BLOCKSIZE : nbytes;
      int n = tud_cdc_read(dest, maxnow);
      if (n < 0) {
        err = true;
        return false;
      }
      if (checkascii && first && n>0 && (*dest & 0x80)==0) {
        // switch to ascii -> data are not for us
        memcpy(inbuffer, dest, n);
        inidx = n;
        return false;
      }
      dest += n;
      first = false;
      nbytes -= n;
    }
    return true;
  }
      

  void reportend(bool ok) {
    if (ok)
      sendtext("+stop ok");
    else
      sendtext("+stop error");
  }

  void reportok(char const *label) {
    int n = strnlen(label, 30);
    outbuffer[0] = '+';
    memcpy(outbuffer + 1, label, n++);
    strcpy(outbuffer + n, " ok");
    sendoutbuffer(n + 3);
  }

  void reportbad(char const *label) {
    int n = strnlen(label, 30);
    outbuffer[0] = '+';
    memcpy(outbuffer + 1, label, n++);
    strcpy(outbuffer + n, " ??");
    sendoutbuffer(n + 3);
  }

  void reportint(char const *label, int32_t value) {
    if (!VERBOSE)
      if (label[0] == '~')
        return;
    int n = strnlen(label, 30);
    outbuffer[0] = '+';
    memcpy(outbuffer + 1, label, n++);
    outbuffer[n++] = ' ';
    int ndigits = 0;
    if (value < 0) {
      USB::outbuffer[n++] = '-';
      value = -value;
    }
    int v1 = value;
    while (v1) {
      ndigits++;
      v1 /= 10;
    }
    if (ndigits==0)
      ndigits = 1; // special case for zero
    int len = n + ndigits;
    while (ndigits--) {
      outbuffer[n + ndigits] = '0' + (value % 10);
      value /= 10;
    }
    sendoutbuffer(len);
  }

  void senddebug(char const *x) {
#if DEBUG
    sendtext(x);
#endif
  }

  bool receivechecksummedbinary(uint16_t *data, int count,
                                uint32_t *checksump) {
    uint32_t checksum = 0;
    constexpr int BLOCKHWORDS = 2 * USB::BLOCKWORDS;
    bool err;
    while (count > 0) {
      int now = count > BLOCKHWORDS ? BLOCKHWORDS : count;
      if (!receivebinary((uint8_t*)data, 2 * now, false, err))
        return false;
      for (int k=0; k<now; k++) {
        checksum += *data++;
        checksum += checksum << 10;
        checksum &= 0x7fffffff;
        checksum ^= checksum >> 5;
      }
      count -= now;
    }
    *checksump = checksum;
    return true;
  }
}

