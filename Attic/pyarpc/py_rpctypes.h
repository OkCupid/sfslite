// -*-c++-*-

#ifndef __PY_RPCTYPES_H_INCLUDED__
#define __PY_RPCTYPES_H_INCLUDED__

#include <Python.h>
#include "structmember.h"
#include "arpc.h"
#include "rpctypes.h"

// 
// Here are the classes that we are going to be using to make Python
// wrappers for C++/Async style XDR structures:
//
//   py_u_int32_t 
//   py_int32_t
//   py_u_int64_t
//   py_int64_t
//
//   - note these classes are normal C++ classes that wrap
//     a python PyObject *, in which the data is actually
//     stored.  We need this wrapper ability for when we
//     reassign new values to the underlying C++ data; we
//     cannot afford to reassign the whole object then, and
//     therefore need to wrap them.
// 
//   py_rpc_str<n>
//
//    - as above, but parameterized by n, a max size parameter.
//
// for each one of these types, and also the custom types, we
// define the following functions, as required by the XDR 
// interface.
//
//   T_decref (T *self)
//    - in the case of Python Objects, deference; in the case
//      of native types, we'll simply call a deconstructor function;
//
//   void * T_alloc ()
//    - allocate an object of the given variety
//
//   PyObject *T_unwrap()
//    - for complex types, simply return the type;  for wrapped
//      simple types, incref and return the wrapped python object
//      while destroying the wrapper.
//
//   void *T_convert (PyObject *o, PyObject *e)
//    - checks to make sure that the passed in object o is of the 
//      appropriate type. if not, exceptions are set, and NULL
//      is returned.  If so, and if it is a complex XDR type,
//      then the object itself is increfed and returned.
//      That is, it accepts a borrwed reference and returns new
//      reference the the requested object. If the passed in
//      object is a basic type, then a new wrapper is created,
//      pointing to a converted version of the interior object.
//
//   PyObject *T_convert_py2py (PyObject *o)
//    - optional
//
//   T * assign_py_to_c (T &t, PyObject *i)
//     - assign t = *i, roughly, returning the pointer to the
//       new object used. For simple types (with wrapped-in Python
//       objects), it will make a new reference of i, assign t._obj to i, 
//       and then return &t. For complex types, which are are Python
//       objects, recall, it will ensure that the input i is of the
//       right type, INCREF i, and then return i.
//
//   bool xdr_T (XDR *x, void *objp)
//   template<class X> bool rpc_traverse (X &x, T &t)
//    - standard from DM's XDR library.
//

struct py_rpcgen_table_t {
  void *(*convert_arg) (PyObject *arg, PyObject *rpc_exception);
  void *(*convert_res) (PyObject *arg, PyObject *rpc_exception);
  PyObject *(*unwrap_res) (void *in);
  void (*decref_arg) (void *in);
  void (*decref_res) (void *in);
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

  // takes a New reference (not a borrowed reference) and hence does
  // not INC reference count
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

#if 0
template<class T> inline bool
assign_py_to_c (T &t, PyObject *o)
{
  PyErr_SetString (PyExc_RuntimeError, "assign_py_to_c not defined");
  return false;
}
#endif


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

  bool safe_set_obj (PyObject *in)
  {
    PyObject *out = py_rpc_str_convert_py2py (in);
    if (!out) return false;
    return set_obj (out);
  }
};


template<class T, size_t max>
class py_rpc_vec : public py_rpc_base_t 
{
public:
  py_rpc_vec () : _sz (0) {}
  PyListObject *list () { return reinterpret_cast<PyListObject *> (_obj); }

  int size ()
  {
    int l = PyList_Size (_obj);
    if (l < 0) {
      PyErr_String (PyExc_RuntimeError,
		    "got negative list length from vector");
      return -1;
    }
    _sz = l;
    return l;
  }

  T *get (int i, T *out) 
  {
    assert (i < _sz && i >= 0);
    PyObject *in = PyList_GetItem (_obj, i);
    if (!in)
      return false;
    return assign_py_to_c (*in, out);
  }

  bool set (PyObject *el, int i)
  {
    int rc;
    if (i < _sz) {
      rc = PyList_SetItem (_obj, i, el);
    } else if (i == sz) {
      rc = PyList_Append (_obj, el);
      _sz ++;
    } else {
      PyErr_SetString (PyExc_IndexError, "out-of-order list insertion");
      return false;
    }
    return (rc >= 0);
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
generic_py_xdr_decref (void *o)
{
  delete reinterpret_cast<py_rpc_base_t *> (o);
}

#define PY_XDR_UNWRAP(type)                                     \
inline PyObject *type##_unwrap (void *o)                        \
{ return generic_py_xdr_unwrap (o); }

#define PY_XDR_DEALLOC(type)                                    \
inline void type##_decref (void *o)                             \
{ generic_py_xdr_decref (o); }

#define DECLXDR(type)				                \
extern BOOL xdr_##type (XDR *, void *);		                \
extern void *type##_alloc ();                                   \
extern PyObject *type##_convert_py2py (PyObject *o);            \
extern void *type##_convert (PyObject *o, PyObject *e);         \
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
  bool safe_set_obj (PyObject *i)                                \
  {                                                              \
    PyObject *o = py_##ctype##_convert_py_to_py (i);             \
    if (!o) return false;                                        \
    return set_obj (o);                                          \
  }                                                              \
};

#define RPC_TRAVERSE_DECL(ptype)                                 \
bool rpc_traverse (XDR *xdrs, ptype &obj);

#define ASSIGN_PY_TO_C(T)                                        \
T *assign_py_to_c (T &t, PyObject *o)                            \
{ return t.safe_set_obj (o) ? o : NULL } 

#define INT_DO_ALL_H(ctype,ptype)                                \
DECLXDR(py_##ctype)                                              \
INT_XDR_CLASS(ctype, ptype)                                      \
ASSIGN_PY_TO_C(py_##ctype)                                       \
RPC_TRAVERSE_DECL(py_##ctype)                                    \
PY_RPC_TYPE2STR_DECL(ctype)                                      \
RPC_PRINT_TYPE_DECL(py_##ctype)                                  \
RPC_PRINT_DECL(py_##ctype)                                     

INT_DO_ALL_H(u_int32_t, UnsignedLong)
INT_DO_ALL_H(int32_t, Long)
INT_DO_ALL_H(u_int64_t, UnsignedLongLong)
INT_DO_ALL_H(int64_t, LongLong)

template<size_t m> ASSIGN_PY_TO_C(py_rpc_str<m>)

PyObject *convert_error (PyObject *, PyObject *e);
inline PyObject *unwrap_error (void *o) { return NULL; }
inline void decref_error (void *o) { return; }

PyObject *void_convert (PyObject *o, PyObject *e);
PyObject *void_unwrap (void *o);
inline void void_decref (void *o) { return; }

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
  switch (xdrs->x_op) {
  case XDR_ENCODE:
    {
      A el, *elp;
      if (!(elp = obj.get (i, &el)))
	return false;
      return rpc_traverse (xdr, *elp);
    }
  case XDR_DECODE:
    {
      A el;
      if (!rpc_traverse (xdr, el))
	return false;
      return obj.set (&el, i);
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
