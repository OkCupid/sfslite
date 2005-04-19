
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

#define INT_DO_ALL_C(T, s)                                      \
DEFXDR (py_##T);                                                \
RPC_PRINT_GEN (py_##T, sb.fmt ("0x%" s , obj.get ()));          \
RPC_PRINT_DEFINE (py_##T);                                      \
INT_RPC_TRAVERSE(T)                    

INT_DO_ALL_C (u_int32_t, "x");
INT_DO_ALL_C (int32_t, "d");

#if SIZEOF_LONG != 8
INT_DO_ALL_C (u_int64_t, "qx");
INT_DO_ALL_C (int64_t, "qd");
# else /* SIZEOF_LONG == 8 */
INT_DO_ALL_C (u_int64_t, "lx");
INT_DO_ALL_C (int64_t, "ld");
#endif /* SIZEOF_LONG == 8 */




PyObject *
convert_error (PyObject *in, PyObject *rpc_exception)
{
  PyErr_SetString (rpc_exception, "undefined RPC procedure called");
  return NULL;
}

PyObject *
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

PyObject *
py_u_int32_t_convert (PyObject *in, PyObject *rpc_exception)
{
  PyObject *out = NULL;
  assert (in);
  if (PyInt_Check (in))
    out = PyLong_FromLong(PyInt_AsLong (in));
  else if (!PyLong_Check (in)) {
    PyErr_SetString (PyExc_TypeError, "expect an integer or long");
  } else {
    out = in;
  }
  return out;
}


py_rpcgen_table_t py_rpcgen_error = 
{
  convert_error, convert_error, unwrap_error, dealloc_error
};
