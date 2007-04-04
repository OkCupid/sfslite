

// -*-c++-*-
/* $Id: tame_event.h 2678 2007-04-03 19:27:46Z max $ */

#ifndef _LIBTAME_TAME_EVENT_OPT_H_
#define _LIBTAME_TAME_EVENT_OPT_H_

#include "tame_event.h"
#include "tame_event_ag.h"
#include "tame_closure.h"

class event_v_opt_t;
typedef ptr<event_v_opt_t> event_v_opt_ptr_t;

//
// An experimental optimization for events are void
//
class event_v_opt_t : public _event<>
{
public:
  event_v_opt_t (closure_ptr_t c, const char *loc) 
    : _event<> (_tame_slot_set<> (), loc),
      _closure (c),
      _can_recycle (true) {}

  void reinit (closure_ptr_t c, const char *loc) 
  {
    _event_cancel_base::reinit (loc);
    _closure = c;
    _can_recycle = true;
  }

  void clear_action ()
  {
    closure_ptr_t p = _closure;
    _closure = NULL;
    p = NULL;
  }

  bool perform_action (_event_cancel_base *e, const char *loc, bool reuse)
  {
    bool ret = false;
    if (!_closure) {
      tame_error (loc, "event reused after deallocation");
    } else {
      if (_closure->block_dec_count (loc)) {
	_closure->v_reenter ();
      }
      clear_action ();
      ret = true;
    }
    return ret;
  }

  static event_v_opt_ptr_t alloc (closure_ptr_t c, const char *loc);

  void finalize ();

  ~event_v_opt_t () {}

private:
  closure_ptr_t _closure;
  bool _can_recycle;
};

template<class C>
typename event<>::ref
_mkevent (const closure_wrapper<C> &c, const char *loc)
{
  return event_v_opt_t::alloc (c.closure (), loc);
}

#endif /* _LIBTAME_TAME_EVENT_OPT_H_ */
