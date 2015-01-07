// -*-c++-*-
/* $Id$ */

/*
 *
 * Copyright (C) 1998 David Mazieres (dm@uun.org)
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

#ifndef _RPCTYPES_H_
#define _RPCTYPES_H_ 1

#include "str.h"
#include "vec.h"
#include "array.h"
#include "union.h"
#include "keyfunc.h"
#include "err.h"
#include "qhash.h"

#ifdef __GXX_EXPERIMENTAL_CXX0X__
#include <initializer_list>
#endif


typedef void * (*xdr_alloc_t) ();

struct rpcgen_table {
  const char *name;

  const std::type_info *type_arg;
  xdr_alloc_t alloc_arg;
  sfs::xdrproc_t xdr_arg;
  void (*print_arg) (const void *, const strbuf *, int,
		     const char *, const char *);

  const std::type_info *type_res;
  xdr_alloc_t alloc_res;
  sfs::xdrproc_t xdr_res;
  void (*print_res) (const void *, const strbuf *, int,
		     const char *, const char *);
};

struct rpc_program {
  u_int32_t progno;
  u_int32_t versno;
  const struct rpcgen_table *tbl;
  size_t nproc;
  const char *name;
  bool lookup (const char *rpc, u_int32_t *out) const;
};

struct xdr_procpair_t {
  xdr_procpair_t () : alloc (NULL), proc (NULL) {}
  xdr_procpair_t (xdr_alloc_t a, sfs::xdrproc_t p) : alloc (a), proc (p) {}
  xdr_alloc_t alloc;
  sfs::xdrproc_t proc;
};

enum { RPC_INFINITY = 0x7fffffff };

template<class T> class rpc_ptr {
  T *p;

public:
  SFS_INLINE_VISIBILITY rpc_ptr () { p = NULL; }
  SFS_INLINE_VISIBILITY rpc_ptr (const rpc_ptr &rp) { p = rp ? New T (*rp) :
      NULL; }
  SFS_INLINE_VISIBILITY ~rpc_ptr () { delete p; }

  SFS_INLINE_VISIBILITY void clear () { delete p; p = NULL; }
  SFS_INLINE_VISIBILITY rpc_ptr &alloc () { if (!p) p = New T; return *this; }
  rpc_ptr &assign (T *tp) { clear (); p = tp; return *this; }
  T *release () { T *r = p; p = NULL; return r; }

  SFS_INLINE_VISIBILITY operator T *() const { return p; }
  SFS_INLINE_VISIBILITY T *operator-> () const { return p; }
  SFS_INLINE_VISIBILITY T &operator* () const { return *p; }

  rpc_ptr &operator= (const rpc_ptr &rp) {
    if (!rp.p)
      clear ();
    else if (p)
      *p = *rp.p;
    else
      p = New T (*rp.p);
    return *this;
  }
  void swap (rpc_ptr &a) { T *ap = a.p; a.p = p; p = ap; }
};

template<class T> inline void
swap (rpc_ptr<T> &a, rpc_ptr<T> &b)
{
  a.swap (b);
}

// MK 2010/11/29 -- change this inheritance to be public rather than
// private...  it was causing too many headaches otherwise, and i don't
// understand why it was the way it was....
template<class T, size_t max> class rpc_vec : public vec<T> {
  typedef vec<T> super;
public:
  typedef typename super::elm_t elm_t;
  typedef typename super::base_t base_t;
  using super::basep;
  using super::firstp;
  using super::lastp;
  using super::limp;
  enum { maxsize = max };

protected:
  bool nofree;

  SFS_INLINE_VISIBILITY  void init () { nofree = false; super::init (); }
  SFS_INLINE_VISIBILITY  void del () { if (!nofree) super::del (); }

  void copy (const elm_t *p, size_t n) {
    clear ();
    reserve (n);
    const elm_t *e = p + n;
    while (p < e)
      push_back (*p++);
  }
  template<size_t m> void copy (const rpc_vec<T, m> &v) {
    assert (v.size () <= maxsize);
#if 0
    if (v.nofree) {
      del ();
      nofree = true;
      basep = limp = NULL;
      firstp = v.firstp;
      lastp = v.lastp;
      return;
    }
#endif
    if (nofree)
      init ();
    super::operator= (v);
  }
  SFS_INLINE_VISIBILITY  void ensure (size_t m) {
    assert (!nofree);
    assert (size () + m <= maxsize);
  }

public:
  SFS_INLINE_VISIBILITY  rpc_vec () { init (); }
  SFS_INLINE_VISIBILITY  rpc_vec (const rpc_vec &v) { init (); copy (v); }

  #ifdef __GXX_EXPERIMENTAL_CXX0X__
  SFS_INLINE_VISIBILITY  explicit rpc_vec(std::initializer_list<T> l) : rpc_vec() {
    reserve(l.size());
    for (auto v:l) { push_back(v); }
  }
  #endif

  template<size_t m>
  SFS_INLINE_VISIBILITY  rpc_vec (const rpc_vec<T, m> &v) { init (); copy (v); }
  SFS_INLINE_VISIBILITY  ~rpc_vec () { if (nofree) super::init (); }
  SFS_INLINE_VISIBILITY  void clear () { del (); init (); }

  rpc_vec &operator= (const rpc_vec &v) { copy (v); return *this; }
  template<size_t m>
  SFS_INLINE_VISIBILITY rpc_vec &operator= (const rpc_vec<T, m> &v)
    { copy (v); return *this; }
  template<size_t m>
  SFS_INLINE_VISIBILITY rpc_vec &operator= (const ::vec<T, m> &v)
    { copy (v.base (), v.size ()); return *this; }
  template<size_t m>
  SFS_INLINE_VISIBILITY rpc_vec &operator= (const array<T, m> &v)
    { switch (0) case 0: case m <= max:; copy (v.base (), m); return *this; }

  void swap (rpc_vec &v) {
    bool nf = v.nofree;
    v.nofree = nofree;
    nofree = nf;
    base_t::swap (v);
  }

  rpc_vec &set (elm_t *base, size_t len) {
    assert (len <= maxsize);
    del ();
    nofree = true;
    basep = limp = NULL;
    firstp = base;
    lastp = base + len;
    return *this;
  }
  template<size_t m> rpc_vec &set (const ::vec<T, m> &v)
    { set (v.base (), v.size ()); return *this; }

  SFS_INLINE_VISIBILITY  void reserve (size_t m) {
    ensure (m);
    super::reserve (m);
  }

  void setsize (size_t n) {
    assert (!nofree);
    assert (n <= max);
    super::setsize (n);
  }

  SFS_INLINE_VISIBILITY size_t size () const { return super::size (); }
  SFS_INLINE_VISIBILITY bool empty () const { return super::empty (); }

  SFS_INLINE_VISIBILITY elm_t *base () { return super::base (); }
  SFS_INLINE_VISIBILITY const elm_t *base () const { return super::base (); }

  SFS_INLINE_VISIBILITY  elm_t *lim () { return super::lim (); }
  SFS_INLINE_VISIBILITY  const elm_t *lim () const { return super::lim (); }

  SFS_INLINE_VISIBILITY  elm_t &operator[] (size_t i) {
    return super::operator[] (i);
  }
  SFS_INLINE_VISIBILITY  const elm_t &operator[] (size_t i) const {
    return super::operator[] (i);
  }
  elm_t &at (size_t i) { return (*this)[i]; }
  const elm_t &at (size_t i) const { return (*this)[i]; }

#define _append(v)						\
do {								\
  reserve (v.size ());						\
  for (const elm_t *s = v.base (), *e = v.lim (); s < e; s++)	\
    this->cconstruct (*lastp++, *s);					\
} while (0)

  template<size_t m> rpc_vec &append (const rpc_vec<T,m> &v)
  { _append(v); return *this; }
  template<size_t m> rpc_vec &append (const vec<T,m> &v)
  { _append(v); return *this; }
  template<size_t m> rpc_vec &operator+= (const rpc_vec<T,m> &v)
  { return append(v); }
  template<size_t m> rpc_vec &operator+= (const vec<T,m> &v)
  { return append(v); }

  template<size_t m> rpc_vec (const vec<T, m> &v) {
    init ();
    (void) append (v);
  }

#undef append

  SFS_INLINE_VISIBILITY elm_t &push_back () {
    ensure (1);
    return super::push_back ();
  }
  SFS_INLINE_VISIBILITY elm_t &push_back (const elm_t &e) {
    ensure (1);
    return super::push_back (e);
  }
  elm_t pop_back () {
    if (nofree) {
      assert (lastp > firstp);
      return *--lastp;
    }
    else
      return super::pop_back ();
  }
  elm_t pop_front () {
    if (nofree) {
      assert (lastp > firstp);
      return *firstp++;
    }
    else
      return super::pop_front ();
  }

  elm_t &front () { return super::front (); }
  const elm_t &front () const { return super::front (); }
  elm_t &back () { return super::back (); }
  const elm_t &back () const { return super::back (); }
};

template<class T, size_t max> void
swap (rpc_vec<T, max> &a, rpc_vec<T, max> &b)
{
  a.swap (b);
}

template<size_t max = RPC_INFINITY> struct rpc_str : str
{
  enum { maxsize = max };

private:
  void check () {
    assert (len () == strlen (cstr ()));
    assert (len () <= maxsize);
  }

public:
  rpc_str () : str ("") {}
  rpc_str (const rpc_str &s) : str (s) {}
  rpc_str (const str &s) : str (s) { check (); }
  rpc_str (const char *p) : str (p) { assert (len () <= maxsize); }
  rpc_str (const strbuf &b) : str (b) { check (); }
  rpc_str (const char *buf, size_t len) : str (buf, len) { check (); }
  rpc_str (const iovec *iov, int cnt) : str (iov, cnt) { check (); }
  rpc_str (mstr &m) : str (m) { check (); }

  rpc_str &operator= (const rpc_str &s)
    { str::operator= (s); return *this; }
  rpc_str &operator= (const char *p)
    { str::operator= (p); if (p) assert (len () <= maxsize); return *this; }
  template<class T> rpc_str &operator= (const T &t)
    { str::operator= (t); check (); return *this; }
  rpc_str &operator= (mstr &m)
    { str::operator= (m); check (); return *this; }
  rpc_str &setbuf (const char *buf, size_t len)
    { str::setbuf (buf, len); check (); return *this; }
  rpc_str &setiov (const iovec *iov, int cnt)
    { str::setiov (iov, cnt); check (); return *this; }
};

template<size_t n = RPC_INFINITY> struct rpc_opaque : array<char, n> {
  rpc_opaque () { bzero (this->base (), this->size ()); }
};
template<size_t n = RPC_INFINITY> struct rpc_bytes : rpc_vec<char, n> {
  using rpc_vec<char, n>::base;
  using rpc_vec<char, n>::size;

  void setstrmem (const str &s) { rpc_vec<char,n>::set (s.cstr (), s.len ()); }
  rpc_bytes &operator= (const str &s) 
  { 
    rpc_vec<char,n>::setsize (s.len ()); 
    memcpy (base (), s.cstr (), size ()); 
    return *this; 
  }
  template<size_t m> rpc_bytes &operator= (const rpc_vec<char, m> &v)
    { rpc_vec<char, n>::operator= (v); return *this; }
  template<size_t m> rpc_bytes &operator= (const array<char, m> &v)
    { rpc_vec<char, n>::operator= (v); return *this; }
};

#if 0
template<size_t n> struct equals<rpc_opaque<n> > {
  equals () {}
  bool operator() (const rpc_opaque<n> &a, const rpc_opaque<n> &b) const
    { return !memcmp (a.base (), b.base (), n); }
};

template<size_t n> struct equals<rpc_bytes<n> > {
  equals () {}
  bool operator() (const rpc_bytes<n> &a, const rpc_bytes<n> &b) const
    { return a.size () == b.size ()
	&& !memcmp (a.base (), b.base (), a.size ()); }
};
#endif

template<size_t n> inline bool
operator== (const rpc_opaque<n> &a, const rpc_opaque<n> &b)
{
  return !memcmp (a.base (), b.base (), n);
}
template<size_t n> inline bool
operator== (const rpc_bytes<n> &a, const rpc_bytes<n> &b)
{
  return a.size () == b.size () && !memcmp (a.base (), b.base (), a.size ());
}

template<size_t n> inline bool
operator!= (const rpc_opaque<n> &a, const rpc_opaque<n> &b)
{
  return memcmp (a.base (), b.base (), n);
}
template<size_t n> inline bool
operator!= (const rpc_bytes<n> &a, const rpc_bytes<n> &b)
{
  return a.size () != b.size () || memcmp (a.base (), b.base (), a.size ());
}

#if 0
template<size_t n, size_t m> inline bool
operator== (const rpc_bytes<n> &a, const rpc_bytes<m> &b)
{
  return a.size () == b.size () && !memcmp (a.base (), b.base (), a.size ());
}
template<size_t n, size_t m> inline bool
operator== (const rpc_bytes<n> &a, const rpc_opaque<m> &b)
{
  return a.size () == b.size () && !memcmp (a.base (), b.base (), a.size ());
}
template<size_t n, size_t m> inline bool
operator== (const rpc_opaque<n> &a, const rpc_bytes<m> &b)
{
  return a.size () == b.size () && !memcmp (a.base (), b.base (), a.size ());
}

template<size_t n, size_t m> inline bool
operator!= (const rpc_bytes<n> &a, const rpc_bytes<m> &b)
{
  return a.size () != b.size () || memcmp (a.base (), b.base (), a.size ());
}
template<size_t n, size_t m> inline bool
operator!= (const rpc_bytes<n> &a, const rpc_opaque<m> &b)
{
  return a.size () != b.size () || memcmp (a.base (), b.base (), a.size ());
}
template<size_t n, size_t m> inline bool
operator!= (const rpc_opaque<n> &a, const rpc_bytes<m> &b)
{
  return a.size () != b.size () || memcmp (a.base (), b.base (), a.size ());
}
#endif

template<size_t n> struct hashfn<rpc_opaque<n> > {
  hashfn () {}
  bool operator () (const rpc_opaque<n> &a) const
    { return hash_bytes (a.base (), n); }
};
template<size_t n> struct hashfn<rpc_bytes<n> > {
  hashfn () {}
  bool operator () (const rpc_bytes<n> &a) const
    { return hash_bytes (a.base (), a.size ()); }
};

/* 
 * Default dummy enter and exit functions
 */
