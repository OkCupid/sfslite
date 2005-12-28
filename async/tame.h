
// -*-c++-*-
/* $Id$ */

#ifndef _ASYNC_UNWRAP_H
#define _ASYNC_UNWRAP_H

#include "async.h"
#include "qhash.h"

template<class T>
class weak_refcounted_t {
public:
  weak_refcounted_t () :
    _destroyed_flag (New refcounted<bool> (false)),
    _refcnt (1) {}

  virtual ~weak_refcounted_t () { *_destroyed_flag = true; }

  void weak_incref () { _refcount++; }

  void weak_decref () 
  {
    assert ( --_refcnt >= 0);
    if (!_refcnt) {
      cbv::ptr c = _weak_finalize_cb;
      _weak_finalize_cb = NULL;
      if (c) 
	delaycb (0,0,c);
    }
  }
  
  ptr<bool> destroyed_flag () { return _destroyed_flag; }
  void set_weak_finalize_cb (cbv::ptr c) { _weak_finalize_cb = c; }

private:
  ptr<bool> _destroyed_flag;
  int _refcnt;
  cbv::ptr _weak_finalize_cb;
};

/**
 * Weak reference: hold a pointer to an object, without increasing its
 * refcount, but still knowing whether or not it's been destroyed.
 * If destroyed, don't access it!
 *
 * @param T a class that implements the method ptr<bool> destroyed_flag()
 */
template<class T>
class weak_ref_t {
public:
  weak_ref_t (weak_refcounted_t<T> *p) : 
    _pointer (p), 
    _destroyed_flag (p->destroyed_flag ()) {}
  T * pointer () { return (*_destroyed_flag ? NULL : _pointer); }
  
private:
  weak_refcounted_t<T>         *_pointer;
  ptr<bool>                    _destroyed_flag;
};

class closure_t : public virtual refcount , 
		  public weak_refcounted_t<closure_t>
{
public:
  closure_t (bool c = false) : 
    _jumpto (0), 
    _cceoc_count (0),
    _has_cceoc (c)
  {}
  void set_jumpto (int i) { _jumpto = i; }
  u_int jumpto () const { return _jumpto; }

  // "Call-Exactly-Once Checked Continuation"
  void inc_cceoc_count () { _cceoc_count ++; }
  void set_has_cceoc (bool f) { _has_cceoc = f; }
  void enforce_cceoc (const str &loc);

  void end_of_scope_checks (str loc);

protected:
  u_int _jumpto;
  int _cceoc_count;
  bool _has_cceoc;
};

void check_closure_destroyed (str loc, ptr<bool> flag);

template<class T1 = int, class T2 = int, class T3 = int, class T4 = int>
struct value_set_t {
  value_set_t () {}
  value_set_t (T1 v1) : v1 (v1) {}
  value_set_t (T1 v1, T2 v2) : v1 (v1), v2 (v2) {}
  value_set_t (T1 v1, T2 v2, T3 v3) : v1 (v1), v2 (v2), v3 (v3) {}
  value_set_t (T1 v1, T2 v2, T3 v3, T4 v4) 
    : v1 (v1), v2 (v2), v3 (v3), v4 (v4) {}

  T1 v1;
  T2 v2;
  T3 v3;
  T4 v4;
};

/**
 * functions defined in tame.C, mainly for reporting errors, and
 * determinig what will happen when an error occurs. Change the
 * runtime behavior of what happens in an error via TAME_OPTIONS
 */
void tame_error (const str &loc, const str &msg);
INIT(tame_init);

template<class T1 = int, class T2 = int, class T3 = int, class T4 = int>
class join_group_pointer_t : public virtual refcount {
public:
  join_group_pointer_t (const char *f, int l) : 
    _n_out (0), _file (f), _lineno (l)  {}

  // warning for now
  ~join_group_pointer_t () 
  { 
    // XXX - find some way to identify this join, either by filename
    // and line number, or other ways.
    if (need_join ()) {
      str s = (_file && _lineno) ?
	str (strbuf ("%s:%d", _file, _lineno)) :
	str ("(unknown)");
      tame_error (s, "non-joined continuations leaked!"); 
    }
  }


