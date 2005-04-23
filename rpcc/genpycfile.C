/* $Id$ */

/*
 *
 * Copyright (C) 1998 David Mazieres (dm@uun.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */

#include "rpcc.h"

str module;

typedef 
enum { PASS_ONE = 1, PASS_TWO = 2, PASS_THREE = 3, N_PASSES = 4}
pass_num_t;

static void
dump_obj_wrap ()
{
  aout << "struct PyObjWrap {\n"
       << "  bool scalar;         // =true if scalar\n"
       << "  bool fixed;          // =true if fixed\n"
       << "  int bound;           // max # of elements in vec\n"
       << "  PyObject *obj;       // actual object\n"
       << "};\n\n";
}

static str
pyc_type (const str &s)
{
  if (s == "void")
    return s;

  strbuf b;
  b << "py_" << s;
  return b;
}

static str
pyw_type (const str &s)
{
  strbuf b;
  b << "pyw_" << s;
  return b;
}

static bool
is_int_type (const str &s)
{
  return (s == "u_int32_t" || s == "int32_t" );
}

static bool
is_long_type (const str &s)
{
  return (s == "int64_t" || s == "u_int64_t" );
}

static bool
is_double_type (const str &s)
{
  return (s == "double"  || s == "quadruple");
}

static bool
is_number (const str &s)
{
  return is_double_type (s) || is_int_type (s) || is_long_type (s);
}

static str
py_type (const str &in)
{
  strbuf b;
  b << in << "_Type";
  return pyc_type (b);
}


static void
pmshl (str id)
{
  aout <<
    "void *" << id << "_alloc ();\n"
    XDR_RETURN " xdr_" << id << " (XDR *, void *);\n";
}

static str
decltype (const rpc_decl *d)
{
  str wt = pyw_type (d->type);
  strbuf b;
  if (d->type == "string") {
    b << pyw_type ("rpc_str") << "<" << d->bound << ">";
  } else if (d->type == "opaque")
    switch (d->qual) {
    case rpc_decl::ARRAY:
      b << pyw_type ("rpc_opaque") << "<" << d->bound << ">";
      break;
    case rpc_decl::VEC:
      b << pyw_type ("rpc_bytes") << "<" << d->bound << ">";
      break;
    default:
      panic ("bad rpc_decl qual for opaque (%d)\n", d->qual);
      break;
    }
  else
    switch (d->qual) {
    case rpc_decl::SCALAR:
      b << pyw_type (d->type) ;
      break;
    case rpc_decl::PTR:
      b << pyw_type ("rpc_ptr") << "<" << pyc_type (d->type) << ">";
      break;
    case rpc_decl::ARRAY:
      b << pyw_type ("rpc_array") << "<" << wt << ", " << d->bound << ">";
      break;
    case rpc_decl::VEC:
      b << pyw_type ("rpc_vec") << "<" << wt << ", " << d->bound << ">";
      break;
    default:
      panic ("bad rpc_decl qual (%d)\n", d->qual);
    }
  return b;
}

static void
pdecl (str prefix, const rpc_decl *d)
{
  str name = d->id;
  aout << prefix << decltype (d) << " " << name << ";\n";
}


static str
py_type (const rpc_decl *d)
{
  if (d->type == "string" || d->type == "opaque") {
    return "PyString_Type";
  } else {
    switch (d->qual) {
    case rpc_decl::SCALAR:
    case rpc_decl::PTR:
      if (is_int_type (d->type)) 
	return "PyInt_Type";
      else if (is_long_type (d->type))
	return "PyLong_Type";
      else if (is_double_type (d->type))
	return "PyDouble_Type";
      else
	return py_type (d->type);
    case rpc_decl::VEC:
    case rpc_decl::ARRAY:
      return "PyList_Type";
    default:
      break;
    }
  }
  return NULL;
}

static bool
is_scalar (const rpc_decl *d)
{
  return (d->qual == rpc_decl::SCALAR);
}

static bool
is_fixed (const rpc_decl *d)
{
  return (d->qual == rpc_decl::ARRAY);
}

static str
obj_in_class (const rpc_decl *d)
{
  strbuf b;
  b << d->id << ".obj ()";
  return b;
}

static str
bound (const rpc_decl *d)
{
  return (is_scalar (d) ? "0" : d->bound);
}

static str
py_typecheck_unqual (const str &typ, const str &v)
{
  strbuf b;
  if (typ == "string" || typ == "opqaue") {
    b << "PyString_Check (" << v << ")";
  } else if (is_number (typ)) {
    b << "PyNumber_Check (" << v << ")";
  } else {
    b << "PyObject_IsInstance (" << v << ", " << py_type (typ) << ")";
  }
  return b;
}

#if 0
static str
py_member_init (const rpc_decl *d)
{
  str pt = py_type (d->type);
  str ct = pyc_type (d->type);
  // XXX
  // for now, don't distinguish between VEC's and ARRAY's
  if (d->type == "string" || d->type == "opaque") 
    return "PyString_FromString (\"\")";
  else {
    switch (d->qual) {
    case rpc_decl::SCALAR:
      {
	if (is_int_type (d->type)) {
	  return "PyInt_FromLong (0)";
	} else if (is_long_type (d->type)) {
	  return "PyLong_FromLong (0)";
	} else if (is_double_type (d->type)) {
	  return "PyFloat_FromDouble (0.0)";
	} else {
	  strbuf b;
	  b << "PyObject_New (" << ct << ", " << pt << ")";
	  return b;
	}
      }
    case rpc_decl::PTR:
      return "Py_None";
    case rpc_decl::VEC:
    case rpc_decl::ARRAY:
      return "PyList_New (0)";
    default:
      panic ("Bad rpc_decl qual (%d)\n", d->qual);
    }
  }
  panic ("bad data type: %s\n", d->type.cstr ());
}
#endif

static void
dump_init_frag (str prfx, const rpc_decl *d)
{
  aout << prfx << "if (!self->" << d->id << ".init ()) {\n"
       << prfx << "  Py_DECREF (self);\n"
       << prfx << "  return NULL;\n"
       << prfx << "}\n"
       << "\n";
}