template<class T> void rpc_enter_field (T &t, const char *f) {}
template<class T> void rpc_exit_field (T &t, const char *f) {}
template<class T> void rpc_exit_array (T &t) {}
template<class T> void rpc_enter_slot (T &t, size_t i) {}
template<class T> void rpc_exit_slot (T &t, size_t i) {}
template<class T> void rpc_exit_pointer (T &t, bool b) {}

/*
 * MK 2010/11/01
 *
 * rpc_enter_array is used for both RPC vectors and RPC arrays
 * The former can size dynamically, the latter are statically sized.
 * In default XDR, for vectors, this method will put on the wire
 * (or read off the wire) the size of the vector. For arrays, that's
 * not necessary.  For other types of RPC like JSON, it's
 * not necessary for arrays or vectors.  They can specialize this
 * template to achieve that result.
 *
 */
template<class T> bool rpc_enter_array (T &t, u_int32_t &i, bool is_vector) 
{
  bool ret = true;
  if (is_vector) { ret = rpc_traverse (t, i); }
  return ret;
}

/*
 * Default traversal functions
 */

template<class T, class R, size_t n> inline bool
rpc_traverse (T &t, array<R, n> &obj, const char *field = NULL) 
{
  typedef typename array<R, n>::elm_t elm_t;
  bool ret = true;

  rpc_enter_field(t, field);
  u_int32_t sz = obj.size ();
  rpc_enter_array (t, sz, false);

  elm_t *p = obj.base ();
  elm_t *e = obj.lim ();
  size_t s = 0;
  while (ret && p < e) {
    rpc_enter_slot (t, s);
    if (!rpc_traverse (t, *p++))
      ret = false;
    rpc_exit_slot (t, s++);
  }

  rpc_exit_array (t);
  rpc_exit_field (t, field);
  return ret;
}

