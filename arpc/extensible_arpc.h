// -*-c++-*-

#pragma once

#include "async.h"

//=======================================================================

// support for virtual XDR handlers

class v_XDR_dispatch_t;

//------------------------------------------------------------

// forward-declare this, which is in crypt.h
class bigint;

// forward declare this, which comes from asrv.h, for RPC_global_proc_t
// below...
class svccb;
class axprt;
class xdrsuio;

//------------------------------------------------------------

class v_XDR_t : public virtual refcount {
public:
  v_XDR_t (ptr<v_XDR_dispatch_t> d, XDR *x);
  virtual ~v_XDR_t ();
  XDR *xdrp () { return m_x; }
  virtual bool rpc_traverse (u_int32_t &obj) = 0;
  virtual bool rpc_traverse (u_int64_t &obj) = 0;
  virtual bool rpc_traverse (int32_t &obj) = 0;
  virtual bool rpc_traverse (int64_t &obj) = 0;
  virtual bool rpc_traverse (double &obj) = 0;
  virtual bool rpc_encode (str s) = 0;
  virtual bool rpc_decode (str *s) = 0;
  virtual bool rpc_encode_opaque (str s) = 0;
  virtual bool rpc_decode_opaque (str *s) = 0;
  virtual bool rpc_traverse (bigint &b) = 0;
  virtual void enter_field (const char *f) = 0;
  virtual void exit_field (const char *f) = 0;
  virtual bool rpc_traverse_null () = 0;
  virtual bool enter_array (u_int32_t &i, bool dyn_sized) = 0;
  virtual void enter_slot (size_t i) = 0;
  virtual void exit_slot (size_t i) = 0;
  virtual void exit_array () = 0;
  virtual bool init_decode (const char *msg, ssize_t len) = 0;
  virtual bool enter_pointer (bool &b) = 0;
  virtual bool exit_pointer (bool b) = 0;
  virtual void flush (xdrsuio *x) {}

protected:
  ptr<v_XDR_dispatch_t> m_dispatch;
  XDR *m_x;
};

//------------------------------------------------------------

class v_XDR_dispatch_t : public virtual refcount {
public:
  virtual ptr<v_XDR_t> alloc (u_int32_t rpcvers, XDR *input) = 0;
  void remove (XDR *x);
  void add (v_XDR_t *x);
  ptr<v_XDR_t> lookup (XDR *x);
  virtual void v_asrv_alloc (ptr<axprt> x) {}
protected:
  static uintptr_t key (const XDR *v);
  qhash<uintptr_t, v_XDR_t *> m_tab;
};

//------------------------------------------------------------

ptr<v_XDR_t> xdr_virtualize (XDR *x);
ptr<v_XDR_t> xdr_virtual_map (u_int32_t rpcvers, XDR *x);
void xdr_virtual_asrv_alloc (ptr<axprt> x);

//------------------------------------------------------------

extern ptr<v_XDR_dispatch_t> v_XDR_dispatch;

//------------------------------------------------------------

inline void rpc_enter_field (ptr<v_XDR_t> x, const char *f) 
{ x->enter_field (f); }
inline void rpc_exit_field (ptr<v_XDR_t> x, const char *f) 
{ x->exit_field (f); }
inline bool rpc_enter_array (ptr<v_XDR_t> x, u_int32_t &i, bool dyn_sized) 
{ return x->enter_array (i, dyn_sized); }

inline void rpc_exit_array (ptr<v_XDR_t> x) { x->exit_array (); }
inline void rpc_enter_slot (ptr<v_XDR_t> x, size_t s) { x->enter_slot (s); }
inline void rpc_exit_slot (ptr<v_XDR_t> x, size_t s) { x->exit_slot (s); }

//------------------------------------------------------------

#define V_RPC_TRAV_2(T)							\
  inline bool								\
  rpc_traverse (ptr<v_XDR_t> x, T &obj, const char *field = NULL)	\
  {									\
    x->enter_field (field);						\
    bool ret = x->rpc_traverse (obj);					\
    x->exit_field (field);						\
    return ret;								\
  }

V_RPC_TRAV_2(u_int32_t)
V_RPC_TRAV_2(u_int64_t)
V_RPC_TRAV_2(int64_t)
V_RPC_TRAV_2(int32_t)
V_RPC_TRAV_2(double)

