
#include "cgc2.h"

namespace cgc2 {

  //=======================================================================

  static mgr_t<> *_g_mgr;

  mgr_t<> *
  meta_mgr_t<>::get ()
  {
    if (!_g_mgr) {
      _g_mgr = New std_mgr_t<> (std_cfg_t ());
    }
    return _g_mgr;
  }

  void meta_mgr_t<>::set (mgr_t<> *m) { _g_mgr = m; }

  //=======================================================================

};
