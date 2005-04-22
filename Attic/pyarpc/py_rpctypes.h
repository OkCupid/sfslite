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

//-----------------------------------------------------------------------
// Class Definitions
//

class py_rpc_base_t {
public:
  py_rpc_base_t () : _obj (NULL), _typ (NULL), _onstack (false) {}
  py_rpc_base_t (PyTypeObject *t) : _obj (NULL), _typ (t), _onstack (false) {}
  py_rpc_base_t (PyObject *o) : _obj (o), _typ (NULL) { Py_XINCREF (_obj); }
  ~py_rpc_base_t () { Py_XDECREF (_obj); }

  bool init () ;
  void alloc () { assert (_typ); _obj = _PyObject_New (_typ); }
  void clear ();
  PyObject *obj () { return _obj; }

  // takes a New reference (not a borrowed reference) and hence does
  // not INC reference count
  bool set_obj (PyObject *o);
  PyObject *unwrap () ;
  void set_onstack () { _onstack = true; }

protected:
  PyObject *_obj;
  PyTypeObject *_typ;
  bool _onstack;
};

template<size_t M = RPC_INFINITY>
class py_rpc_str : public py_rpc_base_t 
{
public:
  py_rpc_str () : py_rpc_base_t (&PyString_Type) {}
  char *get (size_t *sz) const;
  const char * get () const { return PyString_AsString (_obj); }
  bool set (char *buf, size_t len);
  bool safe_set_obj (PyObject *in);
  bool init ();
  enum { maxsize = M };
};

template<class T, size_t max>
class py_rpc_vec : public py_rpc_base_t 
{
public:
  py_rpc_vec () : py_rpc_base_t (&PyList_Type), _sz (0) {}
  PyListObject *list () { return static_cast<PyListObject *> (_obj); }
  int size ();
  T *get (int i, T *out) ;
  bool set (PyObject *el, int i);
  bool safe_set_obj (PyObject *in);
  bool shrink (size_t sz);
  bool init ();
  enum { maxsize = max };
private:
  int _sz;
};



//-----------------------------------------------------------------------
// 
// Wrap/Unwrap functions
//
//  - wrapped basic types need to be able to give up their wrapped
//    objects and deallocate themselves, while complex, compiled
//    XDR types are native Python objects, and do not.  These
//    functions help to navigate this difference.
//

template<class T> PyObject *unwrap_tmpl (T *t) { return t->unwrap (); }

static inline PyObject *
generic_py_xdr_unwrap (void *o)
{
  return static_cast<py_rpc_base_t *> (o)->unwrap ();
}

#define PY_XDR_UNWRAP(type)                                     \
inline PyObject *type##_unwrap (void *o)                        \
{ return generic_py_xdr_unwrap (o); }

inline PyObject *unwrap_error (void *o) { return NULL; }
PyObject *void_unwrap (void *o);

PY_XDR_UNWRAP(py_rpc_str);
PY_XDR_UNWRAP(py_rpc_vec);
//
// end of wrapping functions
// 
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
// 
// Allocate / Deallocate / Decref / init
//
// 

template<class T> inline T * alloc_temporary (T &t) { 
  t.set_onstack ();
  return &t; 
}
template<class T> inline void dealloc_temporary (T *t) {}

// for native types, we just call into the class, but compiled
// types don't have methods in their structs, so they will simply
// specialize this template
template<class T> inline bool py_init (T &t) { return t.init (); }

static inline void
generic_py_xdr_decref (void *o)
{
  delete static_cast<py_rpc_base_t *> (o);
}

template<size_t n> inline void *
py_rpc_str_alloc ()
{
  return New py_rpc_str<n> ();
}

inline void void_decref (void *o) { return; }
inline void decref_error (void *o) { return; }

#define PY_XDR_DEALLOC(T)                                       \
inline void T##_decref (void *o)                                \
{ generic_py_xdr_decref (o); }

PY_XDR_DEALLOC(py_rpc_str_t)

#define ALLOC_DECL(T)                                           \
extern void * T##_alloc ();

template<size_t M> bool
py_rpc_str<M>::init ()
{
  if (!py_rpc_base_t::init ())
    return false;
  _typ = &PyString_Type;
  return true;
}

template<class T, size_t M> bool
py_rpc_vec<T,M>::init ()
{
  if (!py_rpc_base_t::init ())
    return false;
  _typ = &PyList_Type;
  PyObject *l = PyList_New (0);
  if (!l) {
    PyErr_SetString (PyExc_MemoryError, "allocation of new list failed");
    return false;
  }
  if (!set_obj (l))
    return false;
  
  return true;
}

//
//
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// XDR wrapper functions
// 

