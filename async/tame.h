
// -*-c++-*-
/* $Id$ */

#ifndef _ASYNC_UNWRAP_H
#define _ASYNC_UNWRAP_H

#include "async.h"

/**
 * hold a pointer to an object without actually increasing its
 * reference count; have a refcounted bool to know if the object
 * is safe to access or not. Should be used on object of type
 * 'weak_referenceable_t', but we template it so we don't need
 * to cast up.
 */
template<class T>
class weak_ref_t {
public:
  weak_ref_t (ptr<T> c) : _p (c), _df (c->destroyed ()) {}
  T *pointer () { return *_df ? NULL : _p; }
private:
  T *_p;
  ptr<bool> _df;
};

class weak_referenceable_t : public virtual refcount {
public:
  weak_referenceable_t () : _destroyed (New refcounted<bool> (false)) {}
  virtual ~weak_referenceable_t () { *_destroyed = true; }
  ptr<bool> destroyed () { return _destroyed; }
private:
  ptr<bool> _destroyed;
};

class closure_t : public weak_referenceable_t {
public:
  closure_t () : _jumpto (0) {}
  closure_t () {}
  void set_jumpto (int i) { _jumpto = i; }
  u_int jumpto () const { return _jumpto; }

protected:
  u_int _jumpto;
};

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


template<class T1 = int, class T2 = int, class T3 = int, class T4 = int>
class join_group_t {
public:
  join_group_t () : _n_out (0) {}

  void set_join_cb (cbv::ptr c) { _join_cb = c; }

  void launch_one () { _n_out ++; }

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


#define CLOSURE ptr<closure_t> __frame = NULL

#endif /* _ASYNC_UNWRAP_H */
