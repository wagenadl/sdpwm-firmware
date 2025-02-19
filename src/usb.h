// usb.h

#ifndef USB_H

#define USB_H

#include <stdint.h>
#include <tusb.h>

#define BULK_PACKET_SIZE (TUD_OPT_HIGH_SPEED ? 512 : 64)


namespace USB {
  void initialize(); // call once
  inline void poll() { tud_task_ext(0, false); } // call periodically
  
  constexpr uint32_t BLOCKSIZE = 64;
  constexpr uint32_t BLOCKWORDS = BLOCKSIZE/4;
  extern char inbuffer[BLOCKSIZE];
  extern int inidx;
  extern char outbuffer[BLOCKSIZE];

  /// Send text to the host.
  /// Text may be zero-terminated, or by any ASCII control character.
  /// A newline is insert at the end and transmitted.
  /// The text must be prepared in advance in OUTBUFFER
  /// and may not be more than BLOCKSIZE - 1 characters long.
  /// Returns immediately if not connected.
  /// Otherwise, blocks until output space available.
  void sendoutbuffer(int len);
  void sendtext(char const *src);
  void senddebug(char const *src);
  void sendhex(uint32_t hex);
  void reportint(char const *label, int32_t value);
  void reportok(char const *label);
  void reportbad(char const *label);
  /// Receive text from host.
  /// Text will be 0-terminated.
  /// Does not block. Returns zero if line not complete.
  /// You may edit returned text in-place.
  /// Text will be overwritten by next call, even if it returns zero.
  /// Host must not send more than one line in a single USB block
  char *receivetext();
  /// Receive binary data from host.
  /// Returns true if all bytes received, or false if the connection
  /// is closed prematurely. If checkascii, Also returns false if the
  /// first byte received is less than 0x80, in which case all
  /// received bytes are pushed into the "inbuffer" for receivetext to
  /// return later.
  /// err is set if usb read fails, but is never cleared
  bool receivebinary(uint8_t *dest, int nbytes, bool checkascii,
                     bool &err);
  bool receivechecksummedbinary(uint16_t *data, int count,
                                uint32_t *checksump);
  inline bool receiveavailable() {
    poll();
    return tud_cdc_available();
  }
  
  inline bool connected() {
    poll();
    return tud_cdc_connected();
  }
  inline bool cansendblock() {
    poll();
    return tud_cdc_write_available() >= USB::BLOCKSIZE;
  }
  inline bool write(void const *data) {
    while (tud_cdc_write_available() < USB::BLOCKSIZE) {
      if (!connected())
        return false;
      else
        poll();
    }
    int n = tud_cdc_write(data, 64);
    tud_cdc_write_flush();
    poll();
    return n == 64;
  }
  void busywait();
}

#endif