  void set_join_cb (cbv::ptr c) { _join_cb = c; }

  void launch_one () { _n_out ++; }
  void rejoin () { _n_out ++; }

  void join (value_set_t<T1, T2, T3, T4> v) 
  {
    _n_out --;
    _pending.push_back (v);
    if (_join_cb) {
      cbv cb = _join_cb;
      _join_cb = NULL;
      (*cb) ();
    }
  }

  bool need_join () const { return _n_out > 0 || _pending.size () > 0; }

  static value_set_t<T1,T2,T3,T4> to_vs ()
  { return value_set_t<T1,T2,T3,T4> (); }

  bool pending (value_set_t<T1, T2, T3, T4> *p)
  {
    bool ret = false;
    if (_pending.size ()) {
      *p = _pending.pop_front ();
      ret = true;
    }
    return ret;
  }

private:
  // number of calls out that haven't yet completed
  u_int _n_out; 

  // number of completing calls that are waiting for a join call
  vec<value_set_t<T1, T2, T3, T4> > _pending;

  // callback to call once
  cbv::ptr _join_cb;

  // try to debug leaked joiners
  const char *_file;
  int _lineno;

  // keep weak references to all of the closures that we references;
  // when we go out of scope, we will unregister at those closures,
  // so the closures can detect closure leaks!
  qhash<u_int, weak_ref_t<closure_t> > _closures;
};

/**
 * @brief A wrapper class around a join group pointer for tighter code
 */
template<class T1 = int, class T2 = int, class T3 = int, class T4 = int>
class join_group_t {
public:
  join_group_t (const char *f = NULL, int l = 0) 
    : _pointer (New refcounted<join_group_pointer_t<T1,T2,T3,T4> > (f, l)) {}
  join_group_t (ptr<join_group_pointer_t<T1,T2,T3,T4> > p) : _pointer (p) {}

  void set_join_cb (cbv::ptr c) { _pointer->set_join_cb (c); }
  void launch_one () { _pointer->launch_one (); }
  void rejoin () { _pointer->rejoin (); }
  bool need_join () const { return _pointer->need_join (); }
  ptr<join_group_pointer_t<T1,T2,T3,T4> > pointer () { return _pointer; }
  bool pending (value_set_t<T1, T2, T3, T4> *p)
  { return _pointer->pending (p); }

  static value_set_t<T1,T2,T3,T4> to_vs () 
  { return value_set_t<T1,T2,T3,T4> (); }

  cbv make_join_cb (value_set_t<T1,T2,T3,T4> w)
  { return wrap (_pointer, &join_group_pointer_t<T1,T2,T3,T4>::join, w); }

private:
  ptr<join_group_pointer_t<T1,T2,T3,T4> > _pointer;
};


template<class T1>
struct pointer_set1_t {
  pointer_set1_t (T1 *p1) : p1 (p1) {}
  T1 *p1;
};

template<class T1, class T2>
struct pointer_set2_t {
  pointer_set2_t (T1 *p1, T2 *p2) : p1 (p1), p2 (p2) {}
  T1 *p1;
  T2 *p2;
};

template<class T1, class T2, class T3>
struct pointer_set3_t {
  pointer_set3_t (T1 *p1, T2 *p2, T3 *p3) : p1 (p1), p2 (p2), p3 (p3) {}
  T1 *p1;
  T2 *p2;
  T3 *p3;
};

template<class T1, class T2, class T3, class T4>
struct pointer_set4_t {
  pointer_set4_t (T1 *p1, T2 *p2, T3 *p3, T4 *p4) 
    : p1 (p1), p2 (p2), p3 (p3), p4 (p4) {}
  T1 *p1;
  T2 *p2;
  T3 *p3;
  T4 *p4;
};


#define CLOSURE        ptr<closure_t> __frame = NULL
#define TAME_OPTIONS   "TAME_OPTIONS"

extern int tame_global_int;


#endif /* _ASYNC_UNWRAP_H */
