

// -*-c++-*-
#include <Python.h>
#include "structmember.h"
#include "async.h"
#include "arpc.h"
#include "py_rpctypes.h"
#include "py_gen.h"


struct py_axprt_t {
  PyObject_HEAD
  ptr<axprt> x;
  void dealloc () { x = NULL; }
};

struct py_axprt_stream_t : public py_axprt_t {

};

struct py_axprt_dgram_t : public py_axprt_t {

};

struct py_aclnt_t {
  PyObject_HEAD
  ptr<aclnt> cli;
};

PY_ABSTRACT_CLASS(py_rpc_program_t, "arpc.prc_program");
PY_ABSTRACT_CLASS(py_axprt_t, "arpc.axprt");

PY_CLASS_NEW(py_axprt_stream_t);
PY_CLASS_NEW(py_aclnt_t);

static int
py_axprt_stream_t_init (py_axprt_stream_t *self, PyObject *args, 
			PyObject *kwds)
{
  static char *kwlist[] = { "fd", "defps", NULL };
  int fd;
  int defps = 0;

  if (! PyArg_ParseTupleAndKeywords (args, kwds, "i|i", kwlist,
				     &fd, &defps))
    return -1;

  if (defps) {
    self->x = axprt_stream::alloc (fd, defps);
  } else {
    self->x = axprt_stream::alloc (fd);
  }
  
  return 0;
}

static void
py_axprt_stream_t_dealloc (py_axprt_stream_t *self)
{
  self->x = NULL;
  self->ob_type->tp_free ((PyObject *)self);
}

PY_CLASS_DEF(py_axprt_stream_t, "arpc.axprt_stream", 1, dealloc, -1,
	     "py_axprt_stream_t object", 0, 0, 0, init, new, 
	     &py_axprt_t_Type);

static void
py_aclnt_t_dealloc (py_aclnt_t *self)
{
  self->cli = NULL;
  self->ob_type->tp_free ((PyObject *)self);
}

static int
py_aclnt_t_init (py_aclnt_t *self, PyObject *args, PyObject *kwds)
{
  PyObject *x = NULL;
  PyObject *prog = NULL;
  PyObject *tmp;
  static char *kwlist[] = { "x", "prog", NULL };
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "OO", kwlist, &x, &prog))
    return -1;

  if (!x || !PyObject_IsInstance (x, (PyObject *)&py_axprt_t_Type)) {
    PyErr_SetString (PyExc_TypeError,
		     "aclnt expects arg 1 as an axprt transport");
    return -1;
  }

  if (!prog || 
      !PyObject_IsInstance (prog, (PyObject *)&py_rpc_program_t_Type)) {
    PyErr_SetString (PyExc_TypeError,
		     "aclnt expects arg 2 as an rpc_program");
    return -1;
  }
  py_axprt_t *p_x = (py_axprt_t *)x;
  py_rpc_program_t *p_prog = (py_rpc_program_t *)prog;

  self->cli = aclnt::alloc (p_x->x, *p_prog->prog);
  
  return 0;
}

PY_CLASS_DEF(py_aclnt_t, "arpc.aclnt", 1, dealloc, -1, 
	     "aclnt object wrapper", 0, 0, 0, init, new, 0);

static PyMethodDef module_methods[] = {
  { NULL }
};

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initarpc (void)
{
  PyObject* m;
  if (PyType_Ready (&py_axprt_t_Type) < 0 ||
      PyType_Ready (&py_axprt_stream_t_Type) < 0 ||
      PyType_Ready (&py_rpc_program_t_Type) < 0 ||
      PyType_Ready (&py_aclnt_t_Type) < 0)
    return;

  m = Py_InitModule3 ("async.arpc", module_methods,
                      "arpc wrappers for libasync");

  if (m == NULL)
    return;

  Py_INCREF (&py_axprt_t_Type);
  Py_INCREF (&py_axprt_stream_t_Type);
  Py_INCREF (&py_rpc_program_t_Type);
  Py_INCREF (&py_aclnt_t_Type);
  PyModule_AddObject (m, "axprt", (PyObject *)&py_axprt_t_Type);
  PyModule_AddObject (m, "axprt_stream", (PyObject *)&py_axprt_stream_t_Type);
  PyModule_AddObject (m, "rpc_program", (PyObject *)&py_rpc_program_t_Type);
  PyModule_AddObject (m, "aclnt", (PyObject *)&py_aclnt_t_Type);
}

