
#include "py_gen.h"

static bool
import_exception (PyObject *m, const char *e, PyObject **d)
{
  PyObject *x;
  x = PyObject_GetAttrString (m, "AsyncXDRException");
  if (!x) {
    PyErr_SetString (PyExc_TypeError,
		     "Cannot load exception types from aysnc.err");
    return false;
  } else {
    if (d)
      *d = x;
  }
  return true;
}

bool
import_async_exceptions (PyObject **xdr, PyObject **rpc)
{
  PyObject *module = PyImport_ImportModule ("async.err");
  bool rc = true;
  if (!module) 
    rc = false;
  else {
    if (!import_exception (module, "AsyncXDRException", xdr) ||
	!import_exception (module, "AsyncRPCException", rpc))
      rc = false;
  }

  Py_DECREF (module);
  return rc;
}