static str
get_inner_obj (const str &vn, const str &ct)
{
  strbuf bout;
  bout << "  if (!_obj) {\n"
       << "    PyErr_SetString (PyExc_RuntimeError,\n"
       << "                     \"null wrapped obj unexpected\");\n"
       << "    return false;\n"
       << "  }\n"
       << "  " << ct << " *" << vn 
       << " = reinterpret_cast<" << ct << " *> (_obj);\n" ;

  return bout;
}

static void
dump_hacked_trav (const rpc_struct *rs, const str &f)
{
  bool first = true;
  str ct = pyc_type (rs->id);
  str wt = pyw_type (rs->id);
  aout << "bool\n"
       << wt << "::" << f << "()\n"
       << "{\n"
       << get_inner_obj ("o", ct)
       << "  return (";
  for (const rpc_decl *rd = rs->decls.base (); rd < rs->decls.lim (); rd++) {
    if (!first)
      aout << " &&\n"
	   << "          ";
    else
      first = false;
    aout << "o->" << rd->id << "." << f << " ()";
  }
  aout << ");\n"
       << "}\n\n";
}

static void
dump_class_new_func (const rpc_struct *rs)
{
  str ct = pyc_type (rs->id);
  aout << "\nstatic PyObject *\n"
       << ct
       << "_new (PyTypeObject *type, PyObject *args, PyObject *kwds)\n"
       << "{\n"
       << "  " << ct << " *self;\n"
       << "  if (!(self = (" << ct << " *)type->tp_alloc (type, 0)))\n"
       << "    return NULL;\n";
  for (const rpc_decl *rd = rs->decls.base (); rd < rs->decls.lim (); rd++) {
    dump_init_frag ("  ", rd);
  }
  aout << "  return (PyObject *)self;\n"
       << "}\n\n";
}

static void
dump_class_init_func (const rpc_struct *rs)
{
  str ct = pyc_type (rs->id);
  aout << "static int\n"
       << ct << "_init (" << ct << " *self, PyObject *args, PyObject *kwds)\n"
       << "{\n"
       << "  PyObject *tmp;\n";
  for (const rpc_decl *rd = rs->decls.base (); rd < rs->decls.lim (); rd++) {
    aout << "  PyObject *" << rd->id << " = NULL;\n";
  }
  aout << "\n";
  aout << "  static char *kwlist[] = { ";
  bool first = true;
  for (const rpc_decl *rd = rs->decls.base (); rd < rs->decls.lim (); rd++) {
    if (first)
      first = false;
    else
      aout << ", ";
    aout << "\"" << rd->id << "\"";
  }
  aout << ", NULL};\n";

  aout << "  if (!PyArg_ParseTupleAndKeywords (args, kwds, \"|";
  for (const rpc_decl *rd = rs->decls.base (); rd < rs->decls.lim (); rd++) 
    aout << "O";
  aout << "\",\n"
       << "                                    kwlist,\n"
       << "                                    ";
  first = true;
  for (const rpc_decl *rd = rs->decls.base (); rd < rs->decls.lim (); rd++) {
    if (first)
      first = false;
    else
      aout << ", ";
    aout << "&" << rd->id;
  }
  aout << "))\n"
       << "    return -1;\n\n";

  for (const rpc_decl *rd = rs->decls.base (); rd < rs->decls.lim (); rd++) {
    aout << "  if (" << rd->id << ") \n"
	 << "     self->" << rd->id << ".set_obj (" << rd->id << ");\n";
  }
  
  
  aout << "  return 0;\n"
       << "}\n\n";
}

static void
dump_class_dealloc_func (const rpc_struct *rs)
{
  str ct = pyc_type (rs->id);
  aout << "\nstatic void\n"
       << ct << "_dealloc (" << ct << " *self)\n"
       << "{\n";
  for (const rpc_decl *rd = rs->decls.base (); rd < rs->decls.lim (); rd++) {
    aout << "  self->" << rd->id << ".clear ();\n";
  }
  aout << "  self->ob_type->tp_free ((PyObject *) self);\n"
       << "}\n\n";
}

static void
dump_allocator (const rpc_struct *rs)
{
  str pct = pyc_type (rs->id);
  str ppt = py_type (rs->id);
  aout << "void *\n"
       << pyc_type (rs->id) << "_alloc ()\n"
       << "{\n"
       << "  return (void *)" << pct << "_new (&" << ppt << ", NULL, NULL);\n"
       << "}\n\n";
}

static void
dump_convert (const rpc_struct *rs)
{
  str ptt = py_type (rs->id);
  str pct = pyc_type (rs->id);
  str wt = pyw_type (rs->id);
  
  aout << "template<> struct converter_t<" << wt << ">\n"
       << "{\n"
       << "  static PyObject *convert (PyObject *obj)\n"
       << "  {\n"
       << "    if (!PyObject_IsInstance (obj, (PyObject *)&" 
       <<                                           ptt <<  ")) {\n"
       << "       PyErr_SetString (PyExc_TypeError, \"expected object of type "
       <<                                           rs->id << "\");\n"
       << "       return NULL;\n"
       << "    }\n"
       << "    Py_INCREF (obj);\n"
       << "    return obj;\n"
       << "  }\n"
       << "};\n\n";
}

static void
dump_xdr_func (const rpc_struct *rs)
{
  str ct = pyc_type (rs->id);
  str wt = pyw_type (rs->id);
  aout << "bool\n"
       << "xdr_" << wt << " (XDR *xdrs, void *objp)\n"
       << "{\n"
       << "  return rpc_traverse (xdrs, *static_cast <" << wt << " *> "
       << "(objp));\n"
       << "}\n\n";
}

static str
rpc_trav_func (const str &a1, const str &obj, const rpc_decl *d)
{
  return str ();
}

static void
dump_w_rpc_traverse (const rpc_struct *rs)
{
  str ct = pyc_type (rs->id);
  str wt = pyw_type (rs->id);

  aout << "template<class T> bool\n"
       << "rpc_traverse (T &t, " << wt << " &wo)\n"
       << "{\n"
       << "  " << ct << " *io = static_cast<" << ct << " *> (wo.get_obj ());\n"
       << "  if (!io) {\n"
       << "    PyErr_SetString (PyExc_UnboundLocalError,\n"
       << "                     \"uninitialized XDR field\");\n"
       << "    return false;\n"
       << "  }\n"
       << "  return rpc_traverse (t, *io);\n"
       << "}\n\n";
}

