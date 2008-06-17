
#include "cgc2.h"

namespace cgc2 {

  mgr_t<> *foo;

  // A single global manager for allocation of lots of smaller objects.
  static template<> mgr_t<> *mgr_t<>::_mgr;

};
