
#include "py_util.h"


bool
assure_callable (PyObject *obj)
{
  if (! PyCallable_Check (obj)) {
    PyErr_SetString (PyExc_TypeError, "expected callable object");
    return false;
  }
  return true;
}

void
py_throwup ()
{
  if (PyErr_Occurred ()) {
    PyErr_Print ();
    exit (2);
  }
}