static void
dump_rpc_traverse (const rpc_struct *rs)
{

  aout << "template<class T> "
       << (rs->decls.size () > 1 ? "" : "inline ") << "bool\n"
       << "rpc_traverse (T &t, " << pyc_type (rs->id) << " &obj)\n"
       << "{\n";
  str fnc;
  const rpc_decl *rd = rs->decls.base ();
  if (rd < rs->decls.lim ()) {
    aout << "  return rpc_traverse (t, obj." << rd->id << ")";
    rd++;
    while (rd < rs->decls.lim ()) { 
      aout << "\n         && rpc_traverse (t, obj." << rd->id << ")";
      rd++;
    }
    aout << ";\n";
  }
  else
    aout << "  return true;\n";
  aout << "}\n\n";
}

static void
dump_class_member_frag (str prefix, str typ, const rpc_decl *d)
{
  aout << prefix << "{ \"" << d->id << "\", T_OBJECT_EX, "
       << "offsetof (" << typ << ", " << d->id << "), 0, "
       << "\"" << d->id << " field of object of type " << typ << "\"},\n"
    ;
}

static void
dump_class_members (const rpc_struct *rs)
{
  aout << "static PyMemberDef " << pyc_type (rs->id) << "_members[] = {\n";

  // use get/setters for now
#if 0 
  for (const rpc_decl *rd = rs->decls.base (); rd < rs->decls.lim (); rd++) {
    dump_class_member_frag ("  ", pyc_type (rs->id), rd);
  }
#endif

  aout << "  {NULL}\n"
       << "};\n\n" ;
}

static void
dump_class_method_decls (const rpc_struct *rs)
{
  str ct = pyc_type (rs->id);
  aout << "PyObject * " << ct << "_pydump (" << ct << " *self);\n"
       << "PyObject * " << ct << "_str2xdr (" << ct 
       << " *self, PyObject *args);\n"
       << "PyObject * " << ct << "_xdr2str (" << ct << " *self);\n\n";
}

static void
dump_class_methods (const rpc_struct *rs)
{
  str ct = pyc_type (rs->id);
  aout << "PyObject *\n" 
       << ct << "_pydump (" << ct << " *self)\n"
       << "{\n"
       << "  dump_" << ct << " (self);\n"
       << "  Py_INCREF (Py_None);\n"
       << "  return Py_None;\n"
       << "}\n\n"
       << "PyObject *\n"
       << ct << "_xdr2str (" << ct << " *self)\n"
       << "{\n"
       << "  str ret = xdr2str (*self);\n"
       << "  if (ret) {\n"
       << "    return PyString_FromStringAndSize (ret.cstr (), ret.len ());\n"
       << "  } else {\n"
       << "    Py_INCREF (Py_None);\n"
       << "    return Py_None;\n"
       << "  }\n"
       << "}\n\n"
       << "PyObject *\n"
       << ct << "_str2xdr (" << ct << " *self, PyObject *args)\n"
       << "{\n"
       << "  PyObject *input;\n" 
       << "  if (!PyArg_Parse (args, \"(O)\", &input))\n"
       << "    return NULL;\n"
       << "\n"
       << "  if (!PyString_Check (input)) {\n"
       << "    PyErr_SetString (PyExc_TypeError,\n"
       << "                     \"Expected a string object\");\n"
       << "    return NULL;\n"
       << "  }\n"
       << "\n"
       << "  char *dat;\n"
       << "  int len;\n"
       << "  if (PyString_AsStringAndSize (input, &dat, &len) < 0) {\n"
       << "    PyErr_SetString (PyExc_Exception,\n"
       << "                     \"Garbled / corrupted string passed\");\n"
       << "    return NULL;\n"
       << "  }\n"
       << "  if (len == 0 || !dat) {\n"
       << "    goto succ;\n"
       << "  }\n"
       << "  if (!str2xdr (*self, str (dat, len))) {\n"
       << "    PyErr_SetString (AsyncXDR_Exception,\n"
       << "                     \"Failed to demarshal XDR data\");\n"
       << "    return NULL;\n"
       << "  }\n"
       << " succ:\n"
       << "  Py_INCREF (Py_None);\n"
       << "  return Py_None;\n"
       << "}\n\n";
}

static void
dump_class_methods_struct (const rpc_struct *rs)
{
  str ct = pyc_type (rs->id);
  aout << "static PyMethodDef " << ct << "_methods[] = {\n"
       << "  {\"dump\", (PyCFunction)" << ct << "_pydump, "
       <<        " METH_NOARGS,\n"
       << "   \"RPC-pretty print this method to stderr via async::warn\"},\n"
       << "  {\"xdr2str\", (PyCFunction)" << ct << "_xdr2str, "
       <<        " METH_NOARGS,\n"
       << "   \"Export RPC structure to a regular string buffer\"},\n"
       << "  {\"str2xdr\", (PyCFunction)" << ct << "_str2xdr, "
       <<        " METH_VARARGS,\n"
       << "   \"Import RPC structure from a regular string buffer\"},\n"
       << " {NULL}\n"
       << "};\n\n";
}

static void
dump_object_table (const rpc_struct *rs)
{
  str t = py_type (rs->id);
  str ct = pyc_type (rs->id);
  str pt = rs->id;

  aout << "PY_CLASS_DEF(" << ct << ", \"" << module << "." << pt 
       <<               "\", 1, dealloc, "
       <<               "-1, \"" << pt << " object\",\n"
       << "             methods, members, getsetters, init, new, 0);\n"
       << "\n";
}

static str
py_rpc_procno_method (const str &ct, const str &id)
{
  strbuf b;
  b << ct << "_RPC_PROCNO_" << id;
  return b;
}

static str
py_rpcprog_type (const rpc_program *prog, const rpc_vers *rv)
{
  return pyc_type (rpcprog (prog, rv));
}

static str
py_rpcprog_extension (const rpc_program *prog, const rpc_vers *rv)
{
  strbuf sb;
  sb << "py_" << rpcprog (prog, rv) << "_tbl";
  return sb;
}

