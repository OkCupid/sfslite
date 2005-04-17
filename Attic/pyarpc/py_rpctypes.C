
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
  typecheck_error,
  typecheck_error
};

int
py_u_int32_t_typecheck (PyObject *o)
{
  if (!o)
    return -1;
  return (PyInt_Check (o) || PyLong_Check (o)) ? 1 : 0;
}
