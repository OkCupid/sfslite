
// -*-c++-*-
/* $Id$ */

#ifndef _LIBTAME_TAME_EVENT_H_
#define _LIBTAME_TAME_EVENT_H_

#include "refcnt.h"
#include "vec.h"
#include "init.h"

struct nil_t {};

// A set of references
template<class T1=nil_t, class T2=nil_t, class T3=nil_t, class T4=nil_t>
class refset_t {
public:
  refset_t (T1 &r1, T2 &r2, T3 &r3, T4 &r4) 
    :  _r1 (r1), _r2 (r2), _r3 (r3), _r4 (r4) {}
  refset_t (T1 &r1, T2 &r2, T3 &r3)
    : _r1 (r1), _r2 (r2), _r3 (r3), _r4 (_dummy) {}
  refset_t (T1 &r1, T2 &r2)
    : _r1 (r1), _r2 (r2), _r3 (_dummy), _r4 (_dummy) {}
  refset_t (T1 &r1)
    : _r1 (r1), _r2 (_dummy), _r3 (_dummy), _r4 (_dummy) {}
  refset_t ()
    : _r1 (_dummy), _r2 (_dummy), _r3 (_dummy), _r4 (_dummy) {}

  void assign (const T1 &v1, const T2 &v2, const T3 &v3, const T4 &v4) 
  { _r1 = v1; _r2 = v2; _r3 = v3; _r4 = v4; }
  void assign (const T1 &v1, const T2 &v2, const T3 &v3)
  { _r1 = v1; _r2 = v2; _r3 = v3; }
  void assign (const T1 &v1, const T2 &v2) { _r1 = v1; _r2 = v2; }
  void assign (const T1 &v1) { _r1 = v1; }
  void assign () {}

private:
  nil_t _dummy;

  T1 &_r1;
  T2 &_r2;
  T3 &_r3;
  T4 &_r4;
};

class event_action_t : public virtual refcount {
public:
  virtual void perform (bool reuse) = 0;
};


template<class B1 = void, class B2 = void, class B3 = void>
class event_base_t<B1,B2,B3> {
public:
  event_base_t (ptr<event_action_t> a, refset_t<B1,B2,B3> rs, 
		const char *loc = NULL) : 
    _action (a),
    _refset (rs),
    _loc (loc), 
    _state (NONE),
    _reuse (false) {}

  ~event_base_t () { _state = DELETED; }

  typedef enum { NONE = 0, INUSE = 1, CLEARED = 2, CANCELED = 3, 
		 DELETED = 4 } state_t;

  void set_reuse (bool b) { _reuse = b; }

  void dotrig (bool legacy, const B1 &b1, const B2 &b2, const B3 &b3)
  {
    ptr<event_action_t> a = _action;
    if (!_reuse) {
      if (_state != NONE) {
	callback_second_trigger (_loc);
      }
      _state = CLEARED;
      _action = NULL;
    } else {
      _stat = INUSE;
    }
    _refset.assign (b1,b2,b3);
    a->perform (_reuse);
  }

  void finalize ()
  {
    assert (_reuse);
  }

private:
  ptr<event_action_t> _action;
  refset_t<B1,B2,B3> _refset;
  const char *const _loc;
  state_t _state;
  bool _reuse;
};

#ifdef WRAP_DEBUG
# define CALLBACK_ARGS(x) "???", x, x
# else
# define CALLBACK_ARGS(x)
#endif


#endif /* _LIBTAME_TAME_EVENT_H_ */