static void
dump_prog_py_obj (const rpc_program *prog)
{
  for (const rpc_vers *rv = prog->vers.base (); rv < prog->vers.lim (); rv++) {
    str type = py_rpcprog_type (prog, rv);
    str objType = rpcprog (prog, rv);
    aout << "struct " << type << " : public py_rpc_program_t {\n"
	 << "};\n\n"
	 << "PY_CLASS_NEW(" << type << ")\n\n"
	 << "static int\n"
	 << type << "_init (" << type << " *self, PyObject *args, "
	 << "PyObject *kwds)\n"
	 << "{\n"
	 << "  if (!PyArg_ParseTuple (args, \"\"))\n"
	 << "    return -1;\n"
	 << "  self->prog = &" << rpcprog (prog, rv) << ";\n" 
	 << "  self->pytab = " << py_rpcprog_extension (prog, rv) << ";\n"
	 << "  return 0;\n"
	 << "}\n\n";

    for (const rpc_proc *rp = rv->procs.base (); rp < rv->procs.lim (); rp++) {
      aout << "static PyObject *\n"
	   << py_rpc_procno_method (type, rp->id) 
	   <<  " (" <<  type << " *self)"
	   << "{\n"
	   << "  return Py_BuildValue (\"i\", " << rp->id << ");\n"
	   << "}\n\n";
    }

    aout << "static PyMethodDef " << type << "_methods[] = {\n";
    for (const rpc_proc *rp = rv->procs.base (); rp < rv->procs.lim (); rp++) {
      aout << "  {\"" << rp->id <<  "\", (PyCFunction)" 
	   << py_rpc_procno_method (type, rp->id)
	   << ", METH_NOARGS,\n"
	   << "   \"RPC Procno for " << rp->id << "\"},\n";
    }
    aout << "  {NULL}\n"
	 << "};\n\n";


	 
    aout << "PY_CLASS_DEF(" << type << ", \"" << module << "." 
	 << objType << "\", "
	 <<               "1, 0, -1,\n"
	 << "             \"" << objType << " RPC program wrapper\", "
	 <<               "methods, 0, 0, init, new, 0);\n\n";
  }
  
}

static void
dump_getter_decl (const str &cl, const rpc_decl *d)
{
  aout << "PyObject * " << cl << "_get" << d->id 
       << " (" << cl << " *self, void *closure);\n";
}

static void
dump_getter (const str &cl, const rpc_decl *d)
{
  strbuf b;
  b << "self->" << obj_in_class (d);
  str obj = b;
  aout << "PyObject * " << cl << "_get" << d->id 
       << " (" << cl << " *self, void *closure)\n"
       << "{\n"
       << "  PyObject *obj = " << obj << ";\n"
       << "  if (!obj) {\n"
       << "    PyErr_SetString (PyExc_UnboundLocalError,\n"
       << "                     \"undefined class variable\");\n"
       << "    return NULL;\n"
       << "  }\n"
       << "  Py_XINCREF (obj);\n"
       << "  return obj;\n"
       << "}\n\n";
}

static str 
py_converter (const str &s)
{
  strbuf b;
  // note the extra space for nested templates
  b << "converter_t<" << s << " >::convert";
  return b;
}

static str
py_converter (const rpc_decl *d)
{
  return py_converter (decltype (d));
}

#if 0
static str
py_typecheck (const rpc_decl *d, const str &v)
{
  str s;
  if (d->type == "string" || d->type == "opaque") {
    s = py_typecheck_unqual (d->type, v);
  } else {
    switch (d->qual) {
    case rpc_decl::PTR:
    case rpc_decl::SCALAR: 
      s =  py_typecheck_unqual (d->type, v);
      break;
    case rpc_decl::VEC:
    case rpc_decl::ARRAY: 
      { 
	strbuf b;
	b << "PyList_Check (" << v << ")";
	s = b;
      }
      break;
    default:
      break;
    }
  }
  return s;
}
#endif

static void
dump_setter (const str &cl, const rpc_decl *d)
{
  aout << "int " << cl << "_set" << d->id 
       << " (" << cl << " *self, PyObject *value, void *closure)\n"
       << "{\n"
       << "  if (value == NULL) {\n"
       << "    PyErr_SetString(PyExc_RuntimeError, "
       << "\"Unexpected NULL first attribute\");\n"
       << "    return -1;\n"
       << "  }\n"
       << "  PyObject *o = " << py_converter (d) << " (value);\n"
       << "  if (!o) return -1;\n"
       << "  self->" << d->id << ".set_obj (o);\n"
       << "\n"
       << "  return 0;\n"
       << "};\n\n";
}



static void
dump_setter_decl (const str &cl, const rpc_decl *d)
{
  aout << "int " << cl << "_set" << d->id 
       << " (" << cl << " *self, PyObject *value, void *closure);\n";
}

static void
dump_getsetter_decl (const str &cl, const rpc_decl *d)
{
  dump_getter_decl (cl, d);
  dump_setter_decl (cl, d);
}

static void
dump_getsetter (const str &cl, const rpc_decl *d)
{
  dump_getter (cl, d);
  dump_setter (cl, d);
}

static void
dump_getsetters_decls (const rpc_struct *rs)
{
  for (const rpc_decl *rd = rs->decls.base (); rd < rs->decls.lim (); rd++)
    dump_getsetter_decl (pyc_type (rs->id), rd);
  aout << "\n\n";
}

static void
dump_getsetters (const rpc_struct *rs)
{
  for (const rpc_decl *rd = rs->decls.base (); rd < rs->decls.lim (); rd++)
    dump_getsetter (pyc_type (rs->id), rd);
}

static void
dump_getsetter_table_row (const str &prfx, const str &typ, const rpc_decl *d)
{
  aout << prfx << "{\"" << d->id << "\", "
       <<            "(getter)" << typ << "_get" << d->id << ", "
       <<            "(setter)" << typ << "_set" << d->id << ", "
       <<            "\"class variable: " << d->id << "\", "
       <<            "NULL },\n" ;
}

