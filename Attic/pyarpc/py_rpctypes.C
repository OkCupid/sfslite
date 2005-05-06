
#include "py_rpctypes.h"


#define INT_RPC_TRAVERSE(T)                                       \
bool                                                              \
rpc_traverse (XDR *xdrs, pyw_##T &obj)                            \
{                                                                 \
  T tmp;                                                          \
  switch (xdrs->x_op) {                                           \
  case XDR_ENCODE:                                                \
    if (!obj.get (&tmp)) return false;                            \
  default:                                                        \
    break;                                                        \
  }                                                               \
  bool rc = rpc_traverse (xdrs, tmp);                             \
  if (rc) obj.set (tmp);                                          \
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
PY_RPC_PRINT_GEN (pyw_##T, sb.fmt ("0x%" s , obj.get ()));      \
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


pyw_base_t *
wrap_error (PyObject *in, PyObject *rpc_exception)
{
  PyErr_SetString (rpc_exception, "undefined RPC procedure called");
  return NULL;
}


py_rpcgen_table_t py_rpcgen_error = 
{
  wrap_error, wrap_error
};


bool
pyw_base_t::clear ()
{
  if (_obj) {
    Py_XDECREF (_obj);
    _obj = NULL;
  }
  return true;
}

bool
pyw_base_t::set_obj (PyObject *o)
{
  PyObject *tmp = _obj;
  Py_INCREF (o);
  _obj = o;
  Py_XDECREF (tmp);
  return true;
}

PyObject *
pyw_base_t::get_obj ()
{
  Py_XINCREF (_obj);
  return _obj;
}


PyObject *
pyw_base_t::unwrap ()
{
  PyObject *ret = get_obj ();
  delete this;
  return ret;
}

bool
pyw_base_t::init ()
{
  _obj = NULL;
  _typ = NULL;
  return true;
}


bool
pyw_base_t::alloc ()
{
  if (_typ) {
    _obj = _typ->tp_new (_typ, NULL, NULL);
    if (!_obj) {
      PyErr_SetString (PyExc_MemoryError, "out of memory in allocator");
      return false;
    }
  } else {
    _obj = Py_None;
    Py_INCREF (Py_None);
  }
  return true;
}


bool
pyw_base_t::pyw_print_err (const strbuf &b, const str &prfx) const
{
  str s;
  switch (_err) {
  case PYW_ERR_NONE:
    return false;
  case PYW_ERR_TYPE:
    s = "TYPE";
    break;
  case PYW_ERR_BOUNDS:
    s = "BOUNDS";
    break;
  default:
    s = "unspeficied";
    break;
  }
  b << "<" << s << "-ERROR>;\n";
  return true;

}
