// -*-c++-*-

#ifndef __PY_RPCTYPES_H_INCLUDED__
#define __PY_RPCTYPES_H_INCLUDED__

#include <Python.h>
#include "structmember.h"
#include "arpc.h"
#include "rpctypes.h"

struct py_rpcgen_table_t {
  PyObject *(*convert_arg) (PyObject *arg, PyObject *rpc_exception);
  PyObject *(*convert_res) (PyObject *arg, PyObject *rpc_exception);
  PyObject *(*unwrap_res) (void *in);
  void (*dealloc_res) (void *in);
};

extern py_rpcgen_table_t py_rpcgen_error;

class py_rpc_base_t {
public:
  py_rpc_base_t () : _obj (NULL) {}

  py_rpc_base_t (PyObject *o) : _obj (o)
  {
    Py_XINCREF (_obj);
  }
  
  ~py_rpc_base_t ()
  {
    Py_XDECREF (_obj);
  }

  void clear ()
  {
    if (_obj) {
      Py_XDECREF (_obj);
      _obj = NULL;
    }
  }
  PyObject *obj () { return _obj; }

  bool set_obj (PyObject *o)
  {
    PyObject *tmp = _obj;
    _obj = o; // no INCREF since usually just constructed
    Py_XDECREF (tmp);
    return true;
  }

  PyObject *unwrap () 
  {
    PyObject *ret = _obj;
    Py_INCREF (ret);
    delete this;
    return ret;
  }

protected:
  PyObject *_obj;
};

template<size_t max = RPC_INFINITY>
class py_rpc_str : public py_rpc_base_t 
{
public:
  char *get (size_t *sz) const
  {
    char *ret;
    int i;
    if ( PyString_AsStringAndSize (_obj, &ret, &i) < 0) {
      return NULL;
    }
    *sz = i;
    if (*sz >= max) {
      PyErr_SetString (PyExc_TypeError, 
		       "Length of string exceeded\n");
      *sz = max;
    }
    return ret;
  }

  const char * get () const 
  {
    return PyString_AsString (_obj);
  }

  bool set (char *buf, size_t len)
  {
    Py_XDECREF (_obj);
    if (len > max) {
      PyErr_SetString (PyExc_TypeError, 
		       "Length of string exceeded; value trunc'ed\n");
      len = max;
    }
    return (_obj = PyString_FromStringAndSize (buf, len));
  }
};

template<class T, size_t max>
class py_rpc_vec : public py_rpc_base_t 
{
public:
  py_rpc_vec () : _sz (-1) {}
  PyListObject *list () { return reinterpret_cast<PyListObject *> (_obj); }

  int size ()
  {
    int l = PyList_Type.tp_as_sequence->list_length (list ());
    if (l < 0) {
      PyErr_String (PyExc_RuntimeError,
		    "got negative list length from vector");
      return -1;
    }
    return l;
  }
#if 0
    if (l > (int )max) {
      PyErr_SetString (PyExc_OverflowError,
		       "predeclared Length of rpc vector exceeded");
      return -1;
    }
    _sz = l;
    return l;
#endif

  PyObject *get (int i) 
  {
    assert (_sz >= 0 && i < _sz && i >= 0);
    PyObject *r = PyList_Type.tp_as_sequence->list_item (list (), i);
    if (!r) {
      strbuf b;
      b << "undefined object slot @ " << i;
      PyErr_SetString (PyExc_IndexError, str (b));
      return NULL;
    }
    if (!py_type_check<T> (r)) {
      strbuf b;
      b << "typecheck failed on slot " << i << " of vector\n";
      PyErr_SetString (PyExc_TypeError, str (b));
      return NULL;
    }
    return r;
  }

  bool resize (int i)
  {
    if (i > (int)max) {
      PyErr_SetString (PyExc_OverflowError,
		       "cannot resize rpc_vec; overflows preassigned length");
      return false;
    }
    _sz = i;
  }


  bool set (PyObject *obj, int i)
  {
    assert (_sz >= 0 && i < _sz && i >= 0);
    if (!py_type_check<T> (obj)) {

    }
  }
  enum { maxsize = max };
  int _sz;
};

template<size_t n> inline void *
py_rpc_str_alloc ()
{
  return New py_rpc_str<n> ();
}


template<size_t n> inline bool
rpc_traverse (XDR *xdrs, py_rpc_str<n> &obj)
{
  switch (xdrs->x_op) {
  case XDR_ENCODE:
    {
      size_t sz;
      char *dat = obj.get (&sz);
      return dat && xdr_putint (xdrs, sz)
	&& xdr_putpadbytes (xdrs, dat, sz);
    }
  case XDR_DECODE:
    {
      u_int32_t size;
      if (!xdr_getint (xdrs, size) || size > n)
	return false;
      char *dp = (char *) XDR_INLINE (xdrs, size + 3 & ~3);
      if (!dp || memchr (dp, '\0', size))
	return false;
      return obj.set (dp, size);
    }
  default:
    return true;
  }
}