static void
dump_getsetter_table (const rpc_struct *rs)
{
  str ct = pyc_type (rs->id);
  aout << "static PyGetSetDef " << ct << "_getsetters[] = {\n";
  for (const rpc_decl *rd = rs->decls.base (); rd < rs->decls.lim (); rd++)
    dump_getsetter_table_row ("  ", ct, rd);
  aout << "  {NULL}\n"
       << "};\n\n";
}

static void
print_print (str type)
{
  str pref (strbuf ("%*s", int (8 + type.len ()), ""));
  aout << "void\n"
    "print_" << type << " (const void *_objp, const strbuf *_sbp, "
    "int _recdepth,\n" <<
    pref << "const char *_name, const char *_prefix)\n"
    "{\n"
    "  rpc_print (_sbp ? *_sbp : warnx, *static_cast<const " << type
       << " *> (_objp),\n"
    "             _recdepth, _name, _prefix);\n"
    "}\n";

  aout << "void\n"
    "dump_" << type << " (const " << type << " *objp)\n"
    "{\n"
    "  rpc_print (warnx, *objp);\n"
    "}\n\n";
}

static void
print_struct (const rpc_struct *s)
{
  str ct = pyc_type (s->id);
  aout <<
    "const strbuf &\n"
    "rpc_print (const strbuf &sb, const " << ct << " &obj, "
    "int recdepth,\n"
    "           const char *name, const char *prefix)\n"
    "{\n"
    "  if (name) {\n"
    "    if (prefix)\n"
    "      sb << prefix;\n"
    "    sb << \"" << s->id << " \" << name << \" = \";\n"
    "  };\n"
    "  const char *sep;\n"
    "  str npref;\n"
    "  if (prefix) {\n"
    "    npref = strbuf (\"%s  \", prefix);\n"
    "    sep = \"\";\n"
    "    sb << \"{\\n\";\n"
    "  }\n"
    "  else {\n"
    "    sep = \", \";\n"
    "    sb << \"{ \";\n"
    "  }\n";
  const rpc_decl *dp = s->decls.base (), *ep = s->decls.lim ();
  if (dp < ep)
    aout <<
      "  rpc_print (sb, obj." << dp->id << ", recdepth, "
      "\"" << dp->id << "\", npref);\n";
  while (++dp < ep)
    aout <<
      "  sb << sep;\n"
      "  rpc_print (sb, obj." << dp->id << ", recdepth, "
      "\"" << dp->id << "\", npref);\n";
  aout <<
    "  if (prefix)\n"
    "    sb << prefix << \"};\\n\";\n"
    "  else\n"
    "    sb << \" }\";\n"
    "  return sb;\n"
    "}\n";
  print_print (ct);
}

static void
print_union (const rpc_union *u)
{
}

static void
print_enum (const rpc_enum *e)
{
}

static void
dumpprint (const rpc_sym *s)
{
  switch (s->type) {
  case rpc_sym::STRUCT:
    print_struct (s->sstruct.addr ());
    break;
  case rpc_sym::UNION:
    print_union (s->sunion.addr ());
    break;
  case rpc_sym::ENUM:
    print_enum (s->senum.addr ());
    break;
  case rpc_sym::TYPEDEF:
    print_print (pyc_type (s->stypedef->id));
  default:
    break;
  }
}

static void
dumpstruct_mthds (const rpc_sym *s)
{
  const rpc_struct *rs = s->sstruct.addr ();
  dump_getsetters (rs);
  dump_allocator (rs);
  dump_class_methods (rs);
}

static void
dump_class_py (const rpc_struct *rs)
{
  str ct = pyc_type (rs->id);
  aout << "struct " << ct << " {\n"
       << "  PyObject_HEAD\n";
  for (const rpc_decl *rd = rs->decls.base (); rd < rs->decls.lim (); rd++)
    pdecl ("  ", rd);
  aout << "};\n\n";
  aout << "RPC_STRUCT_DECL (" << ct << ")\n\n";
}

static void
dump_w_class (const rpc_struct *rs)
{
  str ct = pyc_type (rs->id);
  str wt = pyw_type (rs->id);
  str pt = py_type (rs->id);
  aout << "struct " << wt << " : public pyw_base_t<" << wt << ">\n"
       << "{\n"
       << "  " << wt << " () : pyw_base_t<" << wt << "> (&" << pt << ")\n"
       << "  { alloc (); }\n\n"
       << "  bool init ();\n"
       << "  bool clear ();\n"
       << "};\n\n";
}

static void
dumpstruct (const rpc_sym *s)
{
  const rpc_struct *rs = s->sstruct.addr ();
  str ct = pyc_type (rs->id);

  dump_class_py (rs);
  dump_class_dealloc_func (rs);
  dump_class_new_func (rs);
  dump_class_members (rs);
  dump_class_method_decls (rs);
  dump_class_methods_struct (rs);
  dump_getsetters_decls (rs);
  dump_getsetter_table (rs);
  dump_class_init_func (rs);
  dump_object_table (rs);

  dump_w_class (rs);
  dump_convert (rs);
  dump_hacked_trav (rs, "init");
  dump_hacked_trav (rs, "clear");

  dump_rpc_traverse (rs);
  dump_w_rpc_traverse (rs);
  dump_xdr_func (rs);
}

void
static py_pswitch (str prefix, const rpc_union *rs, str swarg,
	 void (*pt) (str, const rpc_union *rs, const rpc_utag *),
	 str suffix, void (*defac) (str, const rpc_union *rs))
{
  bool hasdefault = false;
  str subprefix = strbuf () << prefix << "  ";

  aout << prefix << "switch (" << swarg << ") {" << suffix;
  for (const rpc_utag *rt = rs->cases.base (); rt < rs->cases.lim (); rt++) {
    if (rt->swval) {
      if (rt->swval == "TRUE")
	aout << prefix << "case true:" << suffix;
      else if (rt->swval == "FALSE")
	aout << prefix << "case false:" << suffix;
      else
	aout << prefix << "case " << rt->swval << ":" << suffix;
    }
    else {
      hasdefault = true;
      aout << prefix << "default:" << suffix;
    }
    if (rt->tagvalid)
      pt (subprefix, rs, rt);
  }
  if (!hasdefault && defac) {
    aout << prefix << "default:" << suffix;
    defac (subprefix, rs);
  }
  aout << prefix << "}\n";
}