template<class T, class R, size_t n> inline bool
rpc_traverse (T &t, rpc_vec<R, n> &obj, const char *field = NULL)
{
  typedef typename rpc_vec<R, n>::elm_t elm_t;

  bool ret = true;
  rpc_enter_field (t, field);
  u_int32_t size = obj.size ();

  if (!rpc_enter_array (t, size, true) || size > obj.maxsize) {
    ret = false;
  } else {
    if (size < obj.size ())
      obj.setsize (size);
    else if (size > obj.size ()) {
      size_t maxreserve = 0x10000 / sizeof (elm_t);
      maxreserve = min<size_t> (maxreserve, size);
      if (obj.size () < maxreserve)
	obj.reserve (maxreserve - obj.size ());
    }
    
    elm_t *p = obj.base ();
    elm_t *e = obj.lim ();
    size_t s = 0;
    while (ret && p < e) {
      rpc_enter_slot (t, s);
      if (!rpc_traverse (t, *p++))
	ret = false;
      rpc_exit_slot (t, s++);
    }
    for (size_t i = size - obj.size (); ret && i > 0; i--) {
      rpc_enter_slot (t, s);
      if (!rpc_traverse (t, obj.push_back ()))
	ret = false;
      rpc_exit_slot (t, s++);
    }
  }
  rpc_exit_array (t);
  rpc_exit_field (t, field);
  return ret;
}
  