//-----------------------------------------------------------------------

template<class T> bool
rpc_traverse (ptr<v_XDR_t> x, rpc_ptr<T> &obj, const char *field = NULL)
{
  bool ret = true;
  bool nonnil = obj;

  x->enter_field (field);
  if (!x->enter_pointer (nonnil)) {
    ret = false;
  } else if (nonnil) {
    ret = rpc_traverse (x, *obj.alloc ());
  } else {
    obj.clear ();
  }
  x->exit_pointer (nonnil);
  x->exit_field (field);
  return ret;
}

//-----------------------------------------------------------------------

template<size_t n> inline bool
rpc_traverse (ptr<v_XDR_t> x, rpc_opaque<n> &obj, const char *field = NULL)
{
  bool ret = false;
  x->enter_field (field);
  switch (x->xdrp ()->x_op) {
  case XDR_ENCODE: 
    {
      mstr s (obj.size ());
      memcpy (s.cstr (), obj.base (), obj.size ());
      ret = x->rpc_encode (s);
    }
    break;
  case XDR_DECODE:
    {
      str s;
      if ((ret = x->rpc_decode (&s))) {
	size_t len = min<size_t> (s.len (), n);
	memcpy (obj.base (), s.cstr (), len);
      }
    }
    break;
  default:
    break;
  }
  x->exit_field (field);
  return ret;
}

//-----------------------------------------------------------------------

template<size_t max> inline bool
rpc_traverse (ptr<v_XDR_t> x, rpc_bytes<max> &obj, const char *field = NULL)
{
  bool ret = false;
  x->enter_field (field);
  switch (x->xdrp()->x_op) {
  case XDR_ENCODE: 
    {
      mstr s (obj.size ());
      memcpy (s.cstr (), obj.base(), obj.size ());
      ret = x->rpc_encode_opaque (s);
    }
    break;
  case XDR_DECODE:
    {
      str s;
      if ((ret = x->rpc_decode_opaque (&s))) {
	size_t len = min<size_t> (s.len (), max);
	obj.setsize (len);
	memcpy (obj.base (), s.cstr (), len);
      }
    }
    break;
  default:
    break;
  }
  x->exit_field (field);
  return ret;
}

//-----------------------------------------------------------------------

template<size_t max> inline bool
rpc_traverse (ptr<v_XDR_t> x, rpc_str<max> &obj, const char *field = NULL)
{ 
  bool ret = false;
  x->enter_field (field);
  switch (x->xdrp()->x_op) {
  case XDR_ENCODE: 
    ret = x->rpc_encode (obj);
    break;
  case XDR_DECODE:
    ret = x->rpc_decode (&obj);
    break;
  default:
    break;
  }
  x->exit_field (field);
  return ret;
}

//=======================================================================

typedef enum {
  RPC_CONSTANT_NONE = 0,
  RPC_CONSTANT_PROG = 1,
  RPC_CONSTANT_VERS = 2,
  RPC_CONSTANT_PROC = 3,
  RPC_CONSTANT_ENUM = 4,
  RPC_CONSTANT_POUND_DEF = 5
} rpc_constant_type_t;

class rpc_constant_collector_t {
public:
  virtual ~rpc_constant_collector_t () {}
  virtual void collect (const char *k, int i, rpc_constant_type_t t) = 0;
  virtual void collect (const char *k, str v, rpc_constant_type_t t) = 0;
  virtual void collect (const char *k, const char *c, rpc_constant_type_t t) 
  = 0;

  // Collect maps of XDR class names to XDR routines
  virtual void collect (const char *k, xdr_procpair_t p) = 0;
};

//-----------------------------------------------------------------------

typedef void (*rpc_constant_collect_hook_t) (rpc_constant_collector_t *);
typedef vec<rpc_constant_collect_hook_t> rpc_constant_collect_hooks_t;

class rpc_add_cch_t {
public:
  rpc_add_cch_t (rpc_constant_collect_hook_t h);
};

ptr<rpc_constant_collect_hooks_t> fetch_rpc_constant_collect_hooks ();
void global_rpc_constant_collect (rpc_constant_collector_t *rcc);

//=======================================================================