#define XDR_DECL(T)                                            \
extern BOOL xdr_##T (XDR *xdrs, void *objp);

template<size_t n> inline bool
xdr_py_rpc_str (XDR *xdrs, void *objp)
{
  return rpc_traverse (xdrs, *static_cast<py_rpc_str<n> *> (objp));
}

template<class T, size_t n> inline bool
xdr_py_rpc_vec (XDR *xdrs, void *objp)
{
  return rpc_traverse (xdrs, *static_cast<py_rpc_vec<T,n> *> (objp));
}

//
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
// RPC Printing 
//

#define PY_RPC_TYPE2STR_DECL(T)			\
template<> struct rpc_type2str<py_##T> {	\
  static const char *type () { return #T; }	\
};

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

//
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// convert
//   - a python type into its C equivalent.  For compiled, complex
//     types, this will do a type check, and return the arg passed.
//     for wrapped objects, it will allocate the wrapper, and 
//     add the object into the wrapper
//
#define CONVERT_DECL(T)                                        \
extern void * T##_convert (PyObject *o, PyObject *e);             

#define CONVERT_PY2PY_DECL(T)                                  \
extern PyObject * T##_convert_py2py (PyObject *in);

CONVERT_DECL(void);

template<size_t n> void *
py_rpc_str_convert (PyObject *o, PyObject *e)
{
  PyObject *out = py_rpc_str_convert_py2py (in);
  if (!out) return NULL;
  py_rpc_base_t *ret = static_cast<py_rpc_base_t *> (py_rpc_str_alloc<n> ());
  ret->set_obj (out);
  return ret;  
}

template<size_t m> PyObject *
py_rpc_str_convert_py2py (PyObject *in)
{
  if (!PyString_Check (in)) {
    PyErr_SetString (PyExc_TypeError, "expected string type");
    return NULL;
  }
  if (PyString_Size (in) >= m) {
    PyErr_SetString (PyExc_OverflowError, "string exceeds predeclared limits");
    return NULL;
  }
  Py_INCREF (in);
  return in;
}

template<class T, size_t m> PyObject *
py_rpc_vec_convert_py2py (PyObject *in)
{
  if (!PyList_Check (in)) {
    PyErr_SetString (PyExc_TypeError, "expected a list type");
    return NULL;
  }
  if (PyList_Size (in) >= m) {
    PyErr_SetString (PyExc_OverflowError, 
		     "list execeeds predeclared list length");
    return NULL;
  }

  // Would be nice to typecheck here; maybe use RPC traverse?

  Py_INCREF (in);
  return in;
}

void *convert_error (PyObject *o, PyObject *e);

//
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
// assign_py_to_c
//
//  - given a python PyObject, assign it to its corresponding
//    C type
//  - calls convert eventually

// -- applies for simple wrapped classes found in this file;
//    compiled complex classes need to specialize this template
template<class T> T * assign_py_to_c (T &t, PyObject *o)
{ return t.safe_set_obj (o) ? &t : NULL; } 

//
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
// Shortcuts for allocating integer types all at once
//


