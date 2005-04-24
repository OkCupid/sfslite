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


//-----------------------------------------------------------------------
// Class Definitions
//

class pyw_base_t {
public:
  pyw_base_t () : _obj (NULL), _typ (NULL) {}
  pyw_base_t (PyTypeObject *t) : _obj (NULL), _typ (t) {}
  pyw_base_t (PyObject *o, PyTypeObject *t) : _obj (o), _typ (t) {}
  ~pyw_base_t () { Py_XDECREF (_obj); }

  // sometimes we allocate room for objects via Python's memory
  // allocation routines.  Doing so does not do "smart" initialization,
  // as C++ would do when you call "new."  We can mimic this behavior,
  // though, with a call to tp_alloc (), and then by calling the init()
  // functions.  Note that the init functions should not at all access
  // memory, since memory will be garbage.
  bool init (); 

  bool alloc ();
  bool clear ();
  PyObject *obj () { return _obj; }

  // takes a New reference (not a borrowed reference) and hence does
  // not INC reference count
  bool set_obj (PyObject *o);

  PyObject *get_obj ();
  const PyObject *get_const_obj () const { return _obj; }
  PyObject *unwrap () ;

protected:
  PyObject *_obj;
  PyTypeObject *_typ;
};

template<class W, class P>
class pyw_tmpl_t : public pyw_base_t {
public:
  pyw_tmpl_t (PyTypeObject *t) : pyw_base_t (t) {}
  pyw_tmpl_t (PyObject *o, PyTypeObject *t) : pyw_base_t (o, t) {}

  P *casted_obj () { return reinterpret_cast<P *> (_obj); }
  const P *const_casted_obj () const 
  { return reinterpret_cast<const P *> (_obj); }

  bool safe_set_obj (PyObject *in);
};

template<size_t M = RPC_INFINITY>
class pyw_rpc_str : public pyw_tmpl_t<pyw_rpc_str<M>, PyStringObject > 
{
public:
  pyw_rpc_str () : 
    pyw_tmpl_t<pyw_rpc_str<M>, PyStringObject > (&PyString_Type) {}
  pyw_rpc_str (PyObject *o) : 
    pyw_tmpl_t<pyw_rpc_str<M>, PyStringObject > (o, &PyString_Type) {}
  char *get (size_t *sz) const;
  const char * get () const { return PyString_AsString (_obj); }
  bool set (char *buf, size_t len);
  bool init ();
  enum { maxsize = M };
};

template<class T, size_t max>
class pyw_rpc_vec : public pyw_tmpl_t<pyw_rpc_vec<T, max>, PyListObject > 
{
public:
  pyw_rpc_vec () : 
    pyw_tmpl_t<pyw_rpc_vec<T, max>, PyListObject > (&PyList_Type), _sz (0) {}
  pyw_rpc_vec (PyObject *o) : 
    pyw_tmpl_t<pyw_rpc_vec<T, max>, PyListObject > (o, &PyList_Type), _sz (0) 
  {}

  int size ();
  bool get_slot (int i, T *out) ;
  bool set_slot (PyObject *el, int i);
  bool shrink (size_t sz);
  bool init ();
  enum { maxsize = max };
private:
  int _sz;
};

class pyw_void : public pyw_tmpl_t<pyw_void, PyObject>
{
public:
  pyw_void () : pyw_tmpl_t<pyw_void, PyObject> (NULL)
  {
    _obj = Py_None;
    Py_INCREF (_obj);
  }
  pyw_void (PyObject *o) : pyw_tmpl_t<pyw_void, PyObject> (o, NULL) {}
  bool init () { return true; }

};



//-----------------------------------------------------------------------
// py_rpcgen_table_t
//
//   we need to add just one more method to the basic rpcgen_table,
//   which is for wrapping generic Python objects in their appropriate
//   typed C++ wrappers.
//  

struct py_rpcgen_table_t {
  pyw_base_t *(*wrap_arg) (PyObject *arg, PyObject *rpc_exception);
};

extern py_rpcgen_table_t py_rpcgen_error;

//
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
// 
// Allocate / Deallocate / Decref / init / clear
//
// 

// for native types, we just call into the class, but compiled
// types don't have methods in their structs, so they will simply
// specialize this template
template<class T> inline bool py_init (T &t) { return t.init (); }
template<class T> inline bool py_clear (T &t) { return t.clear (); }

inline void *
pyw_rpc_str_alloc ()
{
  return New pyw_rpc_str<RPC_INFINITY> ();
}

#define ALLOC_DECL(T)                                           \
extern void * T##_alloc ();

