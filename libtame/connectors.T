// -*-c++-*-

#include "tame_connectors.h"

namespace connector {

  tamed static void
  _tand (event<bool>::ptr *out, evv_t in, bool *target)
  {
    tvars {
      bool my_res;
    }
    
    twait {
      *out = mkevent (my_res);
    }
    if (!my_res) { *target = false; }
    in->trigger ();
  }

  evb_t
  tand (evv_t in, bool *b)
  {
    event<bool>::ptr ret;
    _tand (&ret, in, b);
    return ret;
  }

  tamed static void
  _tor (event<bool>::ptr *out, evv_t in, bool *target)
  {
    tvars {
      bool my_res;
    }
    twait {
      *out = mkevent (my_res);
    }
    if (my_res) { *target = true; }
    in->trigger ();
  }

  evb_t 
  tor (evv_t in, bool *b)
  {
    event<bool>::ptr ret;
    _tor (&ret, in, b);
    return ret;
  }

};

