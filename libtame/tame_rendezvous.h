// -*-c++-*-
/* $Id: tame_core.h 2654 2007-03-31 05:42:21Z max $ */

/*
 *
 * Copyright (C) 2005 Max Krohn (max@okws.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */

#ifndef _LIBTAME_RENDEZVOUS_H_
#define _LIBTAME_RENDEZVOUS_H_

#include "tame_event.h"

#ifdef HAVE_TAME_PTH
# include <pth.h>
#endif /* HAVE_TAME_PTH */


template<class T1=nil_t, class T2=nil_t, class T3=nil_t, class T4=nil_t>
struct value_set_t {

  typedef value_set_t<T1,T2,T3,T4> my_type_t;

  value_set_t () {}
  value_set_t (const T1 &v1) : v1 (v1) {}
  value_set_t (const T1 &v1, const T2 &v2) : v1 (v1), v2 (v2) {}
  value_set_t (const T1 &v1, const T2 &v2, const T3 &v3) 
    : v1 (v1), v2 (v2), v3 (v3) {}
  value_set_t (const T1 &v1, const T2 &v2, const T3 &v3, const T4 &v4) 
    : v1 (v1), v2 (v2), v3 (v3), v4 (v4) {}

  T1 v1;
  T2 v2;
  T3 v3;
  T4 v4;
};

typedef enum { JOIN_NONE = 0,
	       JOIN_EVENTS = 1,
	       JOIN_THREADS = 2 } join_method_t;

template<class R, class V>
class rendezvous_action {
public:
  rendezvous_action (typename const weakref<R> &rv,
		     ptr<closure_t> c,
		     const V &vs)
    : _rv (rv),
      _cls (c),
      _value_set (vs),
      _cleared (false)
  {
#ifdef HAVE_TAME_PTH
    pth_mutex_init (&_mutex);
    pth_cond_init (&_cond);
#endif /* HAVE_TAME_PTH */
  }

  void perform (_event_cancel_base *event, const char *loc, bool _reuse)
  {
    R *rp;
    if (_cleared) {
      tame_error (loc, "event reused after deallocation");
    } else if ((rp = _rv.pointer ())) {
      rp->_join (_value_set);
      if (!_reuse) {
	clear (rp, event);
      }
    } else if (!_rv.flag ()->is_cancelled ()) {
      tame_error (loc, "event triggered after rendezvous was deallocated");
    }
  }

  void clear (_event_cancel_base *e)
  {
    R *rp = _rv.pointer ();
    if (rp)
      clear (rp, e);
  }

  void clear (R *rp, _event_cancel_base *e)
  {
    _cls = NULL;
    _cleared = true;
    rp->remove (e);
  }
  

private:
  weakref<R> _rv;
  ptr<closure_t> _cls;
  V _value_set;
  bool _cleared;
};

template<class W1=nil_t, class W2=nil_t, class W3=nil_t, class W4=nil_t>
class rendezvous : public weakrefcount {

  typedef rendezvous<W1,W2,W3,W4> my_type_t;
  typedef value_set_t<W1,W2,W3,W3> my_value_set_t;
  friend class rendezvous_action<my_type_t, my_value_set_t>;

public:
  rendezvous (const char *loc = NULL)
    : _loc (loc ? loc : "(unknown)"),
      _join_method (JOIN_NONE),
      _n_events (0) {}
  
  ~rendezvous () { cleanup(); }

  // Public Interface to Rendezvous Class
  void cancel ()
  {
    this->_flag->set_cancelled ();
    cancel_all_events ();
  }

  u_int n_pending () const { return _pending_values.size (); }
  u_int n_events_out () const { return _n_events; }
  u_int n_triggers_left () const { return n_events_out () + n_pending (); }
  bool need_wait () const { return n_triggers_left () > 0; }
  // End Public Interface

  // Public Interface, but internal to tame (hence _ti_*)
  template<class T1, class T2, class T3>
  typename event<T1,T2,T3>::ptr
  _ti_mkevent (ptr<closure_t> c, const char *loc,
	       const my_value_set_t &vs,
	       const refset_t<T1,T2,T3> &rs)
  {
    typename event<T1,T2,T3>::ptr ret;
    if (!this->_flag->is_alive ()) {
      strbuf b;
      b.fmt ("Attempted to add an event to a rendezvous (allocated %s) "
	     "this is no longer active", _loc);
      str s = b;
      tame_error (loc, b.cstr ();
    } else {
      ret = New refcounted<_event<T1,T2,T3> > 
	(rendezvous_action<my_type_t, my_value_set_t> 
	 (mkweakref (this), c, loc, vs), rs, loc);
      _n_events ++;
      _events.insert_head (ret);
    }
      return ret;
    }
  }

  void _ti_clear_join_method () 
  { 
    if (_join_method == JOIN_EVENTS) {
      _join_cb = NULL;
    }
    _join_method = JOIN_NONE; 
  }

