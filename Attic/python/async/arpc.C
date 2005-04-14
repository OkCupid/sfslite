

// -*-c++-*-
#include <Python.h>
#include "structmember.h"
#include "axprt.h"
#include "arpc.h"

struct py_axprt_t {
  ptr<axprt> x;
};

static void
py_axprt_t_dealloc (py_axprt_t *self)
{
  x->self = NULL;
}

static PyObject *
py_axprt_t_new (PyTypeObject *type, PyObject *args, PyObject *kws)
{
  py_axprt_t *self;
  if (!(self = (py_axprt_t *)type->tp_alloc (type, 0)))
    return NULL;
  return (PyObject *)self;
}



static PyTypeObject py_axprt_t_Type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,                         /*ob_size*/
  "async.axprt",               /*tp_name*/
  sizeof(py_axprt_t),             /*tp_basicsize*/
  0,                         /*tp_itemsize*/
  (destructor)py_axprt_t_dealloc, /*tp_dealloc*/
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
  "py_axprt_t object",           /* tp_doc */
  0,		               /* tp_traverse */
  0,		               /* tp_clear */
  0,		               /* tp_richcompare */
  0,		               /* tp_weaklistoffset */
  0,		               /* tp_iter */
  0,		               /* tp_iternext */
  py_axprt_t_methods,             /* tp_methods */
  py_axprt_t_members,             /* tp_members */
  py_axprt_t_getsetters,          /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)py_axprt_t_init,      /* tp_init */
  0,                         /* tp_alloc */
  py_axpr_t_new,                 /* tp_new */
};

