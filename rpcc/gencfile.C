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
#include "rxx.h"

static void collect_rpctype (str i);

static void
mkmshl (str id)
{
#if 0
  if (!needtype[id])
    return;
#endif
  aout << "void *\n"
       << id << "_alloc ()\n"
       << "{\n"
       << "  return New " << id << ";\n"
       << "}\n"
#if 0
       << "void\n"
       << id << "_free (void *objp)\n"
       << "{\n"
       << "  delete static_cast<" << id << " *> (objp);\n"
       << "}\n"
#endif
       << XDR_RETURN "\n"
       << "xdr_" << id << " (XDR *xdrs, void *objp)\n"
       << "{\n"
       << "  bool_t ret = false;\n"
       << "  switch (xdrs->x_op) {\n"
       << "  case XDR_ENCODE:\n"
       << "  case XDR_DECODE:\n"
       << "    {\n"
       << "      ptr<v_XDR_t> v = xdr_virtualize (xdrs);\n"
       << "      if (v) {\n"
       << "        ret = rpc_traverse (v, *static_cast<"
       << id << " *> (objp));\n"
       << "      } else {\n"
       << "        ret = rpc_traverse (xdrs, *static_cast<"
       << id << " *> (objp));\n"
       << "      }\n"
       << "    }\n"
       << "    break;\n"
       << "  case XDR_FREE:\n"
       << "    rpc_destruct (static_cast<" << id << " *> (objp));\n"
       << "    ret = true;\n"
       << "    break;\n"
       << "  default:\n"
       << "    panic (\"invalid xdr operation %d\\n\", xdrs->x_op);\n"
       << "    break;\n"
       << "  }\n"
       << "  return ret;\n"
       << "}\n"
       << "\n";
  collect_rpctype (id);
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
	 << "};\n";
  }
  aout << "\n";
}

//-----------------------------------------------------------------------

struct rpc_constant_t {
  rpc_constant_t (str i, str t) : id (i), typ (t) {}
  str id, typ;
};

vec<rpc_constant_t> rpc_constants;
vec<str> rpc_types;

void collect_constant (str i, str t)
{
  rpc_constants.push_back (rpc_constant_t (i, t));
}

void collect_rpctype (str i)
{
  rpc_types.push_back (i);
}

static void
collect_enum (const rpc_sym *s)
{
  const rpc_enum *rs = s->senum.addr ();
  for (const rpc_const *rc = rs->tags.base (); rc < rs->tags.lim (); rc++) {
    collect_constant (rc->id, "RPC_CONSTANT_ENUM");
  }
}

static void
collect_pound_def (str s)
{
  static rxx x ("#\\s*define\\s*(\\S+)\\s+(.*)");
  if (guess_defines && x.match (s)) {
    collect_constant (x[1], "RPC_CONSTANT_POUND_DEF");
  }
}

static void
collect_prog (const rpc_program *rs)
{
  collect_constant (rs->id, "RPC_CONSTANT_PROG");
  for (const rpc_vers *rv = rs->vers.base (); rv < rs->vers.lim (); rv++) {
    collect_constant (rv->id, "RPC_CONSTANT_VERS");
    for (const rpc_proc *rp = rv->procs.base (); rp < rv->procs.lim (); rp++) {
      collect_constant (rp->id, "RPC_CONSTANT_PROC");
    }
  }
}

str
make_csafe_filename (str fname)
{
  strbuf hdr;
  const char *fnp, *cp;

  if ((fnp = strrchr (fname.cstr(), '/')))
    fnp++;
  else fnp = fname;

  // strip off the suffix ".h" or ".C"
  for (cp = fnp; *cp && *cp != '.' ; cp++ ) ;
  size_t len = cp - fnp;

  mstr out (len + 1);
  for (size_t i = 0; i < len; i++) {
    if (fnp[i] == '-') { out[i] = '_'; }
    else { out[i] = fnp[i]; }
  }
  out[len] = 0;
  out.setlen (len);

  return out;
}

str 
make_constant_collect_hook (str fname)
{
  strbuf b;
  str csafe_fname = make_csafe_filename (fname);
  b << csafe_fname << "_constant_collect";
  return b;
}