template<class T, class R> inline bool
rpc_traverse (T &t, rpc_ptr<R> &obj, const char *field = NULL)
{
  bool nonnil = obj;
  bool ret = true;
  if (!rpc_traverse (t, nonnil)) {
    ret = false;
  } else if (nonnil) {
    ret = rpc_traverse (t, *obj.alloc ());
  } else {
    obj.clear ();
  }
  return ret;
}

template<class T> inline bool
rpc_traverse (T &t, bool &obj, const char *field = NULL)
{
  u_int32_t val = obj;
  bool ret = true;
  rpc_enter_field (t, field);
  if (!rpc_traverse (t, val)) {
    ret = false;
  } else {
    obj = val;
  }
  rpc_exit_field (t, field);
  return ret;
}

template<class T> inline bool
rpc_traverse (T &t, u_int64_t &obj, const char *field = NULL)
{
  u_int32_t hi = obj >> 32;
  u_int32_t lo = obj;
  bool ret = true;
  rpc_enter_field (t, field);
  if (!rpc_traverse (t, hi) || !rpc_traverse (t, lo)) {
    ret = false;
  } else {
    obj = u_int64_t (hi) << 32 | lo;
  }
  rpc_exit_field (t, field);
  return ret;
}

template<class T> inline bool
rpc_traverse (T &t, double &obj, const char *field = NULL)
{
  int64_t d = 100000000;
  double tmp = obj * d;
  int64_t n = int64_t (tmp);
  bool ret = true;
  rpc_enter_field (t, field);
  if (!rpc_traverse (t, d) || !rpc_traverse (t, n)) {
    ret = false; 
  } else {
    obj = (double)n / (double)d;
  }
  rpc_exit_field (t, field);
  return ret;
}

