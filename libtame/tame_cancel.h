
// -*-c++-*-
/* $Id: tame.h 2206 2006-09-27 18:24:21Z max $ */

#ifndef _ASYNC_TAME_CANCEL_H_
#define _ASYNC_TAME_CANCEL_H_

#include "tame_event.h"
#include "tame_event_ag.h"
#include "tame_typedefs.h"

/**
 * A helper class useful for canceling an TAME'd function midstream.
 */
class canceller_t {
public:
  canceller_t () 
    : _toolate (false), _queued_cancel (false), _cancelled (false) {}

  // the cancelable function calls this 
  void wait (evv_t b) 
  { 
    if (_queued_cancel) {
      b->trigger();
    } else {
      _cb = b; 
    }
  }

  bool cancelled () const { return _cancelled; }

  // the canceller calls this to cancel the cancelable function
  void cancel ()
  {
    _cancelled = true;
    if (_cb) {
      evv_t::ptr t = _cb;
      _cb = NULL;
      t->trigger ();
    } else if (!_toolate) {
      _queued_cancel = true;
    }
  }

  // the cancelable function can call this if it deems that it is too
  // late to cancel.
  void toolate () { _toolate = true; clear (); }
  void clear () { _cb = NULL; }
private:
  evv_t::ptr _cb;
  bool _toolate, _queued_cancel, _cancelled;
};

#endif /* _ASYNC_TAME_CANCEL_H_ */
