
#include "py_rpctypes.h"


#define INT_RPC_TRAVERSE(T)                                       \
bool                                                              \
rpc_traverse (XDR *xdrs, pyw_##T &obj)                            \
{                                                                 \
  T raw = obj.get ();                                             \
  bool rc = rpc_traverse (xdrs, raw);                             \
  if (rc) obj.set (raw);                                          \
  return rc;                                                      \
}

#define ALLOC_DEFN(T)                                           \
void *                                                          \
T##_alloc ()                                                    \
{                                                               \
   return New T;                                                \
}

#define XDR_DEFN(T)						\
BOOL								\
xdr_##T (XDR *xdrs, void *objp)				        \
{								\
  return rpc_traverse (xdrs, *static_cast<T *> (objp));	        \
}								\

#define INT_DO_ALL_C(T, s, P)                                   \
ALLOC_DEFN (pyw_##T);                                           \
XDR_DEFN (pyw_##T);                                             \
RPC_PRINT_GEN (pyw_##T, sb.fmt ("0x%" s , obj.get ()));         \
RPC_PRINT_DEFINE (pyw_##T);                                     \
INT_RPC_TRAVERSE(T);

INT_DO_ALL_C (u_int32_t, "x", UnsignedLong);
INT_DO_ALL_C (int32_t, "d", Long);

#if SIZEOF_LONG != 8
INT_DO_ALL_C (u_int64_t, "qx", UnsignedLong );
INT_DO_ALL_C (int64_t, "qd", Long);
# else /* SIZEOF_LONG == 8 */
INT_DO_ALL_C (u_int64_t, "lx", UnsignedLong);
INT_DO_ALL_C (int64_t, "ld", Long );
#endif /* SIZEOF_LONG == 8 */

//
// Allocat pyw_void type
//
ALLOC_DEFN(pyw_void);
XDR_DEFN(pyw_void);
RPC_PRINT_GEN (pyw_void, sb.fmt ("<void>"));
RPC_PRINT_DEFINE (pyw_void);

bool
rpc_traverse (XDR *xdrs, pyw_void &v)
{
  return true;
}


void *
convert_error (PyObject *in, PyObject *rpc_exception)
{
  PyErr_SetString (rpc_exception, "undefined RPC procedure called");
  return NULL;
}

PyObject *
unwrap_error (void *)
{
  return NULL;
}


py_rpcgen_table_t py_rpcgen_error = 
{
  convert_error, convert_error, unwrap_error
};

