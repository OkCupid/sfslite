
// -*-c++-*-
#include "tame_lock.h"

lock_t::waiter_t *
lock_t::acquire (lock_t::mode_t m, cbv cb)
{
  lock_t::waiter_t *ret = NULL;
  bool wait = false;

  assert (m != OPEN);

  if (m == SHARED) {
    if (_mode == OPEN) {
      _mode = SHARED;
      assert (_sharers == 0);
      _sharers = 1;
    } else if (_mode == SHARED) {
      assert (_sharers > 0);
      _sharers ++;
    } else {
      wait = true;
    }
  } else {
    assert (m == EXCLUSIVE);
    if (_mode == OPEN) {
      _mode = EXCLUSIVE;
      assert (_sharers == 0);
    } else {
      wait = true;
    }
  }
    
  if (wait) {
    ret = New lock_t::waiter_t (m, cb);
    _waiters.insert_head (ret);
  }

  if (!ret) cb->trigger ();
  return ret;
}

void
lock_t::call (waiter_t *w)
{
  _waiters.remove (w);
  cbv cb (w->_cb);
  delete w;
  cb->trigger ();
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
  mode_t old_mode = _mode;

  assert (_mode != OPEN);

  if (_mode == SHARED) {
    assert (_sharers > 0);
    if ( -- _sharers == 0) {
      _mode = OPEN;
    }
  } else {
    assert (_sharers == 0);
    _mode = OPEN;
  }

  if (_mode == OPEN) {

    waiter_t *w = _waiters.first;
    if (w) {
      if (w->_mode == SHARED) {
	assert (old_mode != SHARED);
	_mode = SHARED;

	waiter_t *n, *p;
	for (p = _waiters.first; p; p = n) {
	  n = _waiters.next (p);
	  if (p->_mode == SHARED) {
	    _sharers ++;
	    call (p);
	  }
	}
      } else {
	assert (w->_mode == EXCLUSIVE);
	_mode = EXCLUSIVE;
	call (w);
      }
    }
  }

}

tamed void
lock_t::timed_acquire (lock_t::mode_t m, u_int s, u_int ms, cbb cb)
{
  tvars {
    rendezvous_t<bool> rv  (__FILE__, __LINE__);
    lock_t::waiter_t *w;
    bool ok;
    timecb_t *tcb;
  }
  tcb = delaycb (s, ms, mkevent (rv,false));
  w = acquire (m, mkevent (rv,true) );
  twait (rv, ok);
  if (!ok) cancel (w);
  else timecb_remove (tcb);
  TRIGGER (cb, ok);
}
