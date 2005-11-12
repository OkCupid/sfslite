
// -*-c++-*-
/* $Id$ */

#ifndef _ASYNC_UNWRAP_H
#define _ASYNC_UNWRAP_H

#include "async.h"

class trig_t {
public:
  trig_t (cbv c) : _cb (c), _killed (false) {}
  ~trig_t () { if (!_killed) (*_cb) (); }
  void kill () { _killed = true; }
private:
  cbv _cb;
  bool _killed;
};

class freezer_t : public virtual refcount {
public:
  freezer_t () : _jumpto (0) {}
  virtual void reenter () = 0;
  void set_jumpto (int i) { _jumpto = i; }
  u_int jumpto () const { return _jumpto; }
protected:
  u_int _jumpto;
};



#endif /* _ASYNC_UNWRAP_H */
