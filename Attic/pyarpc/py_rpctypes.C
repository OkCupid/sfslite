
#include "py_rpctypes.h"

bool
rpc_traverse (XDR *xdrs, py_u_int32_t &obj)
{
  switch (xdrs->x_op) {
  case XDR_ENCODE:
    return xdr_putint (xdrs, obj.get ());
  case XDR_DECODE: 
    {
      u_int32_t tmp;
      bool rc = xdr_getint (xdrs, tmp);
      if (rc) 
	obj.set (tmp);
      return rc;
    }
  default:
    return true;
  }
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


DEFXDR (py_u_int32_t);
RPC_PRINT_GEN (py_u_int32_t, sb.fmt ("0x%x", obj.get ()))
RPC_PRINT_DEFINE (py_u_int32_t)


py_rpcgen_table_t py_rcgen_error = 
{
  convert_error
};

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

