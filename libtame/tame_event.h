
// -*-c++-*-
/* $Id$ */

#ifndef _LIBTAME_TAME_EVENT_H_
#define _LIBTAME_TAME_EVENT_H_

#include "refcnt.h"
#include "vec.h"
#include "init.h"
#include "async.h"
#include "tame_recycle.h"

struct nil_t {};

void tame_error (const char *loc, const char *msg);

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

class cancelable_t {
public:
  cancelable_t () {}
  virtual void cancel () = 0 ;
  virtual ~cancelable_t () {}
  virtual void set_notify_on_cancel (cbv::ptr cb) = 0;
  virtual ref_flag_ptr_t toolateflag () = 0;
};

class event_action_t : public virtual refcount {
public:
  virtual void perform (bool reuse) = 0;
  virtual void clear() = 0;
  virtual ~event_action_t () {}
};

typedef ptr<event_action_t> event_action_ptr_t;

void callback_second_trigger (const char *loc);

template<class B1=nil_t, class B2=nil_t, class B3=nil_t>
class event_base_t : public cancelable_t {
public:
  event_base_t (ptr<event_action_t> a, refset_t<B1,B2,B3> rs, 
		const char *loc = NULL) : 
    _action (a),
    _refset (rs),
    _loc (loc), 
    _reuse (false),
    _cancelled (false) {}

  ~event_base_t () 
  { 
    finish (); 
    toolate_to_cancel ();
  }

  ref_flag_ptr_t toolateflag () 
  {
    if (!_tlf) 
      _tlf = ref_flag_t::alloc (false);
    return _tlf;
  }

  void cancel ()
  {
    _cancelled = true;
    if (_noc) {
      cbv c (_noc);
      _noc = NULL;
      (*c) ();
    }
  }

  void set_notify_on_cancel (cbv::ptr cb) { _noc = cb; }

  bool set_reuse (bool b) { _reuse = b; return _action; }
  bool get_reuse () const { return _reuse; }

  void dotrig (const B1 &b1, const B2 &b2, const B3 &b3)
  { dotrig (false, b1, b2, b3); }

  void rawtrig (bool assign, const B1 &b1, const B2 &b2, const B3 &b3)
  { dotrig (false, b1, b2, b3, assign); }

  // Must tune dotrig to accept the correct number of arguments
  void dotrig (bool legacy, const B1 &b1, const B2 &b2, const B3 &b3,
	       bool assign = true)
  {
    if (!_cancelled) {

      // once a trigger happens on this, it can't be cancelled
      toolate_to_cancel ();

      ptr<event_action_t> a = _action;
      if (!a) {
	tame_error (_loc, "event triggered after deallocation");
      } else {
	if (!_reuse)
	  _action = NULL;
	if (assign)
	  _refset.assign (b1,b2,b3);
	a->perform (_reuse);
      }
    }
  }

  void toolate_to_cancel ()
  {
    if (_tlf && !*_tlf)
      _tlf->set (true);
    if (_noc)
      _noc = NULL;
  }

  void finish ()
  {
    if (_action) {
      _action->clear();
      _action = NULL;
    }
  }

private:
  ptr<event_action_t> _action;
  refset_t<B1,B2,B3> _refset;
  const char *const _loc;
  bool _reuse, _cancelled;
  ref_flag_ptr_t _tlf;
  cbv::ptr _noc;
};

// Specify 1 extra argument, that way we can do template specialization
// elsewhere.  We should never have an instatiated event class with
// 4 templated types, though.
template<class T1=nil_t, class T2=nil_t, class T3=nil_t, class T4=nil_t> 
class event;

template<class T1=nil_t, class T2=nil_t, class T3=nil_t>
class event_t {
public:
  typedef ref<event<T1,T2,T3> > ref;
  typedef ptr<event<T1,T2,T3> > ptr;
};


#ifdef WRAP_DEBUG
# define CALLBACK_ARGS(x) "???", x, x
# else
# define CALLBACK_ARGS(x)
#endif


#endif /* _LIBTAME_TAME_EVENT_H_ */
