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
		     const V &vs,
		     const char *loc)
    : _rv (rv),
      _cls (c),
      _value_set (vs),
      _loc (loc),
      _cleared (false)
  {}


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
  const char *_loc;
  bool _cleared;
};

template<class W1=nil_t, class W2=nil_t, class W3=nil_t, class W4=nil_t>
class rendezvous : public weakrefcount {
public:
  rendezvous (const char *loc = NULL)
    : _loc (loc ? loc : "(unknown)"),
      _join_method (JOIN_NONE) {}
  ~rendezvous () {}

  typedef rendezvous<W1,W2,W3,W4> my_type_t;
  typedef value_set_t<W1,W2,W3,W3> my_value_set_t;

  template<class T1, class T2, class T3>
  typename event<T1,T2,T3>::ref
  _mkevent (ptr<closure_t> c, const char *loc,
	    const my_value_set_t &vs,
	    const refset_t<T1,T2,T3> &rs)
  {
    typename event<T1,T2,T3>::ref ret = New refcounted<_event<T1,T2,T3> > 
      (rendezvous_action<my_type_t, my_value_set_t> 
       (mkweakref (this), c, loc, vs), rs, loc);
    _events.insert_head (ret);
  }

  void 
  cancel ()
  {
    this->_flag->set_cancelled ();
    _event_cancel_base *b;
    while ((b = _events.first)) {
      _events.remove (b);
      b->cancel ();
    }
  }

  void join (const my_value_set_t &v)
  {

  }

  void remove (_event_cancel_base *e) { _events.remove (e); }

private:
  list<_event_cancel_base, &_event_cancel_base::_lnk> _events;
  vec<my_value_set_t> _values;
  event<>::ptr _join_ev;
};


#endif /* _LIBTAME_RENDEZVOUS_H_ */

