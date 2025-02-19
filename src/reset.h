// reset.h

#ifndef RESET_H

#define RESET_H

namespace Reset {
  void reset();
  bool check_bootsel();
  void reset_on_bootsel();
  void tobootloader();
}

#endif
