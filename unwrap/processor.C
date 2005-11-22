
#include "unwrap.h"
#include "rxx.h"

var_t::var_t (const str &t, ptr<declarator_t> d, vartyp_t a)
  : _type (t, d->pointer ()), _name (d->name ()), _asc (a), 
    _initializer (d->initializer ()) {}

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
var_t::decl (const str &p, int n) const
{
  strbuf b;
  b << _type.to_str () << p << n;
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
unwrap_fn_t::closure_generic ()
{
  return var_t ("ptr<closure_t>", NULL, "__cls_g");
}

var_t
unwrap_fn_t::trig ()
{
  return var_t ("ptr<trig_t>", NULL, "trig");
}

var_t
unwrap_fn_t::mk_closure () const
{
  strbuf b;
  b << _name_mangled << "__closure_t";

  return var_t (b, "*", "__cls");
}

str
unwrap_fn_t::decl_casted_closure (bool do_lhs) const
{
  strbuf b;
  if (do_lhs) {
    b << "  " << _closure.decl ()  << " =\n";
  }
  b << "    reinterpret_cast<" << _closure.type ().to_str () 
    << "> (static_cast<closure_t *> (" << closure_generic ().name () << "));";
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
  b << closure_nm () << "->_args." << i ;
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
vartab_t::initialize (strbuf &b, bool self) const
{
  bool first = true;
  for (u_int i = 0; i < size (); i++) {
    if (self || _vars[i].initializer ()) {
      if (!first) b << ", ";
      first = false;
      b << _vars[i].name () << " (";
      if (self) {
	b << _vars[i].name ();
      } else {
	b << _vars[i].initializer ();
      }
      b << ")";
    }
  }
}

void
vartab_t::paramlist (strbuf &b, bool types) const
{
  for (u_int i = 0; i < size () ; i++) {
    if (i != 0) b << ", ";
    if (types) {
      b << _vars[i].decl ();
    } else {
      b << _vars[i].name ();
    }
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
// sanity check functions

bool
backref_list_t::check (u_int sz) const
{
  for (u_int i = 0; i < _lst.size (); i++) {
    if (_lst[i].ref_index () == 0) {
      strbuf b;
      b << "Back reference on line " << _lst[i].lineno ()
	<< " is 0-indexed, but should be 1-indexed!";
      yyerror (str (b));
    } else if (_lst[i].ref_index () > sz) {
      strbuf b;
      b << "Back reference on line " << _lst[i].lineno ()
	<< " overshoots wrapped-in arguments!";
      yyerror (str (b));
    }
  }
  return true;
}


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
  if (_wrap_in) {
    for (u_int i = 0; i < _wrap_in->size (); i++) {
      b << ", " << (*_wrap_in)[i].decl ("_a", i+1);
    }
  }

  if (_call_with) {
    for (u_int i = 0; i < _call_with->size (); i++) {
      b << ", " << (*_call_with)[i].decl ("_b", i+1);
    }
  }
  b << ")\n"
    << "  {\n";

  if (_call_with) {
    for (u_int i = 0; i < _call_with->size (); i++) {
      const var_t &v = (*_call_with)[i];
      b << "    ";
      switch (v.get_asc ()) {
      case ARG:
	b << "_args." << v.name ();
	break;
      case STACK:
	b << "_stack." << v.name ();
	break;
      case CLASS:
	b << "_class_tmp." << v.name ();
	break;
      case EXPR:
	b << v.name ();
	break;
      default:
	assert (false);
      }
      b << " = _b" << (i+1) << ";\n";
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
unwrap_fn_t::output_closure (int fd)
{
  strbuf b;
  b << "class " << _closure.type ().base_type ()  << " : public closure_t\n"
    << "{\n"
    << "public:\n"
    << "  " << _closure.type ().base_type () << " ("
    ;
  _args->paramlist (b);

  b << ") : closure_t (), _stack ("
    ;
  _args->paramlist (b, false);

  b << "), _args ("
    ;
  _args->paramlist (b, false);

  b << ") {}\n"
    ;

  int i = 1;
  for (unwrap_callback_t *cb = _cbs.first ; cb; cb = _cbs.next (cb)) {
    cb->output_in_class (b, i++);
  }

  b << "  struct stack_t {\n"
    << "    stack_t (";
  _args->paramlist (b, true);
  b << ")"
    ;

  if (_stack_vars.size ()) {
    strbuf i;
    _stack_vars.initialize (i, false);
    str s (i);
    if (s && s.len () > 0) {
      b << " : " << s << " ";
    }
  }
  b << " {}\n";
    
  _stack_vars.declarations (b, "    ");
  b << "  };\n";

  b << "\n"
    << "  struct args_t {\n"
    << "    args_t (" ;

  if (_args->size ()) 
    _args->paramlist (b, true);

  b << ")";
  
  if (_args->size ()) {
    b << " : ";
    _args->initialize (b, true);
  }
  
  b << " {}\n";
  
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
      << closure_nm () << "->_stack." << v.name () << ";\n" ;
  } 
}

void
unwrap_fn_t::output_jump_tab (strbuf &b)
{
  b << "  switch (" << closure_generic ().name () << "->jumpto ()) {\n"
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
  b << closure_generic ().decl () << ")\n"
    << "{\n"
    << "  " << _closure.decl () << ";\n"
    << "  if (!" << closure_generic ().name() << ") {\n"
    << "    ptr<" << _closure.type ().base_type () << "> tmp" 
    << " = New refcounted<" << _closure.type ().base_type () << "> ("
    ;
  _args->paramlist (b, false);
  b << ");\n"
    << "    " << closure_generic (). name () << " = tmp;\n"
    << "    " << closure_nm () << " = tmp;\n"
    << "  } else {\n"
    << "    " << _closure.name () << " = " << decl_casted_closure (false)
    << "\n"
    << "  }\n\n"
    ;

  output_stack_vars (b);
  b << "\n";

  output_jump_tab (b);

  b.tosuio ()->output (fd);

  // XXX hack : close the function with pass-through tokens
  // (see parse.yy for more details)
}

void 
unwrap_fn_t::output (int fd)
{
  output_closure (fd);
  output_fn_header (fd);
}

void 
unwrap_shotgun_t::output (int fd)
{
  strbuf b;
  b << "  {\n";
  const vartab_t *args = _fn->args ();
  for (u_int i = 0; i < args->size (); i++) {
    b << "    " <<  _fn->closure_nm () << "->_args." << args->_vars[i].name ()
      << " = " << args->_vars[i].name () << ";\n";
  }
  if (_fn->classname ()) 
    b << "    " << _fn->closure_nm () << "->_self = this;\n";
  b << "    " << _fn->closure_generic ().name () << "->set_jumpto (" << _id 
    << ");\n"
    << "\n";

  b << "    " << _fn->trig ().decl () 
    << " = trig_t::alloc (wrap ("
    << _fn->closure_generic ().name ()  
    << ", &closure_t::reenter));\n";

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
      << _fn->closure ().name () << "->_class_tmp." << v.name () << ";\n";
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
  b << "wrap (" << _parent_fn->closure ().name () << ", "
    << "&" << _parent_fn->closure ().type ().base_type () 
    << "::cb" << _cb_id << ", " << _parent_fn->trig (). name ()
    ;
  if (_wrap_in) {
    for (u_int i = 0; i < _wrap_in->size (); i++) {
      b << ", ";
      b << (*_wrap_in)[i].name ();
    }
  }
  b << ")";
  b.tosuio ()->output (fd);

}

//
//-----------------------------------------------------------------------
