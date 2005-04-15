

// -*-c++-*-
#include <Python.h>
#include "structmember.h"
#include "async.h"
#include "arpc.h"

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

static PyObject *
py_axprt_t_new (PyTypeObject *type, PyObject *args, PyObject *kws)
{
  PyErr_SetString(PyExc_TypeError,
		  "The axprt type cannot be instantiated");
  return NULL;
}

static PyTypeObject py_axprt_t_Type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,                         /*ob_size*/
  "async.axprt",             /*tp_name*/
  0,                         /*tp_basicsize*/
  0,                         /*tp_itemsize*/
  0,                         /*tp_dealloc*/
  0,                         /*tp_print*/
  0,                         /*tp_getattr*/
  0,                         /*tp_setattr*/
  0,                         /*tp_compare*/
  0,                         /*tp_repr*/
  0,                         /*tp_as_number*/
  0,                         /*tp_as_sequence*/
  0,                         /*tp_as_mapping*/
  0,                         /*tp_hash */
  0,                         /*tp_call*/
  0,                         /*tp_str*/
  0,                         /*tp_getattro*/
  0,                         /*tp_setattro*/
  0,                         /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
  "py_axprt_t object",       /* tp_doc */
  0,		             /* tp_traverse */
  0,		             /* tp_clear */
  0,		             /* tp_richcompare */
  0,		             /* tp_weaklistoffset */
  0,		             /* tp_iter */
  0,		             /* tp_iternext */
  0,                         /* tp_methods */
  0,                         /* tp_members */
  0,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  0,                         /* tp_init */
  0,                         /* tp_alloc */
  py_axprt_t_new,            /* tp_new */
};

PyObject *
py_axprt_stream_t_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  py_axprt_stream_t *self;

  self = (py_axprt_stream_t *)type->tp_alloc (type, 0);
  return (PyObject *)self;
}

PyObject *
py_aclnt_t_new (PyObject *type, PyObject *args, PyObject *kwds)
{
  py_aclnt_t *self;
  self = (py_aclnt_t *)type->tp_alloc (type, 0);
  return (PyObject *)self;
}


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

static PyTypeObject py_axprt_stream_t_Type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,                         /*ob_size*/
  "async.axprt_stream",      /*tp_name*/
  sizeof(py_axprt_stream_t), /*tp_basicsize*/
  0,                         /*tp_itemsize*/
  (destructor)py_axprt_stream_t_dealloc, /*tp_dealloc*/
  0,                         /*tp_print*/
  0,                         /*tp_getattr*/
  0,                         /*tp_setattr*/
  0,                         /*tp_compare*/
  0,                         /*tp_repr*/
  0,                         /*tp_as_number*/
  0,                         /*tp_as_sequence*/
  0,                         /*tp_as_mapping*/
  0,                         /*tp_hash */
  0,                         /*tp_call*/
  0,                         /*tp_str*/
  0,                         /*tp_getattro*/
  0,                         /*tp_setattro*/
  0,                         /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
  "py_axprt_stream_t object",       /* tp_doc */
  0,		             /* tp_traverse */
  0,		             /* tp_clear */
  0,		             /* tp_richcompare */
  0,		             /* tp_weaklistoffset */
  0,		             /* tp_iter */
  0,		             /* tp_iternext */
  0,                         /* tp_methods */
  0,                         /* tp_members */
  0,                         /* tp_getset */
  &py_axprt_t_Type,          /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)py_axprt_stream_t_init, /* tp_init */
  0,                         /* tp_alloc */
  py_axprt_stream_t_new,     /* tp_new */
};

static void
py_aclnt_t_dealloc (py_aclnt_t *self)
{
  self->cli = NULL;
}

static PyObject *
py_aclnt_t_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  py_aclnt_t *self;
  self = (py_aclnt_t *)type->tp_alloc (type, 0);
  return (PyObject *)self;
}

static int
py_aclnt_t_init (py_aclnt_t *self, PyObject *args, PyObject *kwds)
{
  PyObject *x = NULL;
  PyObject *prog = NULL;
  static char *kwlist[] = { "x", "prog", NULL };
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "OO", kwlist, &x, &prog))
    return -1;
  
  return 0;
}


static PyTypeObject py_aclnt_t_Type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,                         /*ob_size*/
  "async.aclnt",             /*tp_name*/
  sizeof(py_aclnt_t),        /*tp_basicsize*/
  0,                         /*tp_itemsize*/
  (destructor)py_aclnt_t_dealloc, /*tp_dealloc*/
  0,                         /*tp_print*/
  0,                         /*tp_getattr*/
  0,                         /*tp_setattr*/
  0,                         /*tp_compare*/
  0,                         /*tp_repr*/
  0,                         /*tp_as_number*/
  0,                         /*tp_as_sequence*/
  0,                         /*tp_as_mapping*/
  0,                         /*tp_hash */
  0,                         /*tp_call*/
  0,                         /*tp_str*/
  0,                         /*tp_getattro*/
  0,                         /*tp_setattro*/
  0,                         /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
  "py_aclnt_t object",       /* tp_doc */
  0,		             /* tp_traverse */
  0,		             /* tp_clear */
  0,		             /* tp_richcompare */
  0,		             /* tp_weaklistoffset */
  0,		             /* tp_iter */
  0,		             /* tp_iternext */
  0,                         /* tp_methods */
  0,                         /* tp_members */
  0,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)py_aclnt_t_init, /* tp_init */
  0,                         /* tp_alloc */
  py_aclnt_t_new,            /* tp_new */
};

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
  if (PyType_Ready (&py_axprt_t_Type) < 0)
    return;
  if (PyType_Ready (&py_axprt_stream_t_Type) < 0)
    return;

  m = Py_InitModule3 ("async.arpc", module_methods,
                      "Python/rpc/XDR module for example5.");

  if (m == NULL)
    return;

  Py_INCREF (&py_axprt_t_Type);
  Py_INCREF (&py_axprt_stream_t_Type);
  PyModule_AddObject (m, "axprt", (PyObject *)&py_axprt_t_Type);
  PyModule_AddObject (m, "axprt_stream", (PyObject *)&py_axprt_stream_t_Type);
}

