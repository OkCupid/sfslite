
// -*-c++-*-
/* $Id: tame.h 2206 2006-09-27 18:24:21Z max $ */

#ifndef _ASYNC_TAME_CANCEL_H_
#define _ASYNC_TAME_CANCEL_H_

/**
 * A helper class useful for canceling an TAME'd function midstream.
 */
class canceller_t {
public:
  canceller_t () 
    : _toolate (false), _queued_cancel (false), _cancelled (false) {}

  // the cancelable function calls this 
  void wait (cbv b) 
  { 
    if (_queued_cancel) {
      TRIGGER (b);
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
      cbv::ptr t = _cb;
      _cb = NULL;
      TRIGGER (t);
    } else if (!_toolate) {
      _queued_cancel = true;
    }
  }

  // the cancelable function can call this if it deems that it is too
  // late to cancel.
  void toolate () { _toolate = true; clear (); }
  void clear () { _cb = NULL; }
private:
  cbv::ptr _cb;
  bool _toolate, _queued_cancel, _cancelled;
};

#endif /* _ASYNC_TAME_CANCEL_H_ */