template<class T> inline bool
rpc_traverse (T &t, int32_t &obj, const char *field = NULL)
{
  rpc_enter_field (t, field);
  bool ret = rpc_traverse (t, reinterpret_cast<u_int32_t &> (obj));
  rpc_exit_field (t, field);
  return ret;
}

template<class T> inline bool
rpc_traverse (T &t, int64_t &obj, const char *field = NULL)
{
  rpc_enter_field (t, field);
  bool ret = rpc_traverse (t, reinterpret_cast<u_int64_t &> (obj));
  rpc_exit_field (t, field);
  return ret;
}

#define DUMBTRANS(T, type)						\
  inline bool								\
  rpc_traverse (T &, type &, const char *f = NULL)			\
  {									\
    return true;							\
  }

#define DUMBTRAVERSE(T)				\
DUMBTRANS(T, char)				\
DUMBTRANS(T, bool)				\
DUMBTRANS(T, u_int32_t)				\
DUMBTRANS(T, u_int64_t)				\
template<size_t n> DUMBTRANS(T, rpc_str<n>)	\
template<size_t n> DUMBTRANS(T, rpc_opaque<n>)	\
template<size_t n> DUMBTRANS(T, rpc_bytes<n>)


/*
 * Stompcast support
 */

struct stompcast_t {};
extern const stompcast_t _stompcast;

DUMBTRAVERSE(const stompcast_t)

