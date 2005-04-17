// -*-c++-*-

#ifndef __PY_GEN_INCLUDED_H__
#define __PY_GEN_INCLUDED_H__

#include <Python.h>
#include "structmember.h"

#define PY_ABSTRACT_CLASS_NEW(T,Q)                               \
static PyObject *                                                \
T##_new (PyTypeObject *type, PyObject *args, PyObject *kws)      \
{                                                                \
  PyErr_SetString(PyExc_TypeError,                               \
		  "The " Q " type cannot be instantiated");      \
  return NULL;                                                   \
} 

#define PY_CLASS_NEW(T)                                          \
PyObject *                                                       \
T##_new (PyTypeObject *type, PyObject *args, PyObject *kwds)     \
{                                                                \
  T *self;                                                       \
  self = (T *)type->tp_alloc (type, 0);                          \
  return (PyObject *)self;                                       \
}
  

#define PY_TP_FLAGS(flags) \
  (flags == -1 ? (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE) : flags)

#define PY_CLASS_DEF(T, pyname, basicsize, dealloc, flags, doc, methods,  \
                     members, getsetters, init, new, base)                \
int T##_0 = 0;                                                            \
static PyTypeObject T##_Type = {                                          \
  PyObject_HEAD_INIT(&PyType_Type)                                        \
  0,                                                /* ob_size*/          \
  pyname ,                                          /* tp_name*/          \
  (basicsize ? sizeof (T) : 0),                     /* tp_basicsize*/     \
  0,                                                /* tp_itemsize*/      \
  (destructor)T##_##dealloc,                        /* tp_dealloc*/       \
  0,                                                /* tp_print*/         \
  0,                                                /* tp_getattr*/       \
  0,                                                /* tp_setattr*/       \
  0,                                                /* tp_compare*/       \
  0,                                                /* tp_repr*/          \
  0,                                                /* tp_as_number*/     \
  0,                                                /* tp_as_sequence*/   \
  0,                                                /* tp_as_mapping*/    \
  0,                                                /* tp_hash */         \
  0,                                                /* tp_call */         \
  0,                                                /* tp_str */          \
  0,                                                /* tp_getattro*/      \
  0,                                                /* tp_setattro*/      \
  0,                                                /* tp_as_buffer */    \
  PY_TP_FLAGS(flags),                               /* tp_flags */        \
  PyDoc_STR(doc),                                   /* tp_doc */          \
  0,		                                    /* tp_traverse */     \
  0,		                                    /* tp_clear */        \
  0,		                                    /* tp_richcompare */  \
  0,		                                    /* tp_weaklistoffset*/\
  0,		                                    /* tp_iter */         \
  0,		                                    /* tp_iternext */     \
  (PyMethodDef *)T##_##methods,                     /* tp_methods */      \
  (PyMemberDef *)T##_##members,                     /* tp_members */      \
  (PyGetSetDef *)T##_##getsetters,                  /* tp_getset */       \
  (PyTypeObject *)base,                             /* tp_base */         \
  0,                                                /* tp_dict */         \
  0,                                                /* tp_descr_get */    \
  0,                                                /* tp_descr_set */    \
  0,                                                /* tp_dictoffset */   \
  (initproc)T##_##init,                             /* tp_init */         \
  0,                                                /* tp_alloc */        \
  T##_##new                                         /* tp_new */          \
};                                                               

#define PY_ABSTRACT_CLASS(T,Q)                                            \
  PY_ABSTRACT_CLASS_NEW(T,Q)                                              \
  PY_CLASS_DEF(T, Q, 0, 0, -1, Q " base abstract object", 0, 0, 0, 0, new, 0)

bool import_async_exceptions (PyObject **xdr, PyObject **rpc = NULL);


#endif