#if 0
static void
puniontraverse (str prefix, const rpc_union *rs, const rpc_utag *rt)
{
#if 0
  aout << prefix << "if (obj." << rs->tagid << " != tag)\n";
  if (rt->tag.type == "void")
    aout << prefix << "  obj._base.destroy ();\n";
  else
    aout << prefix << "  obj." << rt->tag.id << ".select ();\n";
#endif
  if (rt->tag.type == "void")
    aout << prefix << "return true;\n";
  else
    aout << prefix << "return rpc_traverse (t, *obj." << rt->tag.id << ");\n";
}

static void
pselect (str prefix, const rpc_union *rs, const rpc_utag *rt)
{
  if (rt->tag.type == "void")
    aout << prefix << "_base.destroy ();\n";
  else
    aout << prefix << rt->tag.id << ".select ();\n";
  aout << prefix << "break;\n";
}
#endif

static void
punionmacro (str prefix, const rpc_union *rs, const rpc_utag *rt)
{
  if (rt->tag.type == "void")
    aout << prefix << "voidaction; \\\n";
  else
    aout << prefix << "action (" << rt->tag.type << ", "
	 << rt->tag.id << "); \\\n";
  aout << prefix << "break; \\\n";
}

static void
punionmacrodefault (str prefix, const rpc_union *rs)
{
  aout << prefix << "defaction; \\\n";
  aout << prefix << "break; \\\n";
}

static void
dumpunion (const rpc_sym *s)
{
  bool hasdefault = false;

  const rpc_union *rs = s->sunion.addr ();
  aout << "\nstruct " << rs->id << " {\n"
       << "  const " << rs->tagtype << " " << rs->tagid << ";\n"
       << "  union {\n"
       << "    union_entry_base _base;\n";
  for (const rpc_utag *rt = rs->cases.base (); rt < rs->cases.lim (); rt++) {
    if (!rt->swval)
      hasdefault = true;
    if (rt->tagvalid && rt->tag.type != "void") {
      str type = decltype (&rt->tag);
      if (type[type.len ()-1] == '>')
	type = type << " ";
      aout << "    union_entry<" << type << "> "
	   << rt->tag.id << ";\n";
    }
  }
  aout << "  };\n\n";

  aout << "#define rpcunion_tag_" << rs->id << " " << rs->tagid << "\n";
  aout << "#define rpcunion_switch_" << rs->id
       << "(swarg, action, voidaction, defaction) \\\n";
  py_pswitch ("  ", rs, "swarg", punionmacro, " \\\n", punionmacrodefault);

  aout << "\n"
       << "  " << rs->id << " (" << rs->tagtype << " _tag = ("
       << rs->tagtype << ") 0) : " << rs->tagid << " (_tag)\n"
       << "    { _base.init (); set_" << rs->tagid << " (_tag); }\n"

       << "  " << rs->id << " (" << "const " << rs->id << " &_s)\n"
       << "    : " << rs->tagid << " (_s." << rs->tagid << ")\n"
       << "    { _base.init (_s._base); }\n"
       << "  ~" << rs->id << " () { _base.destroy (); }\n"
       << "  " << rs->id << " &operator= (const " << rs->id << " &_s) {\n"
       << "    const_cast<" << rs->tagtype << " &> ("
       << rs->tagid << ") = _s." << rs->tagid << ";\n"
       << "    _base.assign (_s._base);\n"
       << "    return *this;\n"
       << "  }\n\n";

  aout << "  void set_" << rs->tagid << " (" << rs->tagtype << " _tag) {\n"
       << "    const_cast<" << rs->tagtype << " &> (" << rs->tagid
       << ") = _tag;\n"
       << "    rpcunion_switch_" << rs->id << "\n"
       << "      (_tag, RPCUNION_SET, _base.destroy (), _base.destroy ());\n"
       << "  }\n";

#if 0
  aout << "  void Xstompcast () {\n"
       << "    rpcunion_switch_" << rs->id << "\n"
       << "      (" << rs->tagid << ", RPCUNION_STOMPCAST,\n"
       << "       _base.destroy (), _base.destroy ());\n"
       << "  }\n";
#endif
  aout << "};\n";

  aout << "\ntemplate<class T> bool\n"
       << "rpc_traverse (T &t, " << rs->id << " &obj)\n"
       << "{\n"
       << "  " << rs->tagtype << " tag = obj." << rs->tagid << ";\n"
       << "  if (!rpc_traverse (t, tag))\n"
       << "    return false;\n"
       << "  if (tag != obj." << rs->tagid << ")\n"
       << "    obj.set_" << rs->tagid << " (tag);\n\n"
       << "  rpcunion_switch_" << rs->id << "\n"
       << "    (obj." << rs->tagid << ", RPCUNION_TRAVERSE, "
       << "return true, return false);\n"
       << "}\n"
       << "inline bool\n"
       << "rpc_traverse (const stompcast_t &s, " << rs->id << " &obj)\n"
       << "{\n"
       << "  rpcunion_switch_" << rs->id << "\n"
       << "    (obj." << rs->tagid << ", RPCUNION_REC_STOMPCAST,\n"
       << "     obj._base.destroy (); return true, "
       << "obj._base.destroy (); return true;);\n"
       << "}\n";
  pmshl (rs->id);
  // aout << "RPC_TYPE_DECL (" << rs->id << ")\n";
  aout << "RPC_UNION_DECL (" << rs->id << ")\n";

  aout << "\n";
}