template<class T> inline bool
stompcast (T &t)
{
  return rpc_traverse (_stompcast, t);
}

/*
 * Clearing support
 */

struct rpc_clear_t {};
extern struct rpc_clear_t _rpcclear;
struct rpc_wipe_t : public rpc_clear_t {};
extern struct rpc_wipe_t _rpcwipe;

#define RPC_FIELD const char *field = NULL

inline bool
SFS_INLINE_VISIBILITY rpc_traverse (rpc_clear_t &, u_int32_t &obj, RPC_FIELD)
{
  obj = 0;
  return true;
}
template<size_t n> inline bool
SFS_INLINE_VISIBILITY rpc_traverse (rpc_clear_t &, rpc_opaque<n> &obj, RPC_FIELD)
{
  bzero (obj.base (), obj.size ());
  return true;
}
template<size_t n> inline bool
SFS_INLINE_VISIBILITY rpc_traverse (rpc_wipe_t &, rpc_opaque<n> &obj, RPC_FIELD)
{
  bzero (obj.base (), obj.size ());
  return true;
}
template<size_t n> inline bool
SFS_INLINE_VISIBILITY rpc_traverse (rpc_clear_t &, rpc_bytes<n> &obj, RPC_FIELD)
{
  obj.setsize (0);
  return true;
}
template<size_t n> inline bool
SFS_INLINE_VISIBILITY rpc_traverse (rpc_wipe_t &, rpc_bytes<n> &obj, RPC_FIELD)
{
  bzero (obj.base (), obj.size ());
  obj.setsize (0);
  return true;
}
template<size_t n> inline bool
SFS_INLINE_VISIBILITY rpc_traverse (rpc_clear_t &, rpc_str<n> &obj, RPC_FIELD)
{
  obj = "";
  return true;
}
template<class T> inline bool
SFS_INLINE_VISIBILITY rpc_traverse (rpc_clear_t &, rpc_ptr<T> &obj, RPC_FIELD)
{
  obj.clear ();
  return true;
}
template<class T> inline bool
SFS_INLINE_VISIBILITY rpc_traverse (rpc_wipe_t &t, rpc_ptr<T> &obj, RPC_FIELD)
{
  if (obj)
    rpc_traverse (t, *obj);
  obj.clear ();
  return true;
}
template<class T, size_t n> inline bool
SFS_INLINE_VISIBILITY rpc_traverse (rpc_clear_t &, rpc_vec<T, n> &obj, RPC_FIELD)
{
  obj.setsize (0);
  return true;
}
template<class T, size_t n> inline bool
rpc_traverse (rpc_wipe_t &t, rpc_vec<T, n> &obj, RPC_FIELD)
{
  for (typename rpc_vec<T, n>::elm_t *p = obj.base (); p < obj.lim (); p++)
    rpc_traverse (t, *p);
  obj.setsize (0);
  return true;
}


template<class T> inline void
rpc_clear (T &obj)
{
  rpc_traverse (_rpcclear, obj);
}

template<class T> inline void
rpc_wipe (T &obj)
{
  rpc_traverse (_rpcwipe, obj);
}

/*
 *  Pretty-printing functions
 */

#define RPC_PRINT_TYPE_DECL(type)					\
void print_##type (const void *objp, const strbuf *,			\
                   int recdepth = RPC_INFINITY, const char *name = "",	\
		   const char *prefix = "");

#define RPC_PRINT_DECL(type)						     \
const strbuf &rpc_print (const strbuf &sb, const type &obj,		     \
			 int recdepth = RPC_INFINITY, const char *name = "", \
			 const char *prefix = "");

#define RPC_PRINT_DEFINE(T)						\
void									\
print_##T (const void *objp, const strbuf *sbp, int recdepth,		\
	   const char *name, const char *prefix)			\
{									\
  rpc_print (sbp ? *sbp : warnx, *static_cast<const T *> (objp),	\
             recdepth, name, prefix);					\
}
#define print_void NULL
#define print_false NULL

