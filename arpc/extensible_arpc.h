// -*-c++-*-

#pragma once

#include "async.h"

//=======================================================================

// support for virtual XDR handlers

class v_XDR_dispatch_t;

//------------------------------------------------------------

// forward-declare this, which is in crypt.h
class bigint;

class v_XDR_t : public virtual refcount {
public:
  v_XDR_t (ptr<v_XDR_dispatch_t> d, XDR *x);
  virtual ~v_XDR_t ();
  XDR *xdrp () { return m_x; }
  virtual bool rpc_traverse (u_int32_t &obj) = 0;
  virtual bool rpc_traverse (u_int64_t &obj) = 0;
  virtual bool rpc_encode (str s) = 0;
  virtual bool rpc_decode (str *s) = 0;
  virtual bool rpc_traverse (bigint &b) = 0;
  virtual void enter_field (const char *f) = 0;
  virtual void exit_field (const char *f) = 0;
  virtual void enter_array (size_t i) = 0;
  virtual void enter_slot (size_t i) = 0;
  virtual void exit_slot (size_t i) = 0;
  virtual void exit_array () = 0;
  virtual bool init_decode (const char *msg, ssize_t len) = 0;
  virtual bool enter_pointer (bool &b) = 0;
  virtual bool exit_pointer (bool b) = 0;
  virtual void flush () {}
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
protected:
  static uintptr_t key (const XDR *v);
  qhash<uintptr_t, v_XDR_t *> m_tab;
};

//------------------------------------------------------------

ptr<v_XDR_t> xdr_virtualize (XDR *x);
ptr<v_XDR_t> xdr_virtual_map (u_int32_t rpcvers, XDR *x);

//------------------------------------------------------------

extern ptr<v_XDR_dispatch_t> v_XDR_dispatch;

//------------------------------------------------------------

inline void rpc_enter_field (ptr<v_XDR_t> x, const char *f) 
{ x->enter_field (f); }
inline void rpc_exit_field (ptr<v_XDR_t> x, const char *f) 
{ x->exit_field (f); }
inline void rpc_enter_array (ptr<v_XDR_t> x, size_t i) { x->enter_array (i); }
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
      ret = x->rpc_encode (s);
    }
    break;
  case XDR_DECODE:
    {
      str s;
      if ((ret = x->rpc_decode (&s))) {
	size_t len = min<size_t> (s.len (), max);
	memcpy (obj.base (), s.cstr (), len);
	obj.setsize (len);
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