template<size_t n> inline bool
xdr_py_rpc_str (XDR *xdrs, void *objp)
{
  return rpc_traverse (xdrs, *static_cast<py_rpc_str<n> *> (objp));
}

#define PY_RPC_TYPE2STR_DECL(T)			\
template<> struct rpc_type2str<py_##T> {	\
  static const char *type () { return #T; }	\
};

#if 0
template<size_t n> struct rpc_namedecl<py_rpc_str_<n> > {
  static str decl (const char *name) {
    return rpc_namedecl<rpc_str<n> >::decl (name);
  }
};
#endif 

template<size_t n> const strbuf &
rpc_print (const strbuf &sb, const py_rpc_str<n> &pyobj,
	   int recdepth = RPC_INFINITY,
	   const char *name = NULL, const char *prefix = NULL)
{
  const char *obj = pyobj.get ();
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

inline PyObject *
generic_py_xdr_unwrap (void *o)
{
  return reinterpret_cast<py_rpc_base_t *> (o)->unwrap ();
}

inline void
generic_py_xdr_dealloc (void *o)
{
  delete reinterpret_cast<py_rpc_base_t *> (o);
}

#define PY_XDR_UNWRAP(type)                                     \
inline PyObject *type##_unwrap (void *o)                        \
{ return generic_py_xdr_unwrap (o); }

#define PY_XDR_DEALLOC(type)                                    \
inline void type##_dealloc (void *o)                            \
{ generic_py_xdr_dealloc (o); }

#define DECLXDR(type)				                \
extern BOOL xdr_##type (XDR *, void *);		                \
extern void *type##_alloc ();                                   \
extern PyObject *type##_convert (PyObject *o, PyObject *e);     \
PY_XDR_UNWRAP(type)                                             \
PY_XDR_DEALLOC(type)

PY_XDR_UNWRAP(py_rpc_str_t)
PY_XDR_DEALLOC(py_rpc_str_t)

#define INT_XDR_CLASS(ctype,ptype)                               \
class py_##ctype : public py_rpc_base_t                          \
{                                                                \
public:                                                          \
  ctype get () const { return (PyLong_As##ptype (_obj)); }       \
  bool set (ctype i) {                                           \
    Py_XDECREF (_obj);                                           \
    return (_obj = PyLong_From##ptype (i));                      \
  }                                                              \
};

#define RPC_TRAVERSE_DECL(ptype)                                 \
bool rpc_traverse (XDR *xdrs, ptype &obj);

#define INT_DO_ALL_H(ctype,ptype)                                \
INT_XDR_CLASS(ctype, ptype)                                      \
RPC_TRAVERSE_DECL(py_##ctype)                                    \
DECLXDR(py_##ctype)                                              \
PY_RPC_TYPE2STR_DECL(ctype)                                      \
RPC_PRINT_TYPE_DECL(py_##ctype)                                  \
RPC_PRINT_DECL(py_##ctype)                                     

INT_DO_ALL_H(u_int32_t, UnsignedLong)
INT_DO_ALL_H(int32_t, Long)
INT_DO_ALL_H(u_int64_t, UnsignedLongLong)
INT_DO_ALL_H(int64_t, LongLong)


PyObject *convert_error (PyObject *, PyObject *e);
inline PyObject *unwrap_error (void *o) { return NULL; }
inline void dealloc_error (void *o) { return; }

PyObject *void_convert (PyObject *o, PyObject *e);
PyObject *void_unwrap (void *o);
inline void void_dealloc (void *o) { return; }

struct py_rpc_program_t {
  PyObject_HEAD
  const rpc_program *prog;
  const py_rpcgen_table_t *pytab;
};

template<class T, class A> inline bool
rpc_traverse_slot (T &t, A &a, int i)
{
  PyErr_SetString (PyExc_RuntimeError,
		   "no rpc_traverse_slot function found");
  return false;
}

template<class A, size_t max> inline bool
rpc_traverse_slot (XDR *xdrs, py_rpc_vec<A,max> &obj, int i)
{
  assert (i < obj.maxsize);
  switch (xdrs->x_op) {
  case XDR_ENCODE:
    {
      A *el = obj.get (i);
      if (!el)
	return false;
      return rpc_traverse (xdr, *el);
    }
  case XDR_DECODE:
    {
      A el;
      if (!rpc_traverse (xdr, el))
	return false;
      return obj.set (el, i);
    }
  default:
    return false;
  }
}

template<class T, class A, size_t max> inline bool
rpc_traverse (T &t, py_rpc_vec<A,max> &obj)
{
  int isize = obj.size ();
  if (isize < 0)
    return false;
  u_int32_t size = (u_int32_t)isize;

  if (!rpc_traverse (t, size))
    return false;
  if (size > obj.maxsize) {
    PyErr_SetString (PyExc_OverflowError,
		     "predeclared Length of rpc vector exceeded");
    return false;
  }
  if (size < obj.size ())
    obj.shrink (size);

  for (u_int i = 0; i < size; i++) 
    if (!rpc_traverse_slot (t, obj, i))
      return false;

  return true;
}

#endif
