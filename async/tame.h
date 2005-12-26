
// -*-c++-*-
/* $Id$ */

#ifndef _ASYNC_UNWRAP_H
#define _ASYNC_UNWRAP_H

#include "async.h"

class closure_t : public virtual refcount {
public:
  closure_t (bool c = false) : 
    _jumpto (0), 
    _destroyed (New refcounted<bool> (false)), 
    _ceocc_count (0),
    _has_ceocc (c)
  {}
  virtual ~closure_t () { *_destroyed = true; }
  void set_jumpto (int i) { _jumpto = i; }
  u_int jumpto () const { return _jumpto; }
  bool destroyed_flag () { return _destroyed; }

  // "Call-Exactly-Once Checked Continuation"
  void inc_ceocc_count () { _ceocc_count ++; }
  void set_has_ceocc (bool f) { _has_ceocc = f; }
  void enforce_ceocc (const str &loc);

protected:
  u_int _jumpto;
  ptr<bool> _destroyed;
  int _ceocc_count;
  bool _has_ceocc;
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


#endif /* _ASYNC_UNWRAP_H */