static void
dumpenum (const rpc_sym *s)
{
  int ctr = 0;
  str lastval;
  const rpc_enum *rs = s->senum.addr ();

  aout << "enum " << rs->id << " {\n";
  for (const rpc_const *rc = rs->tags.base (); rc < rs->tags.lim (); rc++) {
    if (rc->val) {
      lastval = rc->val;
      ctr = 1;
      aout << "  " << rc->id << " = " << rc->val << ",\n";
    }
    else if (lastval && (isdigit (lastval[0]) || lastval[0] == '-'
			 || lastval[0] == '+'))
      aout << "  " << rc->id << " = "
	   << strtol (lastval, NULL, 0) + ctr++ << ",\n";
    else if (lastval)
      aout << "  " << rc->id << " = " << lastval << " + " << ctr++ << ",\n";
    else
      aout << "  " << rc->id << " = " << ctr++ << ",\n";
  }
  aout << "};\n";
  pmshl (rs->id);
  aout << "RPC_ENUM_DECL (" << rs->id << ")\n";

  aout << "\ntemplate<class T> inline bool\n"
       << "rpc_traverse (T &t, " << rs->id << " &obj)\n"
       << "{\n"
       << "  u_int32_t val = obj;\n"
       << "  if (!rpc_traverse (t, val))\n"
       << "    return false;\n"
       << "  obj = " << rs->id << " (val);\n"
       << "  return true;\n"
       << "}\n";
}

static void
dumptypedef (const rpc_sym *s)
{
  const rpc_decl *rd = s->stypedef.addr ();
  pdecl ("typedef ", rd);
  pmshl (rd->id);
  aout << "RPC_TYPEDEF_DECL (" << rd->id << ")\n";
}
static void
mktbl (const rpc_program *rs)
{
  for (const rpc_vers *rv = rs->vers.base (); rv < rs->vers.lim (); rv++) {
    str name = rpcprog (rs, rv);
    aout << "static const rpcgen_table " << name << "_tbl[] = {\n"
	 << "  " << rs->id << "_" << rv->val << "_APPLY (XDRTBL_DECL)\n"
	 << "};\n"
	 << "const rpc_program " << name << " = {\n"
	 << "  " << rs->id << ", " << rv->id << ", " << name << "_tbl,\n"
	 << "  sizeof (" << name << "_tbl" << ") / sizeof ("
	 << name << "_tbl[0]),\n"
	 << "  \"" << name << "\"\n"
	 << "};\n\n";
  }
  aout << "\n";
}

static void
dump_procno_ins (const rpc_program *rs)
{
  aout << "static int\n"
       << "py_module_all_ins (PyObject *mod)\n"
       << "{\n"
       << "  int rc = 0;\n";
    for (const rpc_vers *rv = rs->vers.base (); rv < rs->vers.lim (); rv++) {
    for (const rpc_proc *rp = rv->procs.base (); rp < rv->procs.lim (); rp++) {
      aout << "  if ((rc = PyModule_AddIntConstant (mod, \""
	   << rp->id << "\", (long )" << rp->id << ")) < 0)\n"
	   << "    return rc;\n";
    }
  }
  aout << "  return rc;\n"
       << "}\n\n";

}

static void
dumpprog (const rpc_sym *s)
{
  const rpc_program *rs = s->sprogram.addr ();
  // aout << "\nenum { " << rs->id << " = " << rs->val << " };\n";
  aout << "#ifndef " << rs->id << "\n"
       << "#define " << rs->id << " " << rs->val << "\n"
       << "#endif /* !" << rs->id << " */\n";
  for (const rpc_vers *rv = rs->vers.base (); rv < rs->vers.lim (); rv++) {
    //aout << "extern const rpc_program " << rpcprog (rs, rv) << ";\n";
    aout << "enum { " << rv->id << " = " << rv->val << " };\n";
    aout << "enum {\n";
    for (const rpc_proc *rp = rv->procs.base (); rp < rv->procs.lim (); rp++)
      aout << "  " << rp->id << " = " << rp->val << ",\n";
    aout << "};\n";
    aout << "#define " << rs->id << "_" << rv->val
	 << "_APPLY_NOVOID(macro," << pyw_type ("void") << ")";
    u_int n = 0;
    for (const rpc_proc *rp = rv->procs.base (); rp < rv->procs.lim (); rp++) {
      while (n++ < rp->val)
	aout << " \\\n  macro (" << n-1 << ", false, false)";
      aout << " \\\n  macro (" << rp->id << ", " << pyw_type (rp->arg)
	   << ", " << pyw_type (rp->res) << ")";
    }
    aout << "\n";
    aout << "#define " << rs->id << "_" << rv->val << "_APPLY(macro) \\\n  "
	 << rs->id << "_" << rv->val << "_APPLY_NOVOID(macro, "
	 << pyw_type ("void") << ")\n";
  }

  // dump typechecker methods
  aout << "\n";
  for (const rpc_vers *rv = rs->vers.base (); rv < rs->vers.lim (); rv++) {
    u_int n = 0;
    str name = rpcprog (rs, rv);
    aout << "static const py_rpcgen_table_t " 
	 << py_rpcprog_extension (rs, rv) << "[] = {\n";
    for (const rpc_proc *rp = rv->procs.base (); rp < rv->procs.lim (); rp++) {
      while (n++ < rp->val) {
	aout << " py_rpcgen_error,\n";
	//aout << "  { convert_error, convert_error, wrap_error, "
	//    << "dealloc_error },\n";
      }
      aout << "  { " << py_converter (pyw_type (rp->arg)) << ",\n"
	   << "    " << py_converter (pyw_type (rp->res)) << ",\n"
	   << "    unwrap<" << pyw_type (rp->res) << "> }\n";
    }
    aout << "};\n\n";
  }

  mktbl (rs);
  dump_procno_ins (rs);
  dump_prog_py_obj (rs);
}

static void
dumpsym (const rpc_sym *s, pass_num_t pass)
{
  switch (s->type) {
  case rpc_sym::CONST:
    aout << "enum { " << s->sconst->id
	 << " = " << s->sconst->val << " };\n";
    break;
  case rpc_sym::STRUCT:
    if (pass == PASS_ONE)
      dumpstruct (s);
    else if (pass == PASS_THREE) 
      dumpstruct_mthds (s);
    break;
  case rpc_sym::UNION:
    if (pass == PASS_ONE)
      dumpunion (s);
    break;
  case rpc_sym::ENUM:
    if (pass == PASS_ONE)
      dumpenum (s);
    break;
  case rpc_sym::TYPEDEF:
    if (pass == PASS_ONE)
      dumptypedef (s);
    break;
  case rpc_sym::PROGRAM:
    if (pass == PASS_THREE)
      dumpprog (s);
    break;
  case rpc_sym::LITERAL:
    if (pass == PASS_ONE)
      aout << *s->sliteral << "\n";
    break;
  default:
    break;
  }
}