static void
dump_constant_collect_hook (str fname)
{
  str cch = make_constant_collect_hook (fname);
  aout << "void\n"
       << cch << " (rpc_constant_collector_t *rcc)\n"
       << "{\n";
  for (size_t i = 0; i < rpc_constants.size (); i++) {
    const rpc_constant_t &rc = rpc_constants[i];
    aout << "  rcc->collect (\"" << rc.id << "\", "
	 << rc.id << ", " << rc.typ << ");\n";
  }
  for (size_t i = 0; i < rpc_types.size (); i++) {
    str id = rpc_types[i];
    aout << "  rcc->collect (\"" << id << "\", "
	 << "xdr_procpair_t (" << id << "_alloc, xdr_" << id << "));\n";
  }

  aout << "}\n\n";
}

//-----------------------------------------------------------------------

static void
mkns (const rpc_namespace *ns)
{
  for (const rpc_program *rp = ns->progs.base (); rp < ns->progs.lim (); rp++) {
    mktbl (rp);
    collect_prog (rp);
  }
}

//-----------------------------------------------------------------------

static void
dumpsym (const rpc_sym *s)
{
  switch (s->type) {
  case rpc_sym::STRUCT:
    mkmshl (s->sstruct->id);
    break;
  case rpc_sym::UNION:
    mkmshl (s->sunion->id);
    break;
  case rpc_sym::ENUM:
    mkmshl (s->senum->id);
    collect_enum (s);
    break;
  case rpc_sym::TYPEDEF:
    mkmshl (s->stypedef->id);
    break;
  case rpc_sym::PROGRAM:
    {
      const rpc_program *rp = s->sprogram.addr ();
      mktbl (rp);
      collect_prog (rp);
    }
    break;
  case rpc_sym::NAMESPACE:
    mkns (s->snamespace);
    break;
  case rpc_sym::LITERAL:
    collect_pound_def (*s->sliteral);
  default:
    break;
  }
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
print_enum (const rpc_enum *s)
{
  aout <<
    "const strbuf &\n"
    "rpc_print (const strbuf &sb, const " << s->id << " &obj, "
    "int recdepth,\n"
    "           const char *name, const char *prefix)\n"
    "{\n"
    "  const char *p;\n"
    "  switch (obj) {\n";
  for (const rpc_const *cp = s->tags.base (),
	 *ep = s->tags.lim (); cp < ep; cp++)
    aout <<
      "  case " << cp->id << ":\n"
      "    p = \"" << cp->id << "\";\n"
      "    break;\n";
  aout <<
    "  default:\n"
    "    p = NULL;\n"
    "    break;\n"
    "  }\n"
    "  if (name) {\n"
    "    if (prefix)\n"
    "      sb << prefix;\n"
    "    sb << \"" << s->id << " \" << name << \" = \";\n"
    "  };\n"
    "  if (p)\n"
    "    sb << p;\n"
    "  else\n"
    "    sb << int (obj);\n"
    "  if (prefix)\n"
    "    sb << \";\\n\";\n"
    "  return sb;\n"
    "};\n";
  print_print (s->id);
}

static void
print_struct (const rpc_struct *s)
{
  const rpc_decl *dp = s->decls.base (), *ep = s->decls.lim ();
  size_t num = ep - dp;
  aout <<
    "const strbuf &\n"
    "rpc_print (const strbuf &sb, const " << s->id << " &obj, "
    "int recdepth,\n"
    "           const char *name, const char *prefix)\n"
    "{\n"
    "  if (name) {\n"
    "    if (prefix)\n"
    "      sb << prefix;\n"
    "    sb << \"" << s->id << " \" << name << \" = \";\n"
    "  };\n"
    "  str npref;\n"
    "  if (prefix) {\n"
    "    npref = strbuf (\"%s  \", prefix);\n"
    "    sb << \"{\\n\";\n"
    "  } else {\n"
    "    sb << \"{ \";\n"
    "  }\n";

  if (num > 1) {
    aout <<
      "  const char *sep = NULL;\n"
      "  if (prefix) {\n"
      "    sep = \"\";\n"
      "  } else {\n"
      "    sep = \", \";\n"
      "  }\n" ;
  }

  if (dp < ep)
    aout <<
      "  rpc_print (sb, obj." << dp->id << ", recdepth, "
      "\"" << dp->id << "\", npref.cstr());\n";
  while (++dp < ep)
    aout <<
      "  sb << sep;\n"
      "  rpc_print (sb, obj." << dp->id << ", recdepth, "
      "\"" << dp->id << "\", npref.cstr());\n";
  aout <<
    "  if (prefix)\n"
    "    sb << prefix << \"};\\n\";\n"
    "  else\n"
    "    sb << \" }\";\n"
    "  return sb;\n"
    "}\n";
  print_print (s->id);
}

static void
print_case (str prefix, const rpc_union *rs, const rpc_utag *rt)
{
  if (rt->tag.type != "void")
    aout
      << prefix << "sb << sep;\n"
      << prefix << "rpc_print (sb, *obj." << rt->tag.id << ", "
      " recdepth, \"" << rt->tag.id << "\", npref.cstr());\n";
  aout << prefix << "break;\n";
}

static void
print_break (str prefix, const rpc_union *rs)
{
  aout << prefix << "break;\n";
}

static bool will_need_sep(const rpc_union* rs) {
  for (const rpc_utag *rt = rs->cases.base (); rt < rs->cases.lim (); rt++) {
    if (rt->tag.type != "void")
        return true;
  }
  return false;
}

static void
print_union (const rpc_union *s)
{
  bool ns = will_need_sep(s);
  aout <<
    "const strbuf &\n"
    "rpc_print (const strbuf &sb, const " << s->id << " &obj, "
    "int recdepth,\n"
    "           const char *name, const char *prefix)\n"
    "{\n"
    "  if (name) {\n"
    "    if (prefix)\n"
    "      sb << prefix;\n"
    "    sb << \"" << s->id << " \" << name << \" = \";\n"
    "  };\n"
    << ((ns) ? "  const char *sep;\n" : "") <<
    "  str npref;\n"
    "  if (prefix) {\n"
    "    npref = strbuf (\"%s  \", prefix);\n" 
    << ((ns) ? "    sep = \"\";\n" : "") <<
    "    sb << \"{\\n\";\n"
    "  }\n"
    "  else {\n"
    << ((ns) ? "    sep = \", \";\n" : "") <<
    "    sb << \"{ \";\n"
    "  }\n"
    "  rpc_print (sb, obj." << s->tagid << ", recdepth, "
    "\"" << s->tagid << "\", npref.cstr());\n";
  pswitch ("  ", s, "obj." << s->tagid, print_case, "\n", print_break);
  aout <<
    "  if (prefix)\n"
    "    sb << prefix << \"};\\n\";\n"
    "  else\n"
    "    sb << \" }\";\n"
    "  return sb;\n"
    "}\n";
  print_print (s->id);
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
    print_print (s->stypedef->id);
  default:
    break;
  }
}

