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
  if (d->type == "string")
    return strbuf () << pyc_type ("rpc_str") << "<" << d->bound << ">";
  else if (d->type == "opaque")
    switch (d->qual) {
    case rpc_decl::ARRAY:
      return strbuf () << pyc_type ("rpc_opaque") << "<" << d->bound << ">";
      break;
    case rpc_decl::VEC:
      return strbuf () << pyc_type ("rpc_bytes") << "<" << d->bound << ">";
      break;
    default:
      panic ("bad rpc_decl qual for opaque (%d)\n", d->qual);
      break;
    }
  else
    switch (d->qual) {
    case rpc_decl::SCALAR:
      return pyc_type (d->type);
      break;
    case rpc_decl::PTR:
      return strbuf () << pyc_type ("rpc_ptr") << "<" << d->type << ">";
      break;
    case rpc_decl::ARRAY:
      return strbuf () << pyc_type ("array") << "<" 
		       << d->type << ", " << d->bound << ">";
      break;
    case rpc_decl::VEC:
      return strbuf () << pyc_type ("rpc_vec") << "<" << d->type 
		       << ", " << d->bound << ">";
      break;
    default:
      panic ("bad rpc_decl qual (%d)\n", d->qual);
    }
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


static str
py_member_init (const rpc_decl *d)
{
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
	  b << "PyInstance_New (" << py_type (d->type) << ", NULL, NULL)";
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

static void
dump_init_frag (str prfx, const rpc_decl *d)
{
  aout << prfx << "if (!(self->" << d->id << ".set_obj ("
       << py_member_init (d) << "))) {\n"
       << prfx << "  Py_DECREF (self);\n"
       << prfx << "  return NULL;\n"
       << prfx << "}\n"
       << "\n";
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
dump_class_dealloc_func (const rpc_struct *rs)
{
  str ct = pyc_type (rs->id);
  aout << "\nstatic void\n"
       << ct << "_dealloc (" << ct << " *self)\n"
       << "{\n";
  for (const rpc_decl *rd = rs->decls.base (); rd < rs->decls.lim (); rd++) {
    aout << "  self->" << rd->id << ".clear ();\n";
  }
  aout << "}\n\n";
}

static void
dump_allocator (const rpc_struct *rs)
{
  aout << "void *\n"
       << pyc_type (rs->id) << "_alloc ()\n"
       << "{\n"
       << "  return PyInstance_New (" << py_type (rs->id) << ", NULL, NULL);\n"
       << "}\n\n";
}

static void
dump_xdr_func (const rpc_struct *rs)
{
  aout << "bool_t\n"
       << "xdr_" << pyc_type (rs->id) << " (XDR *xdrs, void *objp)\n"
       << "{\n"
       << "  switch (xdrs->x_ops) {\n"
       << "  case XDR_ENCODE:\n"
       << "  case XDR_DECODE:\n"
       << "    return rpc_traverse (xdrs, *static_cast<" 
       << pyc_type (rs->id) << " *> (objp));\n"
       << "  case XDR_FREE:\n"
       << "    Py_XDECREF (static_cast<PyObject *> (objp));\n"
       << "    return true;\n"
       << "  default:\n"
       << "    panic (\"invalid xdr operation %d\\n\", xdrs->x_op);\n"
       << "  }\n"
       << "  return false;\n"
       << "}\n\n";
}

static str
rpc_trav_func (const str &a1, const str &obj, const rpc_decl *d)
{
  return str ();
}

static void
dump_rpc_traverse (const rpc_struct *rs)
{

  aout << "\ntemplate<class T> "
       << (rs->decls.size () > 1 ? "" : "inline ") << "bool\n"
       << "rpc_traverse (T &t, " << pyc_type (rs->id) << " &obj)\n"
       << "{\n";
  str fnc;
  const rpc_decl *rd = rs->decls.base ();
  if (rd < rs->decls.lim ()) {
    aout << "  return rpc_traverse (t, obj." << rd->id << ")";
    rd++;
    while (rd < rs->decls.lim ()) { 
      aout << "\n     && rpc_traverse (t, obj." << rd->id << ")";
      rd++;
    }
    aout << ";\n";
  }
  else
    aout << "  return true;\n";
  aout << "}\n\n";

  str typ = py_type (rs->id);

#if 0
  aout << "template<class T> bool\n"
       << "rpc_traverse_" << typ << " (T &id, PyObjWrap &w)\n"
       << "{\n"
       << "  if (! " << py_typecheck_unqual (rs->id, "w.obj") << ") {\n"
       << "    PyErr_SetString (PyExec_Type_Error, \n"
       << "                     \"Type mismatch in rpc_traverse for type=" 
       <<                       typ << "\");\n"
       << "    return false;\n"
       << "  }\n"
       << "  if (!w.scalar) {\n"
       << "    PyErr_SetString (PyExec_Type_Error,\n"
       << "                     \"Expected a scalar object; got a vector!\")"
       <<                 ";\n"
       << "    return false;\n"
       << "  }\n"
       << "  return rpc_traverse (id, *static_cast<" << rs->id 
       << " *> (obj));\n"
       << "}\n\n";
#endif
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
  for (const rpc_decl *rd = rs->decls.base (); rd < rs->decls.lim (); rd++) {
    dump_class_member_frag ("  ", pyc_type (rs->id), rd);
  }
  aout << "  {NULL}\n"
       << "};\n\n" ;
}

static void
dump_class_methods_struct (const rpc_struct *rs)
{
  aout << "static PyMethodDef " << pyc_type (rs->id) << "_methods[] = {\n"
       << " {NULL}\n"
       << "};\n\n";
}

static void
dump_object_table (const rpc_struct *rs)
{
  str t = py_type (rs->id);
  str ct = pyc_type (rs->id);
  str pt = rs->id;

  aout << "static PyTypeObject " << t << " = {\n"
       << "  PyObject_HEAD_INIT(NULL)\n"
       << "  0,                         /*ob_size*/\n"
       << "  \"" <<  module << "." <<  pt
       << "\",               /*tp_name*/\n"
       << "  sizeof(" << ct << "),             /*tp_basicsize*/\n"
       << "  0,                         /*tp_itemsize*/\n"
       << "  (destructor)" << ct << "_dealloc, /*tp_dealloc*/\n"
       << "  0,                         /*tp_print*/\n"
       << "  0,                         /*tp_getattr*/\n"
       << "  0,                         /*tp_setattr*/\n"
       << "  0,                         /*tp_compare*/\n"
       << "  0,                         /*tp_repr*/\n"
       << "  0,                         /*tp_as_number*/\n"
       << "  0,                         /*tp_as_sequence*/\n"
       << "  0,                         /*tp_as_mapping*/\n"
       << "  0,                         /*tp_hash */\n"
       << "  0,                         /*tp_call*/\n"
       << "  0,                         /*tp_str*/\n"
       << "  0,                         /*tp_getattro*/\n"
       << "  0,                         /*tp_setattro*/\n"
       << "  0,                         /*tp_as_buffer*/\n"
       << "  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/\n"
       << "  \"" << ct << " objects\",           /* tp_doc */\n"
       << "  0,		               /* tp_traverse */\n"
       << "  0,		               /* tp_clear */\n"
       << "  0,		               /* tp_richcompare */\n"
       << "  0,		               /* tp_weaklistoffset */\n"
       << "  0,		               /* tp_iter */\n"
       << "  0,		               /* tp_iternext */\n"
       << "  " << ct << "_methods,             /* tp_methods */\n"
       << "  " << ct << "_members,             /* tp_members */\n"
       << "  " << ct << "_getsetters,          /* tp_getset */\n"
       << "  0,                         /* tp_base */\n"
       << "  0,                         /* tp_dict */\n"
       << "  0,                         /* tp_descr_get */\n"
       << "  0,                         /* tp_descr_set */\n"
       << "  0,                         /* tp_dictoffset */\n"
       << "  (initproc)" << ct << "_init,      /* tp_init */\n"
       << "  0,                         /* tp_alloc */\n"
       << "  " << ct << "_new,                 /* tp_new */\n"
       << "};\n\n";
}

static void
dump_getter_decl (const str &cl, const rpc_decl *d)
{
  aout << "PyObject * " << cl << "_get" << d->id 
       << " (" << cl << " *self, void, *closure);\n";
}

static void
dump_getter (const str &cl, const rpc_decl *d)
{
  aout << "PyObject * " << cl << "_get" << d->id 
       << " (" << cl << " *self, void, *closure)\n"
       << "{\n"
       << "  Py_XINCREF (self->" << obj_in_class (d) << ");\n"
       << "  return self->" << obj_in_class (d) << ";\n"
       << "}\n\n";
}

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

static void
dump_setter (const str &cl, const rpc_decl *d)
{
  aout << "int " << cl << "_set" << d->id 
       << " (" << cl << " *self, PyObject *value, void *closure)\n"
       << "{\n"
       << "  if (value == NULL) {\n"
       << "    PyErr_SetString(PyExc_TypeError, "
       << "\"Cannot delete first attribute\");\n"
       << "    return -1;\n"
       << "  }\n"
       << "  if (! " << py_typecheck (d, "value") << ") {\n"
       << "    PyErr_SetString (PyExc_Type_Error, \n"
       << "                     \"Expected an object of type " 
       <<                        py_type (d) << "\");\n"
       << "    return -1;\n"
       << "  }\n"
       << "  Py_INCREF (value);\n"
       << "  self->" << d->id << ".set_obj (value);\n"
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
  aout << "static PyGetSetDef " << rs->id << "_getsetters[] = {\n";
  for (const rpc_decl *rd = rs->decls.base (); rd < rs->decls.lim (); rd++)
    dump_getsetter_table_row ("  ", rs->id, rd);
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
dumpstruct (const rpc_sym *s)
{
  const rpc_struct *rs = s->sstruct.addr ();
  str ct = pyc_type (rs->id);
  aout << "\nstruct " << ct << " {\n"
       << "  PyObject_HEAD\n";
  for (const rpc_decl *rd = rs->decls.base (); rd < rs->decls.lim (); rd++)
    pdecl ("  ", rd);
  aout << "};\n";
  //pmshl (rs->id);
  aout << "RPC_STRUCT_DECL (" << ct << ")\n";
  // aout << "RPC_TYPE_DECL (" << rs->id << ")\n";

  dump_class_dealloc_func (rs);
  dump_class_new_func (rs);
  dump_allocator (rs);
  dump_xdr_func (rs);
  dump_class_members (rs);
  dump_class_methods_struct (rs);
  dump_getsetters_decls (rs);
  dump_getsetter_table (rs);
  dump_object_table (rs);
  dump_getsetters (rs);

  dump_rpc_traverse (rs);
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
dumpprog (const rpc_sym *s)
{
  const rpc_program *rs = s->sprogram.addr ();
  // aout << "\nenum { " << rs->id << " = " << rs->val << " };\n";
  aout << "#ifndef " << rs->id << "\n"
       << "#define " << rs->id << " " << rs->val << "\n"
       << "#endif /* !" << rs->id << " */\n";
  for (const rpc_vers *rv = rs->vers.base (); rv < rs->vers.lim (); rv++) {
    aout << "extern const rpc_program " << rpcprog (rs, rv) << ";\n";
    aout << "enum { " << rv->id << " = " << rv->val << " };\n";
    aout << "enum {\n";
    for (const rpc_proc *rp = rv->procs.base (); rp < rv->procs.lim (); rp++)
      aout << "  " << rp->id << " = " << rp->val << ",\n";
    aout << "};\n";
    aout << "#define " << rs->id << "_" << rv->val
	 << "_APPLY_NOVOID(macro," << pyc_type ("void") << ")";
    u_int n = 0;
    for (const rpc_proc *rp = rv->procs.base (); rp < rv->procs.lim (); rp++) {
      while (n++ < rp->val)
	aout << " \\\n  macro (" << n-1 << ", false, false)";
      aout << " \\\n  macro (" << rp->id << ", " << pyc_type (rp->arg)
	   << ", " << pyc_type (rp->res) << ")";
    }
    aout << "\n";
    aout << "#define " << rs->id << "_" << rv->val << "_APPLY(macro) \\\n  "
	 << rs->id << "_" << rv->val << "_APPLY_NOVOID(macro, "
	 << pyc_type ("void") << ")\n";
  }
  aout << "\n";
}

static void
dumpsym (const rpc_sym *s)
{
  switch (s->type) {
  case rpc_sym::CONST:
    aout << "enum { " << s->sconst->id
	 << " = " << s->sconst->val << " };\n";
    break;
  case rpc_sym::STRUCT:
    dumpstruct (s);
    break;
  case rpc_sym::UNION:
    dumpunion (s);
    break;
  case rpc_sym::ENUM:
    dumpenum (s);
    break;
  case rpc_sym::TYPEDEF:
    dumptypedef (s);
    break;
  case rpc_sym::PROGRAM:
    dumpprog (s);
    break;
  case rpc_sym::LITERAL:
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

void
genpyc (str fname)
{
  module = makemodulename (fname);
  

  aout << "// -*-c++-*-\n"
       << "/* This file was automatically generated by rpcc. */\n\n"
       << "#include \"xdrmisc.h\"\n"
       << "#include \"py_rpctypes.h\"\n"
       << "\n";

  int last = rpc_sym::LITERAL;
  for (const rpc_sym *s = symlist.base (); s < symlist.lim (); s++) {
    if (last != s->type
	|| last == rpc_sym::PROGRAM
	|| last == rpc_sym::TYPEDEF
	|| last == rpc_sym::STRUCT
	|| last == rpc_sym::UNION
	|| last == rpc_sym::ENUM)
      aout << "\n";
    last = s->type;
    dumpsym (s);
    dumpprint (s);
  }
}