static str
makemodulename (str fname)
{
  strbuf guard;
  const char *p;

  if ((p = strrchr (fname, '/')))
    p++;
  else p = fname;

  while (char c = *p++) {
    if (isalnum (c))
      guard << c;
    else
      break;
  }

  return guard;
}

static vec<str>
get_c_classes (const rpc_sym *s)
{
  vec<str> ret;
  switch (s->type) {
  case rpc_sym::STRUCT:
    ret.push_back (s->sstruct.addr ()->id);
    break;
  case rpc_sym::UNION:
    ret.push_back (s->sunion.addr ()->id);
    break;
  case rpc_sym::PROGRAM:
    {
      const rpc_program *p = s->sprogram.addr ();
      for (const rpc_vers *rv = p->vers.base (); rv < p->vers.lim (); rv++) {
	ret.push_back (rpcprog (p, rv));
      }
    }
    break;
  default:
    break;
  }
  return ret;
}

static void
dumpmodule (const symlist_t &lst)
{
  aout << "static PyMethodDef module_methods[] = {\n"
       << "  {NULL}\n"
       << "};\n\n"
       << "#ifndef PyMODINIT_FUNC	"
       << "/* declarations for DLL import/export */\n"
       << "#define PyMODINIT_FUNC void\n"
       << "#endif\n"
       << "PyMODINIT_FUNC\n"
       << "init" << module << " (void)\n"
       << "{\n"
       << "  PyObject* m;\n"
       << "  PyObject *module;\n"
       << "\n"
       << "  if (!import_async_exceptions (&AsyncXDR_Exception))\n"
       << "    return;\n"
       << "\n"
       << "  // import async.arpc and get the type information for\n"
       << "  // async.arpc.rpc_program\n"
       << "  module = PyImport_ImportModule (\"async.arpc\");\n"
       << "  if (!module)\n"
       << "    return;\n"
       << "  PyObject *tmp = PyObject_GetAttrString (module,\n"
       << "         \"rpc_program\");\n"
       << "  if (!tmp) {\n"
       << "    PyErr_SetString (PyExc_TypeError,\n"
       << "             \"Cannot load rpc_program type from async.arpc\");\n"
       << "    Py_DECREF (module);\n"
       << "    return;\n"
       << "  }\n"
       << "  if (!PyType_Check (tmp)) {\n"
       << "     Py_DECREF (tmp);\n"
       << "     Py_DECREF (module);\n"
       << "     PyErr_SetString(PyExc_TypeError,\n"
       << "             \"Expected async.arpc.rpc_program to be a type\");\n"
       << "     return;\n"
       << "  }\n"
       << "  py_rpc_program = (PyTypeObject *)tmp;\n"
       << "  Py_DECREF (module);\n"
       << "\n";
    
  // now add the subclasses to therpc_program types
  aout << "  // after import, we can fix up the rpc_program types..\n";
  for (const rpc_sym *s = lst.base (); s < lst.lim () ; s++) {
    if (s->type != rpc_sym::PROGRAM) 
      continue;
    vec<str> clss = get_c_classes (s);
    str cls;
    while (clss.size ()) {
      cls = clss.pop_back ();
      aout <<  "  " << py_type (cls) << ".tp_base = py_rpc_program;\n";
    }
  }

  aout << "\n"
       << "  if (";
  bool first = true;
  str cls;
  for (const rpc_sym *s = lst.base (); s < lst.lim () ; s++) {
    vec<str> clss = get_c_classes (s);
    str cls;
    while (clss.size ()) {
      cls = clss.pop_back ();
      if (first)
	first = false;
      else {
	aout << " ||\n     ";
      }
      aout << "PyType_Ready (&" << py_type (cls) << ") < 0";
    }
  }
  aout << ")\n"
       << "    return;\n"
       << "\n"
       << "  m = Py_InitModule3 (\"" << module << "\", module_methods,\n"
       << "                      \"Python/rpc/XDR module for " 
       << module << ".\");\n"
       << "\n"
       << "  if (m == NULL)\n"
       << "    return;\n"
       << "  if (py_module_all_ins (m) < 0)\n"
       << "    return;\n"
       << "\n";
  
  
  for (const rpc_sym *s = lst.base (); s < lst.lim () ; s++) {
    vec<str> clss = get_c_classes (s);
    str cls;
    while (clss.size ()) {
      cls = clss.pop_back ();
      aout << "  Py_INCREF (&" << py_type (cls) << ");\n"
	   << "  PyModule_AddObject (m, \"" << cls
	   << "\", (PyObject *)&" << py_type (cls) << ");\n";
    }
  }
  
  aout << "}\n"
       << "\n";
}

void
genpyc (str fname)
{
  module = makemodulename (fname);
  

  aout << "// -*-c++-*-\n"
       << "/* This file was automatically generated by rpcc. */\n\n"
       << "#include \"xdrmisc.h\"\n"
       << "#include \"crypt.h\"\n"
       << "#include \"py_rpctypes.h\"\n"
       << "#include \"py_gen.h\"\n"
       << "\n"
       << "\n"
       << "static PyObject *AsyncXDR_Exception;\n"
       << "static PyTypeObject *py_rpc_program;\n"
       << "\n";

  int last = rpc_sym::LITERAL;
  
  // 3 - pass system to get dependencies / orders right
  for (pass_num_t i = PASS_ONE; i < N_PASSES ; 
       i = (pass_num_t) ((int )i + 1)) {
    for (const rpc_sym *s = symlist.base (); s < symlist.lim (); s++) {
      if (last != s->type
	  || last == rpc_sym::PROGRAM
	  || last == rpc_sym::TYPEDEF
	  || last == rpc_sym::STRUCT
	  || last == rpc_sym::UNION
	  || last == rpc_sym::ENUM)
	aout << "\n";
      last = s->type;
      switch (i) {
      case PASS_ONE:
      case PASS_THREE:
	dumpsym (s, i);
	break;
      case PASS_TWO:
	dumpprint (s);
	break;
      default:
	break;
      }
    }
  }

  dumpmodule (symlist);
}
