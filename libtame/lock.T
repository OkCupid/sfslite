
// -*-c++-*-
#include "tame_lock.h"

lock_t::waiter_t *
lock_t::acquire (lock_t::mode_t m, cbv cb)
{
  lock_t::waiter_t *ret = NULL;
  assert (m != OPEN);
  if (_mode == OPEN) {
    _mode = m;
  } else if (_mode != SHARED || m != SHARED) {
    ret = New lock_t::waiter_t (m, cb);
    _waiters.insert_head (ret);
  }
  if (!ret) SIGNAL (cb);
  return ret;
}

void
lock_t::call (waiter_t *w)
{
  _waiters.remove (w);
  cbv cb (w->_cb);
  delete w;
  (*cb) ();
}

void
lock_t::cancel (waiter_t *w)
{
  _waiters.remove (w);
  delete w;
}

void
lock_t::release ()
{
  assert (_mode != OPEN);
  _mode = OPEN;
  waiter_t *w;
  while ((w = _waiters.first) && w->_mode == SHARED) {
    _mode = SHARED;
    call (w);
  }
  if ((w = _waiters.first) && _mode == OPEN) {
    assert (w->_mode == EXCLUSIVE);
    call (w);
  }
}

tamed void
lock_t::timed_acquire (lock_t::mode_t m, u_int s, u_int ms, cbb cb)
{
  cvars {
    coordgroup_t<bool> G (__FILE__, __LINE__);
    lock_t::waiter_t *w;
    bool ok;
    timecb_t *tcb;
  }
  tcb = delaycb (s, ms, mkevent (G,false));
  w = acquire (m, mkevent (G,true) );
  cwait (G, ok);
  if (!ok) cancel (w);
  else timecb_remove (tcb);
  SIGNAL (cb, ok);
}