template<class T> struct rpc_type2str {
  static const char *type () { return typeid (T).name (); }
};
template<> struct rpc_type2str<char> {
  static const char *type () { return "opaque"; }
};
#define RPC_TYPE2STR_DECL(T)			\
template<> struct rpc_type2str<T> {		\
  static const char *type () { return #T; }	\
};

#define RPC_PRINT_GEN(T, expr)					\
const strbuf &							\
rpc_print (const strbuf &sb, const T &obj, int recdepth,	\
	   const char *name, const char *prefix)		\
{								\
  if (name) {							\
    if (prefix)							\
      sb << prefix;						\
    sb << rpc_namedecl<T >::decl (name) << " = ";		\
  }								\
  expr;								\
  if (prefix)							\
    sb << ";\n";						\
  return sb;							\
}

RPC_TYPE2STR_DECL (bool)
RPC_TYPE2STR_DECL (int32_t)
RPC_TYPE2STR_DECL (u_int32_t)
RPC_TYPE2STR_DECL (int64_t)
RPC_TYPE2STR_DECL (u_int64_t)

RPC_PRINT_TYPE_DECL (bool)
RPC_PRINT_TYPE_DECL (int32_t)
RPC_PRINT_TYPE_DECL (u_int32_t)
RPC_PRINT_TYPE_DECL (int64_t)
RPC_PRINT_TYPE_DECL (u_int64_t)

RPC_PRINT_DECL (char);
RPC_PRINT_DECL (int32_t);
RPC_PRINT_DECL (u_int32_t);
RPC_PRINT_DECL (int64_t);
RPC_PRINT_DECL (u_int64_t);
RPC_PRINT_DECL (bool);

#ifdef MAINTAINER

static inline str
rpc_dynsize (size_t n)
{
  if (n == (size_t) RPC_INFINITY)
    return "<>";
  return strbuf () << "<" << n << ">";
}
static inline str
rpc_parenptr (const str &name)
{
  if (name[0] == '*')
    return strbuf () << "(" << name << ")";
  return name;
}

template<class T> struct rpc_namedecl {
  static str decl (const char *name) {
    return strbuf () << rpc_type2str<T>::type () << " " << name;
  }
};

template<size_t n> struct rpc_namedecl<rpc_str<n> > {
  static str decl (const char *name) {
    return strbuf () << "string " << rpc_parenptr (name) << rpc_dynsize (n);
  }
};
template<class T> struct rpc_namedecl<rpc_ptr<T> > {
  static str decl (const char *name) {
    return rpc_namedecl<T>::decl ((str (strbuf () << "*" << name)).cstr());
  }
};
template<class T, size_t n> struct rpc_namedecl<rpc_vec<T, n> > {
  static str decl (const char *name) {
    return strbuf () << rpc_namedecl<T>::decl ((rpc_parenptr (name)).cstr())
		     << rpc_dynsize (n);
  }
};
template<class T, size_t n> struct rpc_namedecl<array<T, n> > {
  static str decl (const char *name) {
    return rpc_namedecl<T>::decl ((rpc_parenptr (name)).cstr()) << "[" << n << "]";
  }
};
template<size_t n> struct rpc_namedecl<rpc_bytes<n> > {
  static str decl (const char *name) {
    return rpc_namedecl<rpc_vec<char,n> >::decl (name);
  }
};
template<size_t n> struct rpc_namedecl<rpc_opaque<n> > {
  static str decl (const char *name) {
    return rpc_namedecl<array<char,n> >::decl (name);
  }
};

template<size_t n> const strbuf &
rpc_print (const strbuf &sb, const rpc_str<n> &obj,
	   int recdepth = RPC_INFINITY,
	   const char *name = NULL, const char *prefix = NULL)
{
  if (prefix)
    sb << prefix;
  if (name)
    sb << rpc_namedecl<rpc_str<n> >::decl (name) << " = ";
  if (obj)
    sb << "\"" << obj << "\"";	// XXX should map " to \" in string
  else
    sb << "NULL";
  if (prefix)
    sb << ";\n";
  return sb;
}

template<class T> const strbuf &
rpc_print (const strbuf &sb, const rpc_ptr<T> &obj,
	   int recdepth = RPC_INFINITY,
	   const char *name = NULL, const char *prefix = NULL)
{
  if (name) {
    if (prefix)
      sb << prefix;
    sb << rpc_namedecl<rpc_ptr<T> >::decl (name) << " = ";
  }
  if (!obj)
    sb << "NULL;\n";
  else if (!recdepth)
    sb << "...\n";
  else {
    sb << "&";
    rpc_print (sb, *obj, recdepth - 1, NULL, prefix);
  }
  return sb;
}

struct made_by_user_conversion {
  template<class T> made_by_user_conversion (const T &s) {}
};
inline bool
rpc_isstruct (const made_by_user_conversion &)
{
  return true;
}
inline bool
rpc_isstruct (u_int64_t)
{
  return false;
}
inline bool
rpc_isstruct (int64_t)
{
  return false;
}
inline bool
rpc_isstruct (double)
{
  return false;
}
inline bool
rpc_isstruct (u_int32_t)
{
  return false;
}
inline bool
rpc_isstruct (int32_t)
{
  return false;
}

template<class T> const strbuf &
rpc_print_array_vec (const strbuf &sb, const T &obj,
		     int recdepth = RPC_INFINITY,
		     const char *name = NULL, const char *prefix = NULL)
{
  if (name) {
    if (prefix)
      sb << prefix;
    sb << rpc_namedecl<T >::decl (name) << " = ";
  }
  if (obj.size ()) {
    const char *sep;
    str npref;
    if (prefix) {
      npref = strbuf ("%s  ", prefix);
      sep = "";
      sb << "[" << obj.size () << "] {\n";
    }
    else {
      sep = ", ";
      sb << "[" << obj.size () << "] { ";
    }

    if (rpc_isstruct (obj[0])) {
      size_t i;
      size_t n = min<size_t> (obj.size (), recdepth);
      for (i = 0; i < n; i++) {
	if (i)
	  sb << sep;
	if (npref)
	  sb << npref;
	sb << "[" << i << "] = ";
	rpc_print (sb, obj[i], recdepth, NULL, npref.cstr());
      }
      if (i < obj.size ()) {
        sb << (i ? sep : "");
        if (npref)
          sb << npref;
        sb << "..." << (npref ? "\n" : " ");
      }
    }
    else {
      size_t i;
      size_t n = recdepth == RPC_INFINITY ? obj.size ()
	: min  ((size_t) recdepth * 8, obj.size ());;
      if (npref)
	sb << npref;
      for (i = 0; i < n; i++) {
	if (i & 7)
	  sb << ", ";
	else if (i) {
	  sb << ",\n";
	  if (npref)
	    sb << npref;
	}
	rpc_print (sb, obj[i], recdepth, NULL, NULL);
      }
      if (i < obj.size ()) {
	if (i) {
	  sb << ",\n";
	  if (npref)
	    sb << npref;
	}
	sb << "...";
      }
      sb << (npref ? "\n" : " ");
    }

    if (prefix)
      sb << prefix << "};\n";
    else
      sb << " }";
  }
  else if (prefix)
    sb << "[0] {};\n";
  else
    sb << "[0] {}";
  return sb;
}

#define RPC_ARRAYVEC_DECL(TEMP)					\
template<class T, size_t n> const strbuf &			\
rpc_print (const strbuf &sb, const TEMP<T, n> &obj,		\
	   int recdepth = RPC_INFINITY,				\
	   const char *name = NULL, const char *prefix = NULL)	\
{								\
  return rpc_print_array_vec (sb, obj, recdepth, name, prefix);	\
}

RPC_ARRAYVEC_DECL (array)
RPC_ARRAYVEC_DECL (rpc_vec)

#undef RPC_ARRAYVEC_DECL
#define RPC_ARRAYVEC_DECL(TEMP)					\
template<size_t n> const strbuf &				\
rpc_print (const strbuf &sb, const TEMP<n> &obj,		\
	   int recdepth = RPC_INFINITY,				\
	   const char *name = NULL, const char *prefix = NULL)	\
{								\
  return rpc_print_array_vec (sb, obj, recdepth, name, prefix);	\
}

RPC_ARRAYVEC_DECL (rpc_opaque)
RPC_ARRAYVEC_DECL (rpc_bytes)

#undef RPC_ARRAYVEC_DECL

template<class T> RPC_PRINT_DECL (T);
template<class T> RPC_PRINT_GEN (T, sb << "???");

#endif /* MAINTAINER */

#endif /* !_RPCTYPES_H_ */

