
#include "py_rpctypes.h"
#include "py_gen.h"

// $Id$

static void
py_rpc_ptr_t_dealloc (py_rpc_ptr_t *self)
{
  Py_DECREF (self->typ);
  Py_XDECREF (self->p);
  self->ob_type->tp_free (reinterpret_cast<PyObject *> (self));
}

PY_CLASS_NEW(py_rpc_ptr_t);

static bool
py_rpc_ptr_t_set_internal (py_rpc_ptr_t *self, PyObject *o)
{
  if (!PyObject_IsInstance (o, (PyObject *)self->typ)) {
    PyErr_SetString (PyExc_TypeError, "incorrect type for RPC ptr");
    return false;
  }

  PyObject *old = self->p;
  self->p = o;
  Py_XINCREF (o);
  Py_XDECREF (old);
  
  return true;
}

static int
py_rpc_ptr_t_init (py_rpc_ptr_t *self, PyObject *args, PyObject *kwds)
{
  PyObject *t = NULL;
  PyObject *o = NULL;
  static char *kwlist[] = { "typ", "obj", NULL };
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "O|O", kwlist, &t, &o))
    return -1;

  if (!PyType_Check (t)) {
    PyErr_SetString (PyExc_TypeError, "expected a python type object");
    return -1;
  }
  self->typ = reinterpret_cast<PyTypeObject *> (t);
  Py_INCREF (self->typ);

  if (!py_rpc_ptr_t_set_internal (self, o))
    return -1;
  return 0;
}

static PyObject *
py_rpc_ptr_t_set (py_rpc_ptr_t *self, PyObject *args, PyObject *kwds)
{
  PyObject *o = NULL;
  static char *kwlist[] = { "obj", NULL };
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "O", kwlist, &o))
    return NULL;

  if (!py_rpc_ptr_t_set_internal (self, o))
    return NULL;

  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
py_rpc_ptr_t_get (py_rpc_ptr_t *self)
{
  PyObject *o = self->p ? self->p : Py_None;
  Py_INCREF (o);
  return o;
}

static PyObject *
py_rpc_ptr_t_alloc (py_rpc_ptr_t *self)
{
  if (!self->alloc ())
    return NULL;
  
  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
py_rpc_ptr_t_clear (py_rpc_ptr_t *self)
{
  self->clear ();
  Py_INCREF (Py_None);
  return Py_None;
}

static PyMethodDef py_rpc_ptr_t_methods[] = {
  { "get", (PyCFunction)py_rpc_ptr_t_get,  METH_NOARGS,
    PyDoc_STR ("get the objet that an RPC pointer points to") },
  { "alloc", (PyCFunction)py_rpc_ptr_t_alloc, METH_NOARGS,
    PyDoc_STR ("allocate the object that the pointer points to") },
  { "set", (PyCFunction)py_rpc_ptr_t_set, METH_VARARGS,
    PyDoc_STR ("set the object that the pointer points to") },
  { "clear", (PyCFunction)py_rpc_ptr_t_clear, METH_NOARGS,
    PyDoc_STR ("clear out the pointer") },
 {NULL}
};

PY_CLASS_DEF3(py_rpc_ptr_t, "async.rpc_ptr", 1, dealloc, -1, 
	      "RPC ptr object", methods, 0, 0, init, new, 0, 
	      0,0,0)
