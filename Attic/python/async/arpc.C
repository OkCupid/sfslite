// -*-c++-*-
#include <Python.h>
#include "structmember.h"
#include "async.h"
#include "arpc.h"
#include "py_rpctypes.h"
#include "py_gen.h"

static PyObject *AsyncXDR_Exception;
static PyObject *AsyncRPC_Exception;


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
  py_rpc_program_t *py_prog;
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
  
  self->py_prog = NULL;

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
  self->py_prog = (py_rpc_program_t *)prog;
  Py_INCREF (self->py_prog);

  self->cli = aclnt::alloc (p_x->x, *self->py_prog->prog);
  
  return 0;
}

static void
py_aclnt_t_call_cb (PyObject *arg, PyObject *res, PyObject *cb, clnt_stat e)
{
  PyObject *arglist = Py_BuildValue ("(iO)", (int )e, res);
  PyObject_CallObject (cb, arglist);
  Py_DECREF (arglist);
  Py_XDECREF (arg);
  Py_XDECREF (cb);
}

static PyObject *
py_aclnt_t_call (py_aclnt_t *self, PyObject *args)
{
  int procno = 0;
  PyObject *rpc_args_in = NULL;
  PyObject *rpc_args_out = NULL;
  PyObject *cb = NULL ;
  PyObject *res = NULL;
  callbase *b = NULL;

  if (!PyArg_ParseTuple (args, "i|OO", &procno, &rpc_args_in, &cb))
    return NULL;

  // now check all arguments
  if (procno >= self->py_prog->prog->nproc) {
    PyErr_SetString (AsyncRPC_Exception, "given RPC proc is out of range");
    goto fail;
  }

  // XXX how do you tell if an object is callable?
  if (cb && !PyCallable_Check (cb)) {
    PyErr_SetString (PyExc_TypeError, "expected a callable object as 3rd arg");
    goto fail;
  }

  if (!rpc_args_in) {
    rpc_args_out = (PyObject *)self->py_prog->prog->tbl[procno].alloc_arg ();
  } else {
    if (!(rpc_args_out =
	  self->py_prog->pytab[procno].convert_arg (rpc_args_in,
						    AsyncRPC_Exception)))
      goto fail;
    Py_INCREF (rpc_args_out);
  }


  res = (PyObject *)self->py_prog->prog->tbl[procno].alloc_res ();

  if (cb)
    Py_INCREF (cb);
  b = self->cli->call (procno, rpc_args_out, res, 
		       wrap (py_aclnt_t_call_cb, rpc_args_out, res, cb));
  if (!b) 
    goto fail;

  // XXX eventually return the callbase to cancel a call
  Py_INCREF (Py_None);
  return Py_None;
 fail:
  Py_XDECREF (rpc_args_out);
  Py_XDECREF (res);
  return NULL;
}

static PyMethodDef py_aclnt_t_methods[] = {
  { "call", (PyCFunction)py_aclnt_t_call, METH_VARARGS,
    PyDoc_STR ("aclnt::call function from libasync") },
  {NULL}
};

PY_CLASS_DEF(py_aclnt_t, "arpc.aclnt", 1, dealloc, -1, 
	     "aclnt object wrapper", methods, 0, 0, init, new, 0);

static PyMethodDef module_methods[] = {
  { NULL }
};

#define INS(x)                                                       \
if ((rc = PyModule_AddIntConstant (m, #x, (long )x)) < 0) return rc

static int
all_ins (PyObject *m)
{
  int rc;
  INS (RPC_SUCCESS);
  INS (RPC_CANTENCODEARGS);
  INS (RPC_CANTDECODERES);
  INS (RPC_CANTSEND);
  INS (RPC_CANTRECV);
  INS (RPC_TIMEDOUT);
  INS (RPC_VERSMISMATCH);
  INS (RPC_AUTHERROR);
  INS (RPC_PROGUNAVAIL);
  INS (RPC_PROGVERSMISMATCH);
  INS (RPC_PROCUNAVAIL);
  INS (RPC_CANTDECODEARGS);
  INS (RPC_SYSTEMERROR);
  INS (RPC_NOBROADCAST);
  INS (RPC_UNKNOWNHOST);
  INS (RPC_UNKNOWNPROTO);
  INS (RPC_UNKNOWNADDR);
  INS (RPC_RPCBFAILURE);
  INS (RPC_PROGNOTREGISTERED);
  INS (RPC_N2AXLATEFAILURE);
  INS (RPC_FAILED);
  INS (RPC_INTR);
  INS (RPC_TLIERROR);
  INS (RPC_UDERROR);
  INS (RPC_INPROGRESS);
  INS (RPC_STALERACHANDLE);
  return 1;
}

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

  if (!import_async_exceptions (&AsyncXDR_Exception, &AsyncRPC_Exception))
    return;

  PyObject *errmod = PyImport_ImportModule ("async.err");
  if (!errmod)
    return;

  m = Py_InitModule3 ("async.arpc", module_methods,
                      "arpc wrappers for libasync");

  if (m == NULL)
    return;

  if (all_ins (m) < 0)
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

