
// -*-c++-*-
/* $Id$ */

#ifndef _LIBTAME_TAME_EVENT_H_
#define _LIBTAME_TAME_EVENT_H_

#include "refcnt.h"
#include "vec.h"
#include "init.h"
#include "async.h"
#include "list.h"
#include "tame_run.h"

struct nil_t {};

// A set of references
template<class T1=nil_t, class T2=nil_t, class T3=nil_t, class T4=nil_t>
class ref_set_t {
public:
  ref_set_t (T1 &r1, T2 &r2, T3 &r3, T4 &r4) 
    :  _r1 (r1), _r2 (r2), _r3 (r3), _r4 (r4) {}
  ref_set_t (T1 &r1, T2 &r2, T3 &r3)
    : _r1 (r1), _r2 (r2), _r3 (r3), _r4 (_dummy) {}
  ref_set_t (T1 &r1, T2 &r2)
    : _r1 (r1), _r2 (r2), _r3 (_dummy), _r4 (_dummy) {}
  ref_set_t (T1 &r1)
    : _r1 (r1), _r2 (_dummy), _r3 (_dummy), _r4 (_dummy) {}
  ref_set_t ()
    : _r1 (_dummy), _r2 (_dummy), _r3 (_dummy), _r4 (_dummy) {}

  void assign (const T1 &v1, const T2 &v2, const T3 &v3, const T4 &v4) 
  { _r1 = v1; _r2 = v2; _r3 = v3; _r4 = v4; }
  void assign (const T1 &v1, const T2 &v2, const T3 &v3)
  { _r1 = v1; _r2 = v2; _r3 = v3; }
  void assign (const T1 &v1, const T2 &v2) 
  { _r1 = v1; _r2 = v2; }
  void assign (const T1 &v1) { _r1 = v1; }
  void assign () { }

private:
  nil_t _dummy;

  T1 &_r1;
  T2 &_r2;
  T3 &_r3;
  T4 &_r4;
};

// Specify 1 extra argument, that way we can do template specialization
// elsewhere.  We should never have an instatiated event class with
// 4 templated types, though.
template<class T1=nil_t, class T2=nil_t, class T3=nil_t, class T4=nil_t> 
class _event;

class _event_cancel_base : public virtual refcount {
public:
  _event_cancel_base (const char *loc) : 
    _loc (loc), 
    _cancelled (false),
    _cleared (false) {}

  void set_cancel_notifier (ptr<_event<> > e) { _cancel_notifier = e; }
  void cancel ();
  const char *loc () const { return _loc; }
  bool cancelled () const { return _cancelled; }


  list_entry<_event_cancel_base> _lnk;

protected:
  virtual void clear_action () = 0;
  void clear ();

  const char *_loc;
  bool _cancelled;
  bool _cleared;
  ptr<_event<> > _cancel_notifier;

};

typedef list<_event_cancel_base, &_event_cancel_base::_lnk> 
event_cancel_list_t;

void report_leaks (event_cancel_list_t *lst);

template<class T1=nil_t, class T2=nil_t, class T3=nil_t>
class _event_base : public _event_cancel_base {
public:
  _event_base (const ref_set_t<T1,T2,T3> &rs,
	       const char *loc = NULL) :
    _event_cancel_base (loc),
    _ref_set (rs),
    _reuse (false) {}
    

  typedef _event_base<T1,T2,T3> my_type_t;

  ~_event_base () { /* must clear action in subclass */ }

  void set_reuse (bool b) { _reuse = b; }
  bool get_reuse () const { return _reuse; }

  bool can_trigger ()
  {
    bool ret = false;
    if (this->_cancelled) {
      if (tame_strict_mode ()) 
	tame_error (this->_loc, "event triggered after it was cancelled");
    } else if (this->_cleared) {
      tame_error (this->_loc, "event triggered after it was cleared");
    } else {
      ret = true;
    }
    return ret;
  }

  void base_trigger (const T1 &t1, const T2 &t2, const T3 &t3)
  {
    if (can_trigger ()) {
      _ref_set.assign (t1, t2, t3);
      if (perform_action (this, this->_loc, _reuse))
	this->_cleared = true;
    }
  }

  void trigger_no_assign ()
  {
    if (can_trigger ()) {
      if (perform_action (this, this->_loc, _reuse))
	this->_cleared = true;
    }
  }

  ref_set_t<T1,T2,T3> &ref_set () { return _ref_set; }

  void finish () { clear (); }

  virtual bool perform_action (_event_cancel_base *e, const char *loc,
			       bool reuse) = 0;

protected:
  ref_set_t<T1,T2,T3> _ref_set;
  bool _reuse;
};


template<class A, class T1=nil_t, class T2=nil_t, 
	 class T3=nil_t, class T4=nil_t> 
class _event_impl;

template<class T1=nil_t, class T2=nil_t, class T3=nil_t>
class event {
public:
  typedef ref<_event<T1,T2,T3> > ref;
  typedef ptr<_event<T1,T2,T3> > ptr;
};


#ifdef WRAP_DEBUG
# define CALLBACK_ARGS(x) "???", x, x
# else
# define CALLBACK_ARGS(x)
#endif

#define mkevent(...) _mkevent (__cls_g, __FL__, ##__VA_ARGS__)
#define mkevent_rs(...) _mkevent_rs (__cls_g, __FL__, ##__VA_ARGS__)

#endif /* _LIBTAME_TAME_EVENT_H_ */
