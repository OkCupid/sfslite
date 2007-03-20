
// -*-c++-*-
/* $Id$ */

#ifndef _ASYNC_TAME_EVENT_H_
#define _ASYNC_TAME_EVENT_H_

#include "refcnt.h"
#include "vec.h"
#include "init.h"


class event_base_t {
public:
  event_base_t (const char *loc = NULL) : 
    _loc (loc), 
    _state (NONE) {}
  enum { NONE = 0, CLEARED = 1, CANCELED = 2, DELETED = 4 };
  ~event_base_t () { _state = DELETED; }
private:
  const char *const _loc;
protected:
  state_t _state;
};

#ifdef WRAP_DEBUG
# define CALLBACK_ARGS(x) "???", x, x
# else
# define CALLBACK_ARGS(x)
#endif

template<class B1 = void, class B2 = void, class B3 = void>
class event_t<B1,B2,B3> : public virtual event_base_t, 
			  public callback<void,B1,B2,B3> 
{
public:
  event_t (const char *loc) 
    : event_base_t (loc), callback<void,B1,B2,B3> (CALLBACK_ARGS(loc)) {}
};

template<class B1, class B2, class B3>
class ievent_t : public event_t<B1,B2,B3>
{
public:
  ievent_t (ptr<reenterer_t> r, const char *loc, refset<B1,B2,B3> rs) 
    : event_t<B1,B2,B3> (loc, rs), _reenter (r) {}
private:
  ptr<reenterer_t> _reenter;
};


#endif /* _ASYNC_TAME_EVENT_H_ */
