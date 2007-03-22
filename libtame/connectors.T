
// -*-c++-*-
#include "tame.h"
#include "tame_connectors.h"


tamed static void
__add_cancel (evv_t::ptr *out, cancelable_t *cnc, evv_t in, bool *res)
{
  tvars {
    rendezvous_t<bool> rv (__FILE__, __LINE__);
    bool ok;
  }

  cnc->set_notify_on_cancel (mkevent (rv, false));
  *out = mkevent (rv, true);
  twait (rv, ok);
  if (res) *res = ok;
  in->trigger ();
  rv.cancel ();
}


evv_t::ptr
add_cancel (cancelable_t *cnc, evv_t in, bool *res)
{
  evv_t::ptr ncb;
  __add_cancel (&ncb, cnc, in, res);
  return ncb;
}

