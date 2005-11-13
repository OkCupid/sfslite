
#include "unwrap.h"
#include "rxx.h"

var_t::var_t (const str &t, ptr<declarator_t> d)
  : _type (t, d->pointer ()), _name (d->name ()), _asc (NONE) {}

const var_t *
vartab_t::lookup (const str &n) const
{
  const u_int *ind = _tab[n];
  if (!ind) return NULL;
  return &_vars[*ind];
}

str
type_t::to_str () const
{
  strbuf b;
  b << _base_type << " ";
  if (_pointer)
    b << _pointer;
  return b;
}

str
var_t::decl () const
{
  strbuf b;
  b << _type.to_str () << _name;
  return b;
}

str
var_t::ref_decl () const
{
  strbuf b;
  b << _type.to_str () << "&" << _name;
  return b;
}

str
mangle (const str &in)
{
  const char *i;
  char *o;
  mstr m (in.len ());
  for (i = in.cstr (), o = m.cstr (); *i; i++, o++) {
    *o = (*i == ':' || *i == '<' || *i == '>' || *i == ',') ? '_' : *i;
  }
  return m;
}

str 
strip_to_method (const str &in)
{
  static rxx mthd_rxx ("([^:]+::)+");

  if (mthd_rxx.search (in)) {
    return str (in.cstr () + mthd_rxx[1].len ());
  }
  return in;
}

str 
strip_off_method (const str &in)
{
  static rxx mthd_rxx ("([^:]+::)+");

  if (mthd_rxx.search (in)) 
    return str (in.cstr (), mthd_rxx[1].len () - 2);

  return NULL;
}


bool
vartab_t::add (var_t v)
{
  if (_tab[v.name ()]) {
    return false;
  }

  _vars.push_back (v);
  _tab.insert (v.name (), _vars.size () - 1);

  return true;
}


void
declarator_t::dump () const
{
  warn << "declarator dump:\n";
  if (_name)
    warn << "  name: " << _name << "\n";
  if (_pointer)
    warn << "  pntr: " << _pointer << "\n";
  if (_params)
    warn << "  param list size: " << _params->size () << "\n";
}

void
parse_state_t::passthrough (const str &s)
{
  if (!*_elements.plast || !(*_elements.plast)->append (s)) 
    _elements.insert_tail (New unwrap_passthrough_t (s));

}

void
parse_state_t::new_shotgun (unwrap_shotgun_t *g)
{
  _shotgun = g;
  _elements.insert_tail (g);
}

//-----------------------------------------------------------------------
// Output utility routines
//

var_t
unwrap_fn_t::freezer_generic ()
{
  return var_t ("ptr<freezer_t>", NULL, "__frz_g");
}

var_t
unwrap_fn_t::trig ()
{
  return var_t ("ptr<trig_t>", NULL, "trig");
}

var_t
unwrap_fn_t::mk_freezer () const
{
  strbuf b;
  b << _name_mangled << "__freezer_t";

  return var_t (b, "*", "__frz");
}

str
unwrap_fn_t::decl_casted_freezer (bool do_lhs) const
{
  strbuf b;
  if (do_lhs) {
    b << "  " << _freezer.decl ()  << " =\n";
  }
  b << "    reinterpret_cast<" << _freezer.type ().to_str () 
    << "> (static_cast<freezer_t *> (" << freezer_generic ().name () << "));";
  return b;
}

str
unwrap_fn_t::reenter_fn () const
{
  strbuf b;
  b << _name_mangled << "__renter";
  return b;
}

str
unwrap_fn_t::frozen_arg (const str &i) const
{
  strbuf b;
  b << frznm () << "->_args." << i ;
  return b;
}

void
vartab_t::declarations (strbuf &b, const str &padding) const
{
  for (u_int i = 0; i < size (); i++) {
    b << padding << _vars[i].decl () << ";\n";
  }
}

void
vartab_t::paramlist (strbuf &b) const
{
  for (u_int i = 0; i < size () ; i++) {
    if (i != 0) b << ", ";
    b << _vars[i].decl ();
  }
}

str
unwrap_fn_t::label (u_int id) const
{
  strbuf b;
  b << _name_mangled << "__label" << id ;
  return b;
}

//
//
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// Output Routines

void 
unwrap_passthrough_t::output (int fd)
{
  _buf.tosuio ()->output (fd);
}
  
void 
unwrap_callback_t::output_in_class (strbuf &b, int n)
{
  _cb_id = n;

  b << "  void cb" << n  << " ("
    << unwrap_fn_t::trig ().decl () ;
  if (_vars) {
    for (u_int i = 0; i < _vars->size (); i++) {
      b << ", ";
      const var_t &v = _vars->_vars[i];
      switch (v.get_asc ()) {
      case ARG:
	b << _parent_fn->args ()->lookup (v.name ())->decl ();
	break;
      case STACK:
	b << _parent_fn->stack_vars ()->lookup (v.name ())->decl ();
	break;
      case CLASS:
	b << v.decl ();
	_shotgun->add_class_var (v);
	break;
      default:
	yyerror ("unknown variable type / unexepected runtime error");
	break;
      }
    }
  }

  b << ")\n"
    << "  {\n";
  if (_vars) {
    for (u_int i = 0; i < _vars->_vars.size (); i++) {
      const var_t &v = _vars->_vars[i];
      b << "    ";
      switch (v.get_asc ()) {
      case ARG:
	b << "_args";
	break;
      case STACK:
	b << "_stack";
	break;
      case CLASS:
	b << "_class_tmp";
	break;
      default:
	assert (false);
      }
      b << "." << _vars->_vars[i].name () << " = "  
	<< _vars->_vars[i].name () << ";\n";
    }
  }
  b  << "  }\n\n";
}