static str
makehdrname (str fname)
{
  strbuf hdr;
  const char *p;

  if ((p = strrchr (fname.cstr(), '/')))
    p++;
  else p = fname;

  hdr.buf (p, strlen (p) - 1);
  hdr.cat ("h");

  return hdr;
}

void
gencfile (str fname)
{
  aout << "// -*-c++-*-\n"
       << "/* This file was automatically generated by rpcc. */\n\n"
       << "#include \"" << makehdrname (fname) << "\"\n\n";

#if 0
  for (const rpc_sym *s = symlist.base (); s < symlist.lim (); s++)
    if (s->type == rpc_sym::PROGRAM)
      for (const rpc_vers *rv = s->sprogram->vers.base ();
	   rv < s->sprogram->vers.lim (); rv++)
	for (const rpc_proc *rp = rv->procs.base ();
	     rp < rv->procs.lim (); rp++) {
	  needtype.insert (rp->arg);
	  needtype.insert (rp->res);
	}
#endif

  aout << "#ifdef MAINTAINER\n\n";
  for (const rpc_sym *s = symlist.base (); s < symlist.lim (); s++)
    dumpprint (s);
  aout << "#endif /* MAINTAINER*/\n";

  for (const rpc_sym *s = symlist.base (); s < symlist.lim (); s++)
    dumpsym (s);

  dump_constant_collect_hook (fname);
  
  aout << "\n";
}