template<size_t M> bool
pyw_rpc_str<M>::init ()
{
  if (!pyw_tmpl_t<pyw_rpc_str<M>, PyStringObject >::init ())
    return false;
  _typ = &PyString_Type;
  return true;
}

template<class T, size_t M> bool
pyw_rpc_vec<T,M>::init ()
{
  if (!pyw_tmpl_t<pyw_rpc_vec<T,M >, PyListObject >::init ())
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

ALLOC_DECL(pyw_void);

//
//
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// XDR wrapper functions
// 

#define XDR_DECL(T)                                            \
extern BOOL xdr_##T (XDR *xdrs, void *objp);

template<size_t n> inline bool
xdr_pyw_rpc_str (XDR *xdrs, void *objp)
{
  return rpc_traverse (xdrs, *static_cast<pyw_rpc_str<n> *> (objp));
}

template<class T, size_t n> inline bool
xdr_pyw_rpc_vec (XDR *xdrs, void *objp)
{
  return rpc_traverse (xdrs, *static_cast<pyw_rpc_vec<T,n> *> (objp));
}

//
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
// RPC Printing 
//

#define PY_RPC_TYPE2STR_DECL(T)			\
template<> struct rpc_type2str<pyw_##T> {	\
  static const char *type () { return #T; }	\
};

template<size_t n> const strbuf &
rpc_print (const strbuf &sb, const pyw_rpc_str<n> &pyobj,
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
// operators
//   - multipurpose classes the hold lots of specialized template calls
//     for the particular class
//


//-----------------------------------------------------------------------
// convert
//   - a python type into its C equivalent.  For compiled, complex
//     types, this will do a type check, and return the arg passed.
//     for wrapped objects, it will allocate the wrapper, and 
//     add the object into the wrapper
//

template<class W> struct converter_t {};

template<class W> pyw_base_t *
py_wrap (PyObject *in, PyObject *dummy)
{
  PyObject *out = converter_t<W>::convert (in);
  if (!out) return NULL;
  W * ret = New W (out);
  if (!ret) {
    PyErr_SetString (PyExc_MemoryError, "out of memory in py_wrap");
    return NULL;
  }
  return ret;
}

template<size_t m> 
struct converter_t<pyw_rpc_str<m> >
{
  static PyObject * convert (PyObject *in)
  {
    if (!PyString_Check (in)) {
      PyErr_SetString (PyExc_TypeError, "expected string type");
      return NULL;
    }
    if (PyString_Size (in) >= m) {
      PyErr_SetString (PyExc_OverflowError, 
		       "string exceeds predeclared limits");
      return NULL;
    }
    Py_INCREF (in);
    return in;
  }
};

template<class T, size_t m> struct converter_t<pyw_rpc_vec<T,m> >
{
  static PyObject * convert (PyObject *in)
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
};

template<> struct converter_t<pyw_void>
{
  static PyObject * convert (PyObject *in)
  {
    if (in && in != Py_None) {
      PyErr_SetString (PyExc_TypeError, "expected void argument");
      return NULL;
    }
    Py_INCREF (Py_None);
    return Py_None;
  }
};

PyObject *unwrap_error (void *);
pyw_base_t *wrap_error (PyObject *o, PyObject *e);

//
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
// Shortcuts for allocating integer types all at once
//


#define INT_XDR_CLASS(ctype,ptype)                               \
class pyw_##ctype : public pyw_tmpl_t<pyw_##ctype, PyLongObject> \
{                                                                \
public:                                                          \
  pyw_##ctype ()                                                 \
    : pyw_tmpl_t<pyw_##ctype, PyLongObject> (&PyLong_Type) {}    \
  ctype get () const                                             \
  {                                                              \
    if (!_obj) {                                                 \
      PyErr_SetString (PyExc_UnboundLocalError,                  \
                      "unbound int/long");                       \
      return NULL;                                               \
    }                                                            \
    return (PyLong_As##ptype (_obj));                            \
  }                                                              \
  bool set (ctype i) {                                           \
    Py_XDECREF (_obj);                                           \
    return (_obj = PyLong_From##ptype (i));                      \
  }                                                              \
  bool init ()                                                   \
  {                                                              \
    if (!pyw_tmpl_t<pyw_##ctype, PyLongObject>::init ())         \
      return false;                                              \
    _typ = &PyLong_Type;                                         \
    return true;                                                 \
  }                                                              \
};

inline PyObject *
convert_int (PyObject *in)
{
  PyObject *out = NULL;
  assert (in);
  if (PyInt_Check (in))
    out = PyLong_FromLong (PyInt_AsLong (in));
  else if (!PyLong_Check (in)) {
    PyErr_SetString (PyExc_TypeError,
		     "integer or long value expected");
  } else {
    out = in;
    Py_INCREF (out);
  }
  return out;
}

#define INT_CONVERTER(T)                                         \
template<> struct converter_t<T> {                               \
  static PyObject *convert (PyObject *in)                        \
   { return convert_int (in); }                                  \
};

#define RPC_TRAVERSE_DECL(ptype)                                 \
bool rpc_traverse (XDR *xdrs, ptype &obj);

#define INT_DO_ALL_H(ctype,ptype)                                \
XDR_DECL(pyw_##ctype)                                            \
INT_XDR_CLASS(ctype, ptype)                                      \
RPC_TRAVERSE_DECL(pyw_##ctype)                                   \
PY_RPC_TYPE2STR_DECL(ctype)                                      \
RPC_PRINT_TYPE_DECL(pyw_##ctype)                                 \
RPC_PRINT_DECL(pyw_##ctype)                                      \
INT_CONVERTER(pyw_##ctype);                                      \
ALLOC_DECL(pyw_##ctype);

INT_DO_ALL_H(u_int32_t, UnsignedLong);
INT_DO_ALL_H(int32_t, Long);
INT_DO_ALL_H(u_int64_t, UnsignedLongLong);
INT_DO_ALL_H(int64_t, LongLong);

RPC_TRAVERSE_DECL(pyw_void);
RPC_PRINT_TYPE_DECL (pyw_void);
RPC_PRINT_DECL(pyw_void);
PY_RPC_TYPE2STR_DECL(void);
XDR_DECL(pyw_void);
ALLOC_DECL(pyw_void);

//
//-----------------------------------------------------------------------




//-----------------------------------------------------------------------
// RPC traversal
//

template<size_t n> inline bool
rpc_traverse (XDR *xdrs, pyw_rpc_str<n> &obj)
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
rpc_traverse_slot (XDR *xdrs, pyw_rpc_vec<A,max> &v, int i)
{
  switch (xdrs->x_op) {
  case XDR_ENCODE:
    {
      A el;
      if (!v.get_slot (i, &el))
	return false;
      return rpc_traverse (xdrs, el);
    }
  case XDR_DECODE:
    {
      A el;
      if (!rpc_traverse (xdrs, el)) {
	return false;
      }
      return v.set_slot (el.get_obj (), i);
    }
  default:
    return false;
  }
}

template<class T, class A, size_t max> inline bool
rpc_traverse (T &t, pyw_rpc_vec<A,max> &obj)
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
pyw_rpc_str<M>::get (size_t *sz) const
{
  char *ret;
  int i;

  if (!_obj) {
    PyErr_SetString (PyExc_UnboundLocalError, "undefined string");
    return NULL;
  }


  if ( PyString_AsStringAndSize (_obj, &ret, &i) < 0) {
    PyErr_SetString (PyExc_RuntimeError,
		     "failed to access string data");
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
pyw_rpc_str<m>::set (char *buf, size_t len)
{
  Py_XDECREF (_obj);
  if (len > maxsize) {
    PyErr_SetString (PyExc_TypeError, 
		     "Length of string exceeded; value trunc'ed\n");
    len = maxsize;
  }
  return (_obj = PyString_FromStringAndSize (buf, len));
}

//
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
// py_rpc_vec methods
//

template<class T, size_t m> int
pyw_rpc_vec<T,m>::size ()
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

template<class T, size_t m> bool
pyw_rpc_vec<T,m>::get_slot (int i, T *out) 
{
  assert (i < _sz && i >= 0);
  if (!_obj) {
    PyErr_SetString (PyExc_UnboundLocalError, "unbound vector");
    return NULL;
  }
  PyObject *in = PyList_GetItem (_obj, i);
  if (!in)
    return false;
  return out->safe_set_obj (in);
}

template<class T, size_t m> bool
pyw_rpc_vec<T,m>::set_slot (PyObject *el, int i)
{
  int rc;
  if (!el) {
    PyErr_SetString (PyExc_UnboundLocalError, "NULL array slot assignment");
    return false;
  } else if (i < _sz) {
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
pyw_rpc_vec<T,m>::shrink (size_t n)
{
  assert (n <= _sz);
  _sz -= n;
  int rc = PyList_SetSlice (_obj, _sz, _sz + n, NULL);
  return (rc >= 0);
}

//
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
//

template<class W, class P> bool
pyw_tmpl_t<W,P>::safe_set_obj (PyObject *in)
{
  PyObject *out = converter_t<W>::convert (in);
  return out ? set_obj (out) : false;
}

//
//-----------------------------------------------------------------------


#endif