  void _ti_set_join_method (join_method_t jm)
  {
    assert (_join_method == JOIN_NONE);
    _join_method = jm;
  }

  void _ti_join (const my_value_set_t &v)
  {
    _pending_values.push_back (v);
    if (_join_method == JOIN_EVENT) {
      assert (_join_ev);
      event<>::ptr e = _join_ev;
      _join_ev = NULL;
      (*_join_ev) ();
    } else if (_join_method == JOIN_THREADS) {
#ifdef HAVE_TAME_PTH
      pth_cond_notify (&_cond, 0);
#else
      panic ("no PTH available\n");
#endif
    } else {
      /* called join before a waiter; we can just queue */
    }
  }

  bool _ti_next_trigger (T1 &r1, T2 &r2, T3 &r3, T4 &r4)
  {
    bool ret = true;
    value_set_t<T1,T2,T3,T4> *v;
    if (pending (&v)) {
      r1 = v->v1;
      r2 = v->v2;
      r3 = v->v3;
      r4 = v->v4;
      consume ();
    } else
      ret = false;
    return ret;
  }

  bool _ti_next_trigger (T1 &r1, T2 &r2, T3 &r3)
  {
    bool ret = true;
    value_set_t<T1,T2,T3> *v;
    if (pending (&v)) {
      r1 = v->v1;
      r2 = v->v2;
      r3 = v->v3;
      consume ();
    } else
      ret = false;
    return ret;
  }

  bool _ti_next_trigger (T1 &r1, T2 &r2)
  {
    bool ret = true;
    value_set_t<T1,T2> *v;
    if (pending (&v)) {
      r1 = v->v1;
      r2 = v->v2;
      consume ();
    } else
      ret = false;
    return ret;
  }

  bool _ti_next_trigger (T1 &r1)
  {
    bool ret = true;
    value_set_t<T1> *v;
    if (pending (&v)) {
      r1 = v->v1;
      consume ();
    } else
      ret = false;
    return ret;
  }

  bool _ti_next_trigger () { return pending (); }

  void wait (T1 &r1, T2 &r2, T3 &r3, T4 &r4)
  {
    while (!_ti_next_trigger (r1, r2, r3, r4))
      threadjoin ();
  }

  void wait (T1 &r1, T2 &r2, T3 &r3)
  {
    while (!_ti_next_trigger (r1, r2, r3))
      threadjoin ();
  }

  void wait (T1 &r1, T2 &r2)
  {
    while (!_ti_next_trigger (r1, r2))
      threadjoin ();
  }

  void wait (T1 &r1)
  {
    while (!_ti_next_trigger (r1))
      threadjoin ();
  }

  void wait ()
  {
    while (!_ti_next_trigger ())
      threadjoin ();
  }

  void waitall () { while (need_wait ()) wait (); }

private:

  bool pending (value_set_t<T1, T2, T3, T4> **p = NULL)
  {
    bool ret = false;
    if (_pending.size ()) {
      if (p) *p = &_pending[0];
      ret = true;
    }
    return ret;
  }

  void consume ()
  {
    assert (_pending.size ());
    _pending.pop_front ();
  }

  void cleanup ()
  {
    if (need_wait () && !this->_flag->is_cancelled ()) {
      strbuf b;
      b.fmt ("rendezvous went out of scope when expecting %u trigger(s)",
	     n_triggers_lieft ());
      str s;
      tame_error (loc, s.cstr ());
    } 
    this->_flag->set_dead ();
    report_leaks (&_events);
    cancel_all_events ();
  }

  void remove (_event_cancel_base *e) 
  { 
    _n_events --;
    _events.remove (e); 
  }
  
  void cancel_all_events () ()
  {
    _event_cancel_base *b;
    while ((b = _events.first)) {
      remove (b);
      b->cancel ();
    }
  }

  void threadjoin ()
  {
    _ti_set_join_method (JOIN_THREADS);
    thread_wait ();
    _ti_clear_join_method ();
  }

  void thread_wait ()
  {
#ifdef HAVE_TAME_PTH
    pth_mutex_acquire (&_mutex, 0, NULL);
    pth_cond_await (&_cond, &_mutex, NULL);
    pth_mutex_release (&_mutex);
#else
    panic ("no PTH available...\n");
#endif
  }


private:
  // Disallow copying
  rendezvous (const my_type_t &rv) {}

  event_cancel_list_t _events;
  vec<my_value_set_t> _pending_values;
  event<>::ptr _join_ev;
  join_method_t _join_method;
  u_int _n_events;

#ifdef HAVE_TAME_PTH
  pth_cond_t _cond;
  pth_mutex_t _mutex;
#endif /* HAVE_TAME_PTH */

};


#endif /* _LIBTAME_RENDEZVOUS_H_ */

