// -*-c++-*-

#include "tame_pipeline3.h"

namespace pipeline3 {
#if 0
}
#endif

//-----------------------------------------------------------------------

tamed void
runner_t::queue_for_takeoff (evv_t ev)
{
  tvars {
    time_t d;
  }

  while (_n_out >= _control->get_window_size ()) {
    twait { _ev = mkevent (); }
  }

  _n_out++;
  d = _control->get_delay_usec ();
  if (d) {
    twait { delaycb (d / 1000000, d * 1000, mkevent ()); }
  }

  ev->trigger ();
}

//-----------------------------------------------------------------------

static u_int64_t
get_time_usec () 
{
  struct timespec ts;
  sfs_get_tsnow (&ts);
  u_int64_t ret = ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
  return ret;
}

//-----------------------------------------------------------------------

tamed void
runner_t::_mkev (evv_t::ptr *out)
{
  tvars {
    size_t i;
    u_int64_t start, end;
  }

  assert (_n_out);

  i = _i++;
  start = get_time_usec ();
  twait { *out = mkevent (); }
  _n_out--;
  end = get_time_usec ();
  
  _control->landed (i, _n_out, end - start);

  if (_ev) {
    evv_t::ptr tmp = _ev;
    _ev = NULL;
    tmp->trigger ();
  }
}

//-----------------------------------------------------------------------

evv_t
runner_t::mkev ()
{
  evv_t::ptr tmp;
  _mkev (&tmp);
  return tmp;
}

//-----------------------------------------------------------------------

tamed void
runner_t::flush (evv_t ev)
{
  while (_n_out) { twait { _ev = mkevent (); } }
  ev->trigger ();
}

//-----------------------------------------------------------------------

}