#define INT_XDR_CLASS(ctype,ptype)                               \
class py_##ctype : public py_rpc_base_t                          \
{                                                                \
public:                                                          \
  py_##ctype () : py_rpc_base_t (&PyLong_Type) {}                \
  ctype get () const { return (PyLong_As##ptype (_obj)); }       \
  bool set (ctype i) {                                           \
    Py_XDECREF (_obj);                                           \
    return (_obj = PyLong_From##ptype (i));                      \
  }                                                              \
  bool safe_set_obj (PyObject *i)                                \
  {                                                              \
    PyObject *o = py_##ctype##_convert_py2py (i);                \
    if (!o) return false;                                        \
    return set_obj (o);                                          \
  }                                                              \
  bool init ()                                                   \
  {                                                              \
    if (!py_rpc_base_t::init ())                                 \
      return false;                                              \
    _typ = &PyLong_Type;                                         \
    return true;                                                 \
  }                                                              \
};

#define RPC_TRAVERSE_DECL(ptype)                                 \
bool rpc_traverse (XDR *xdrs, ptype &obj);

#define INT_DO_ALL_H(ctype,ptype)                                \
XDR_DECL(py_##ctype)                                             \
ALLOC_DECL(py_##ctype)                                           \
CONVERT_DECL(py_##ctype)                                         \
CONVERT_PY2PY_DECL(py_##ctype)                                   \
PY_XDR_UNWRAP(py_##ctype)                                        \
PY_XDR_DEALLOC(py_##ctype)                                       \
INT_XDR_CLASS(ctype, ptype)                                      \
RPC_TRAVERSE_DECL(py_##ctype)                                    \
PY_RPC_TYPE2STR_DECL(ctype)                                      \
RPC_PRINT_TYPE_DECL(py_##ctype)                                  \
RPC_PRINT_DECL(py_##ctype)                                     

INT_DO_ALL_H(u_int32_t, UnsignedLong)
INT_DO_ALL_H(int32_t, Long)
INT_DO_ALL_H(u_int64_t, UnsignedLongLong)
INT_DO_ALL_H(int64_t, LongLong)

//
//-----------------------------------------------------------------------




//-----------------------------------------------------------------------
// RPC traversal
//

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

template<class T, class A> inline bool
rpc_traverse_slot (T &t, A &a, int i)
{
  PyErr_SetString (PyExc_RuntimeError,
		   "no rpc_traverse_slot function found");
  return false;
}

template<class A, size_t max> inline bool
rpc_traverse_slot (XDR *xdrs, py_rpc_vec<A,max> &v, int i)
{
  switch (xdrs->x_op) {
  case XDR_ENCODE:
    {
      A el, *elp;
      if (!(elp = v.get (i, &el)))
	return false;
      return rpc_traverse (xdrs, *elp);
    }
  case XDR_DECODE:
    {
      A el, *elp;
      elp = alloc_temporary<A> (el);
      if (!rpc_traverse (xdrs, *elp)) {
	dealloc_temporary<A> (elp);
	return false;
      }
      return v.set (unwrap_tmpl<A> (elp), i);
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
    if (!obj.shrink (size)) {
      PyErr_SetString (PyExc_RuntimeError,
		       "unexepected failure for array shrink");
      return false;
    }

  for (u_int i = 0; i < size; i++) 
    if (!rpc_traverse_slot (t, obj, i))
      return false;

  return true;
}

//
//
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
//  RPC Program
//    - wrapper around the libasync type

struct py_rpc_program_t {
  PyObject_HEAD
  const rpc_program *prog;
  const py_rpcgen_table_t *pytab;
};

//
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
// rpc_str methods
//
template<size_t M> char *
py_rpc_str<M>::get (size_t *sz) const
{
  char *ret;
  int i;
  if ( PyString_AsStringAndSize (_obj, &ret, &i) < 0) {
    return NULL;
  }
  *sz = i;
  if (*sz >= maxsize) {
    PyErr_SetString (PyExc_OverflowError, 
		     "Length of string exceeded\n");
    *sz = maxsize;
  }
  return ret;
}

template<size_t m> bool 
py_rpc_str<m>::set (char *buf, size_t len)
{
  Py_XDECREF (_obj);
  if (len > maxsize) {
    PyErr_SetString (PyExc_TypeError, 
		     "Length of string exceeded; value trunc'ed\n");
    len = maxsize;
  }
  return (_obj = PyString_FromStringAndSize (buf, len));
}

template<size_t m>  bool
py_rpc_str<m>::safe_set_obj (PyObject *in)
{
  PyObject *out = py_rpc_str_convert_py2py (in);
  if (!out) return false;
  return set_obj (out);
}
//
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
// py_rpc_vec methods
//

template<class T, size_t m> int
py_rpc_vec<T,m>::size ()
{
  int l = PyList_Size (_obj);
  if (l < 0) {
    PyErr_SetString (PyExc_RuntimeError,
		     "got negative list length from vector");
    return -1;
  }
  _sz = l;
  return l;
}

template<class T, size_t m> T *
py_rpc_vec<T,m>::get (int i, T *out) 
{
  assert (i < _sz && i >= 0);
  PyObject *in = PyList_GetItem (_obj, i);
  if (!in)
    return false;
  return assign_py_to_c (*out, in);
}

template<class T, size_t m> bool
py_rpc_vec<T,m>::set (PyObject *el, int i)
{
  int rc;
  if (i < _sz) {
    rc = PyList_SetItem (_obj, i, el);
  } else if (i == _sz) {
    rc = PyList_Append (_obj, el);
    _sz ++;
  } else {
    PyErr_SetString (PyExc_IndexError, "out-of-order list insertion");
    return false;
  }
  return (rc >= 0);
}

template<class T, size_t m> bool
py_rpc_vec<T,m>::shrink (size_t n)
{
  assert (n <= _sz);
  _sz -= n;
  int rc = PyList_SetSlice (_obj, _sz, _sz + n, NULL);
  return (rc >= 0);
}

//
//-----------------------------------------------------------------------

// XXX trash heap
#if 0
template<class T> inline bool
assign_py_to_c (T &t, PyObject *o)
{
  PyErr_SetString (PyExc_RuntimeError, "assign_py_to_c not defined");
  return false;
}
#endif

#endif
