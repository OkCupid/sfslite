
#include "cgc2.h"

namespace cgc {

  // A single global manager for allocation of lots of smaller objects.
  static mgr_t<> *mgr_t<>::_mgr;

};