void
unwrap_fn_t::output_reenter (strbuf &b)
{
  b << "  void reenter ()\n"
    << "  {\n"
    << "    ";

  if (_class)
    b << "_self->";

  b << _method_name << " (";

  for (u_int i = 0; _args && i < _args->_vars.size (); i++) {
    b << "_args." << _args->_vars[i].name ();
    b << ", ";
  }
  b << "mkref (this));\n"
    << "  }\n\n";
}

void
unwrap_fn_t::output_freezer (int fd)
{
  strbuf b;
  b << "class " << _freezer.type ().base_type ()  << " : public freezer_t\n"
    << "{\n"
    << "public:\n"
    << "  " << _freezer.type ().base_type () << " () : freezer_t () {}\n"
    << "\n"
    ;

  int i = 1;
  for (unwrap_callback_t *cb = _cbs.first ; cb; cb = _cbs.next (cb)) {
    cb->output_in_class (b, i++);
  }

  b << "  struct stack_t {\n"
    << "    stack_t () {}\n"
    ;
  _stack_vars.declarations (b, "    ");
  b << "  };\n";

  b << "\n"
    << "  struct args_t {\n"
    << "    args_t () {}\n"
    ;
  _args->declarations (b, "    ");
  b << "  };\n";

  b << "\n"
    << "  struct class_tmp_t {\n"
    << "    class_tmp_t () {}\n"
    ;
  _class_vars_tmp.declarations (b, "    ");
 
  b << "  };\n\n";

  output_reenter (b);

  b << "  stack_t _stack;\n"
    << "  class_tmp_t _class_tmp;\n"
    << "  args_t _args;\n" ;

  if (_class) {
    b << "  " << _class << " *_self;\n";
  }

  b << "};\n\n";

  b.tosuio ()->output (fd);
}

void
unwrap_fn_t::output_stack_vars (strbuf &b)
{
  for (u_int i = 0; i < _stack_vars.size (); i++) {
    const var_t &v = _stack_vars._vars[i];
    b << "  " << v.ref_decl () << " = " 
      << frznm () << "->_stack." << v.name () << ";\n" ;
  } 
}


void
unwrap_fn_t::output_jump_tab (strbuf &b)
{
  b << "  switch (" << freezer_generic ().name () << "->jumpto ()) {\n"
    ;
  for (u_int i = 0; i < _shotguns.size (); i++) {
    int id = i + 1;
    b << "  case " << id << ":\n"
      << "    goto " << label (id) << ";\n"
      << "    break;\n";
    _shotguns[i]->set_id (id);
  }
  b << "  default:\n"
    << "    break;\n"
    << "  }\n";
}

void
unwrap_fn_t::output_fn_header (int fd)
{
  strbuf b;
  b << _ret_type.to_str () << "\n"
    << _name << "(";
  if (_args) {
    _args->paramlist (b);
    b << ", ";
  }
  b << freezer_generic ().decl () << ")\n"
    << "{\n"
    << "  " << _freezer.decl () << ";\n"
    << "  if (!" << freezer_generic ().name() << ") {\n"
    << "    ptr<" << _freezer.type ().base_type () << "> tmp" 
    << " = New refcounted<" << _freezer.type ().base_type () << "> ();\n"
    << "    " << freezer_generic (). name () << " = tmp;\n"
    << "    " << frznm () << " = tmp;\n"
    << "  } else {\n"
    << "    " << _freezer.name () << " = " << decl_casted_freezer (false)
    << "\n"
    << "  }\n\n"
    ;

  output_stack_vars (b);
  b << "\n";

  output_jump_tab (b);

  b.tosuio ()->output (fd);

  // XXX hack : close the function with pass-through tokens
}


void 
unwrap_fn_t::output (int fd)
{
  output_freezer (fd);
  output_fn_header (fd);
}

void 
unwrap_shotgun_t::output (int fd)
{
  strbuf b;
  b << "  {\n";
  const vartab_t *args = _fn->args ();
  for (u_int i = 0; i < args->size (); i++) {
    b << "    " <<  _fn->frznm () << "->_args." << args->_vars[i].name ()
      << " = " << args->_vars[i].name () << ";\n";
  }
  if (_fn->classname ()) 
    b << "    " << _fn->frznm () << "->_self = this;\n";
  b << "    " << _fn->freezer_generic ().name () << "->set_jumpto (" << _id 
    << ");\n"
    << "\n";

  b << "    " << _fn->trig ().decl () 
    << " = trig_t::alloc (wrap ("
    << _fn->freezer_generic ().name ()  
    << ", &freezer_t::reenter));\n";

  b.tosuio ()->output (fd);
  b.tosuio ()->clear ();
  for (unwrap_el_t *el = _elements.first; el; el = _elements.next (el)) {
    el->output (fd);
  }

  b << "\n"
    << "    return;\n"
    << "  }\n"
    << " " << _fn->label (_id) << ":\n"
    ;

  for (u_int i = 0; i < _class_vars.size (); i++) {
    const var_t &v = _class_vars._vars[i];
    b << "    " << v.name () << " = " 
      << _fn->freezer ().name () << "->_class_tmp." << v.name () << ";\n";
  }

  b.tosuio ()->output (fd);
}

void
parse_state_t::output (int fd)
{
  for (unwrap_el_t *el = _elements.first; el; el = _elements.next (el)) {
    el->output (fd);
  }
}

void
unwrap_callback_t::output (int fd)
{
  strbuf b;
  b << "wrap (" << _parent_fn->freezer ().name () << ", "
    << "&" << _parent_fn->freezer ().type ().base_type () 
    << "::cb" << _cb_id << ", " << _parent_fn->trig (). name ()
    << ")";
  b.tosuio ()->output (fd);

}




//
//-----------------------------------------------------------------------
