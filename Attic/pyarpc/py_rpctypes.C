
#include "py_rpctypes.h"


#define INT_RPC_TRAVERSE(T)                                       \
bool                                                              \
rpc_traverse (XDR *xdrs, py_##T &obj)                             \
{                                                                 \
  T raw = obj.get ();                                             \
  bool rc = rpc_traverse (xdrs, raw);                             \
  if (rc) obj.set (raw);                                          \
  return rc;                                                      \
}

#define DEFXDR(type)						\
BOOL								\
xdr_##type (XDR *xdrs, void *objp)				\
{								\
  return rpc_traverse (xdrs, *static_cast<type *> (objp));	\
}								\
void *								\
type##_alloc ()							\
{								\
  return New type;						\
}

#define INT_CONVERT(T,P)                                        \
PyObject *                                                      \
T##_convert_py2py (PyObject *in)                                \
{                                                               \
  PyObject *out = NULL;                                         \
  assert (in);                                                  \
  if (PyInt_Check (in))                                         \
    out = PyLong_FromLong (PyInt_AsLong (in));                  \
  else if (!PyLong_Check (in)) {                                \
    PyErr_SetString (PyExc_TypeError,                           \
		     "expect an integer or long");              \
  } else {                                                      \
    out = in;                                                   \
    Py_INCREF (out);                                            \
  }                                                             \
  return out;                                                   \
}                                                               \
void *                                                   \
T##_convert (PyObject *in, PyObject *rpc_exception)             \
{                                                               \
  PyObject *out = T##_convert_py2py (in);                       \
  if (!out) return NULL;                                        \
  py_rpc_base_t *ret = static_cast<py_rpc_base_t *> (T##_alloc ());      \
  ret->set_obj (out);                                           \
  return ret;                                                   \
}

#define INT_DO_ALL_C(T, s, P)                                   \
DEFXDR (py_##T);                                                \
RPC_PRINT_GEN (py_##T, sb.fmt ("0x%" s , obj.get ()));          \
RPC_PRINT_DEFINE (py_##T);                                      \
INT_RPC_TRAVERSE(T)                                             \
INT_CONVERT(py_##T, P)                     

INT_DO_ALL_C (u_int32_t, "x", UnsignedLong);
INT_DO_ALL_C (int32_t, "d", Long);

#if SIZEOF_LONG != 8
INT_DO_ALL_C (u_int64_t, "qx", UnsignedLong );
INT_DO_ALL_C (int64_t, "qd", Long);
# else /* SIZEOF_LONG == 8 */
INT_DO_ALL_C (u_int64_t, "lx", UnsignedLong);
INT_DO_ALL_C (int64_t, "ld", Long );
#endif /* SIZEOF_LONG == 8 */


void *
convert_error (PyObject *in, PyObject *rpc_exception)
{
  PyErr_SetString (rpc_exception, "undefined RPC procedure called");
  return NULL;
}

void *
void_convert (PyObject *in, PyObject *rpc_exception)
{
  if (in && in != Py_None) {
    PyErr_SetString (PyExc_TypeError, "expected void argument");
    return NULL;
  }
  return Py_None;
}

PyObject *
void_unwrap (void *o)
{
  Py_INCREF (Py_None);
  return Py_None;
}

void
py_rpc_base_t::clear ()
{
  if (_obj) {
    Py_XDECREF (_obj);
    _obj = NULL;
  }
}

bool
py_rpc_base_t::set_obj (PyObject *o)
{
  PyObject *tmp = _obj;
  _obj = o; // no INCREF since usually just constructed
  Py_XDECREF (tmp);
  return true;
}

PyObject *
py_rpc_base_t::unwrap ()
{
  PyObject *ret = _obj;
  Py_INCREF (ret);
  if (!_onstack)
    delete this;
  return ret;
}



py_rpcgen_table_t py_rpcgen_error = 
{
  convert_error, convert_error, unwrap_error, decref_error, decref_error
};
