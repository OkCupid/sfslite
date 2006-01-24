
#include "tame.h"
#include "rxx.h"
#include <ctype.h>

//-----------------------------------------------------------------------
// output only the generic callbacks that we need, to speed up compile
// times.
//
bhash<u_int> _generic_cb_tab;

static u_int cross (u_int a, u_int b) 
{
  assert (a <= 0xff && b <= 0xff);
  return ((a << 8) | b);
}

bool generic_cb_exists (u_int a, u_int b) 
{ return _generic_cb_tab[cross (a,b)]; }

void generic_cb_declare (u_int a, u_int b)
{ _generic_cb_tab.insert (cross (a,b)); }

//
//-----------------------------------------------------------------------


var_t::var_t (const str &t, ptr<declarator_t> d, vartyp_t a)
  : _type (t, d->pointer ()), _name (d->name ()), _asc (a), 
    _initializer (d->initializer ()) {}

//
// note that a callback ID is only necessary in the (optimized case)
// of adding a callback to a BLOCK {...} block within a function
//
tame_block_callback_t::tame_block_callback_t (u_int ln, tame_fn_t *f, 
						  tame_block_t *g, 
						  ptr<expr_list_t> l) 
  : tame_callback_t (ln, l), _parent_fn (f), _block (g),
    _cb_ind (_parent_fn->add_callback (this))
{}

str ws_strip (str s)
{
  static rxx wss ("\\s*(.*)\\s*");
  assert (wss.match (s));
  return wss[1];
}

str template_args (str s)
{
  if (!s) return s;
  static rxx txx (".*(<.*>).*");
  if (txx.search (s)) return txx[1];
  else return NULL;
}

static void output_el (int fd, tame_el_t *e) { e->output (fd); }
void element_list_t::output (int fd) { _lst.traverse (wrap (output_el, fd)); }

// Must match "__CLS" in tame_const.h
#define TAME_CLOSURE_NAME     "__cls"


#define CLOSURE_RFCNT         "__cls_r"
#define CLOSURE_GENERIC       "__cls_g"
#define TAME_PREFIX           "__tame_"

str
type_t::type_without_pointer () const
{
  strbuf b;
  b << _base_type;
  if (_template_args)
    b << _template_args << " ";
  return b;
}

str
type_t::mk_ptr () const
{
  my_strbuf_t b;
  b << "ptr<";
  b.mycat (type_without_pointer()) << " >";
  return b;
}

str
type_t::alloc_ptr (const str &n, const str &args) const
{
  my_strbuf_t b;
  b.mycat (mk_ptr ()) << " " << n << " = New refcounted<";
  b.mycat (type_without_pointer ()) << " > (" << args << ")";
  return b;
}

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
type_t::to_str_w_template_args () const
{
  strbuf b;
  b << _base_type;
  if (_template_args)
    b << _template_args;
  b << " ";

  if (_pointer)
    b << _pointer;

  return b;
}

str
var_t::decl () const
{
  strbuf b;
  b << _type.to_str_w_template_args () << _name;
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
var_t::decl (const str &p) const
{
  strbuf b;
  b << _type.to_str ();
  if (p)
    b << p;
  b << _name;
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
  for (i = in.cstr (), o = m.cstr (); *i; i++) {
    if (!isspace (*i)) {
      *o = (*i == ':' || *i == '<' || *i == '>' || *i == ',') ? '_' : *i;
      o++;
    }
  }
  m.setlen (o - m.cstr ());
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
element_list_t::passthrough (const lstr &s)
{
  if (!*_lst.plast || !(*_lst.plast)->append (s)) 
    _lst.insert_tail (New tame_passthrough_t (s));

}

void
parse_state_t::new_block (tame_block_t *g)
{
  _block = g;
  push (g);
}

void
parse_state_t::new_join (tame_join_t *j)
{
  _join = j;
  push (j);
}

void
parse_state_t::new_nonblock (tame_nonblock_t *b)
{
  _nonblock = b;
  push (b);
}

void
tame_fn_t::add_env (tame_env_t *e)
{
  _envs.push_back (e); 
  if (e->is_jumpto ()) 
    e->set_id (++_n_labels);
}

bool
tame_fn_t::do_cceoc () const
{
  return _args && _args->lookup (cceoc_argname);
}

str
tame_fn_t::cceoc_typename () const 
{
  const var_t *v = _args->lookup (cceoc_argname);
  return (v ? v->type ().to_str () : NULL);
}

//-----------------------------------------------------------------------
// Output utility routines
//

var_t
tame_fn_t::closure_generic ()
{
  return var_t ("ptr<closure_t>", NULL, CLOSURE_GENERIC);
}

var_t
tame_fn_t::trig ()
{
  return var_t ("ptr<trig_t>", NULL, "trig");
}

var_t
tame_fn_t::mk_closure () const
{
  strbuf b;
  b << _name_mangled << "__closure_t";

  return var_t (b, "*", TAME_CLOSURE_NAME, NONE, _template_args);
}

str
tame_fn_t::decl_casted_closure (bool do_lhs) const
{
  strbuf b;
  if (do_lhs) {
    b << "  " << _closure.decl ()  << " =\n";
  }
  b << "    reinterpret_cast<" << _closure.type ().to_str_w_template_args () 
    << "> (static_cast<closure_t *> (" << closure_generic ().name () << "));";
  return b;
}

str
tame_fn_t::reenter_fn () const
{
  strbuf b;
  b << closure ().type ().base_type ()
    << "::reenter";
  return b;
}

str
tame_fn_t::frozen_arg (const str &i) const
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
vartab_t::paramlist (strbuf &b, list_mode_t list_mode, str prfx) const
{
  for (u_int i = 0; i < size () ; i++) {
    if (i != 0) b << ", ";
    switch (list_mode) {
    case DECLARATIONS:
      b << _vars[i].decl (prfx);
      break;
    case NAMES:
      if (prfx)
	b << prfx;
      b << _vars[i].name ();
      break;
    case TYPES:
      {
	b.cat (_vars[i].type ().to_str ().cstr (), true);
	break;
      }
    default:
      assert (false);
      break;
    }
  }
}

str
tame_fn_t::label (str s) const
{
  strbuf b;
  b << _name_mangled << "__label_" << s;
  return b;
}

str
tame_fn_t::label (u_int id) const
{
  strbuf b;
  b << id;
  return label (b);
}


//
//
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
// Output Routines

void 
tame_passthrough_t::output (int fd)
{
  if (_strs.size ()) 
    state.output_line_xlate (fd, _strs[0].lineno ());
  _buf.tosuio ()->output (fd);
}

str
tame_nonblock_callback_t::cb_name () const
{
  strbuf b;
  u_int N_w = n_args ();
  u_int N_p = _call_with->size ();
  b << "__nonblock_cb_" << N_w << "_" << N_p;
  return b;
}

void
tame_nonblock_callback_t::output_generic (strbuf &b)
{
  u_int N_w = n_args ();
  u_int N_p = _call_with->size ();

  if (generic_cb_exists (N_w, N_p))
    return;
  bool first = true;

  generic_cb_declare (N_w, N_p);
  if (N_p > 0 || N_w > 0) {
    b << "template<";
    for (u_int i = 1; i <= N_p; i++) {
      if (first) first = false;
      else b << ", ";
      b << "class P" << i;
    }
    for (u_int i = 1; i <= N_w; i++) {
      if (first) first = false;
      else b << ", ";
    b << "class W" << i;
    }
    b << "> ";
  }
  b << "static void\n";
  b.cat (cb_name ().cstr (), true);
  b << " (ptr<closure_t> hold, "
    << "ptr<joiner_t<";
  for (u_int i = 1; i <= N_w; i++) {
    if (i != 1) b << ", ";
    b << "W" << i;
  }
  b << "> > j";
  
  if (N_p) {
    b << ",\n";
    b << "\t\tpointer_set" << N_p << "_t<";
    for (u_int i = 1; i <= N_p; i++) {
      if (i != 1) b << ", ";
      b << "P" << i;
    }
    b << "> p";
  }

  // output a value set even if there are no values to wrap in;
  // for now this makes things much simpler.
  b << ",\n";
  b << "\t\tvalue_set_t<";
  for (u_int i = 1; i <= N_w; i++) {
    if (i != 1) b << ", ";
    b << "W" << i;
  }
  b << "> w";

  if (N_p) {
    b << ",\n\t\t";
    for (u_int i = 1; i <= N_p; i++) {
      if (i != 1) b << ", ";
      b << "P" << i << " v" << i;
    }
  }
  b << ")\n{\n";
  for (u_int i = 1; i <= N_p; i++) {
    b << "  *p.p" << i << " = v" << i << ";\n";
  }
  b << "\n";
  b << "  j->join (w);\n"
    << "}\n\n";
}
  
void 
tame_block_callback_t::output_in_class (strbuf &b)
{
  b << "  void cb" << _cb_ind  << " () {\n";

  str loc = state.loc (_line_number);
  b << "    if (-- _cb_num_calls" << _cb_ind << " < 0 ) {\n"
    << "      tame_error (\"" << loc << "\", \"callback overcalled!\");\n"
    << "    }\n";

  b << "    if (!--_block" << _block->id () << ")\n"
    << "      delaycb (0, 0, wrap (mkref (this), &"
    ;
  b.cat (_parent_fn->reenter_fn ().cstr (), true);
  b << "));\n";
  b << "  }\n\n";
}

void
tame_fn_t::output_reenter (strbuf &b)
{
  b << "  void reenter ()\n"
    << "  {\n"
    ;

  b << "    ";
  if (_class) {
    b << "(";
    if (!(_opts & STATIC_DECL)) {
      b << "(*_self).";
    }
    b << "*_method) ";
  } else {
    b << _name ;
  }

  b << " (";

  for (u_int i = 0; _args && i < _args->_vars.size (); i++) {
    b << "_args." << _args->_vars[i].name ();
    b << ", ";
  }
  b << "mkref (this));\n"
    << "  }\n\n";
}

void
tame_fn_t::output_set_method_pointer (my_strbuf_t &b)
{
  b << "  typedef " ;
  b.mycat (_ret_type.to_str ()) << " (";
  if (!(_opts & STATIC_DECL)) {
    b << _class << "::";
  }
  b << "*method_type_t) (";
  if (_args) {
    _args->paramlist (b, TYPES);
    b << ", ";
  }
  b << "ptr<closure_t>);\n";

  b << "  void set_method_pointer (method_type_t m) { _method = m; }\n\n";
    
}

static void
output_is_onstack (strbuf &b)
{
  b << "  bool is_onstack (const void *p) const\n"
    << "  {\n"
    << "    return (static_cast<const void *> (&_stack) <= p &&\n"
    << "            static_cast<const void *> (&_stack + 1) > p);\n"
    << "  }\n";
}

void
tame_fn_t::output_closure (int fd)
{
  my_strbuf_t b;

  if (_template) {
    b.mycat (template_str ()) << "\n";
  }

  b << "class " << _closure.type ().base_type () 
    << " : public closure_t "
    << "{\n"
    << "public:\n"
    << "  " << _closure.type ().base_type () 
    << " (";

  if (need_self ()) {
    b.cat (_self.decl (), true);
    if (_args)
      b << ", ";
  }

  if (_args) {
    _args->paramlist (b, DECLARATIONS);
  }

  str cceoc = str (do_cceoc () ? "true" : "false");

  b << ") : closure_t (" << cceoc << "), ";
  if (need_self ()) {
    str s = _self.name ();
    b.mycat (s) << " (";
    b.mycat (s) << "), ";
  }

  b << " _stack ("
    ;
  if (_args) _args->paramlist (b, NAMES);
  b << "), _args ("
    ;

  if (_args) _args->paramlist (b, NAMES);
  b << ")";

  for ( u_int i = 1; i <= _n_blocks ; i++) {
    b << ", _block" << i << " (0)";
  }

  for ( u_int i = 1; i <= _cbs.size (); i++) {
    b << ", _cb_num_calls" << i << " (0)";
  }

  b << " {}\n\n";


  if (_class) {
    output_set_method_pointer (b);
  }

  for (u_int i = 0; i < _cbs.size (); i++) {
    _cbs[i]->output_in_class (b);
  }

  output_reenter (b);

  // output the stack structure
  b << "  struct stack_t {\n"
    << "    stack_t (";
  if (_args) _args->paramlist (b, DECLARATIONS);
  b << ")" ;

  // output stack declaration
  if (_stack_vars.size ()) {
    strbuf i;
    _stack_vars.initialize (i, false);
    str s (i);
    if (s && s.len () > 0) {
      b << " : " << s << " ";
    }
  }

  b << " {}\n";
    ;
  _stack_vars.declarations (b, "    ");
  b << "  };\n";
 
  // output the argument capture structure
  b << "\n"
    << "  struct args_t {\n"
    << "    args_t (" ;
  if (_args && _args->size ()) 
    _args->paramlist (b, DECLARATIONS);
  b << ")";
  if (_args && _args->size ()) {
    b << " : ";
    _args->initialize (b, true);
  }
  b << " {}\n";
  if (_args)  _args->declarations (b, "    ");
  b << "  };\n";

  if (need_self ()) {
    b << "  ";
    b.mycat (_self.decl ()) << ";\n";
  }
  b << "  stack_t _stack;\n"
    << "  args_t _args;\n" ;

  if (_class)
    b << "  method_type_t _method;\n";

  for (u_int i = 1; i <= _n_blocks; i++) {
    b << "  int _block" << i << ";\n";
  }

  for (u_int i = 1; i <= _cbs.size () ; i++) {
    b << "  int _cb_num_calls" << i << ";\n";
  }

  output_is_onstack (b);

  b << "};\n\n";

  b.tosuio ()->output (fd);
}

void
tame_fn_t::output_stack_vars (strbuf &b)
{
  for (u_int i = 0; i < _stack_vars.size (); i++) {
    const var_t &v = _stack_vars._vars[i];
    b << "  " << v.ref_decl () << " = " 
      << closure_nm () << "->_stack." << v.name () << ";\n" ;
  } 
}

void
tame_fn_t::output_arg_references (strbuf &b)
{
  for (u_int i = 0; _args && i < _args->size (); i++) {
    const var_t &v = _args->_vars[i];
    b << "  " << v.ref_decl () << " = "
      << closure_nm () << "->_args." << v.name () << ";\n";
  }

  // compiler might complain that the variable references aren't
  // being used.  In this case, we need to use them!
  for (u_int i = 0; _args && i < _args->size (); i++) {
    const var_t &v = _args->_vars[i];
    b << "   use_reference (" << v.name () << "); \n";
  }
}

void
tame_fn_t::output_jump_tab (strbuf &b)
{
  b << "  switch (" << TAME_CLOSURE_NAME << "->jumpto ()) {\n"
    ;
  for (u_int i = 0; i < _envs.size (); i++) {
    if (_envs[i]->is_jumpto ()) {
      int id_tmp = _envs[i]->id ();
      assert (id_tmp);
      b << "  case " << id_tmp << ":\n"
	<< "    goto " << label (id_tmp) << ";\n"
	<< "    break;\n";
    }
  }
  b << "  default:\n"
    << "    break;\n"
    << "  }\n";
}

str
tame_fn_t::signature (bool d, str prfx, bool static_flag) const
{
  my_strbuf_t b;
  if (_template)
    b.mycat (template_str ()) << "\n";
  if (static_flag)
    b << "static ";

  b << _ret_type.to_str () << "\n"
    << _name << "(";
  if (_args) {
    _args->paramlist (b, DECLARATIONS, prfx);
    b << ", ";
  }
  b << closure_generic ().decl ();
  if (d)
    b << " = NULL";
  b << ")";

  return b;
}

void
tame_fn_t::output_static_decl (int fd)
{
  my_strbuf_t b;
  b.mycat (signature (true, NULL, true)) << ";\n\n";
  
  b.tosuio ()->output (fd);
}

void
tame_fn_t::output_fn (int fd)
{
  my_strbuf_t b;
  
  state.set_fn (this);
  state.need_line_xlate ();
  state.output_line_xlate (fd, _lineno);

  b << signature (false, TAME_PREFIX)  << "\n"
    << "{\n";
  
  if (do_cceoc ()) {
    b << "  " << _cceoc_sentinel.decl () << ";\n";
  }


  b << "  " << _closure.decl () << ";\n"
    << "  "
    ;
  b.mycat (_closure.type ().mk_ptr ());
  b << " " << CLOSURE_RFCNT << ";\n"
    << "  if (!" << closure_generic ().name() << ") {\n"
    ;

  b << "    " << CLOSURE_RFCNT << " = New refcounted<"
    << _closure.type().type_without_pointer() << "> (";

  if (need_self ()) {
    b << "this";
    if (_args)
      b << ", ";
  }

  if (_args)
    _args->paramlist (b, NAMES, TAME_PREFIX);

  b << ");\n"
    << "    " << TAME_CLOSURE_NAME << " = " << CLOSURE_RFCNT << ";\n"
    << "    " << CLOSURE_GENERIC << " = " << CLOSURE_RFCNT << ";\n";

  if (_class) {
    b << "    " << TAME_CLOSURE_NAME
      << "->set_method_pointer (&" << _name << ");\n";
  }


  b << "  } else {\n"
    << "    " << _closure.name () << " = " << decl_casted_closure (false)
    << "\n"
    << "    " << CLOSURE_RFCNT << " = mkref (" << TAME_CLOSURE_NAME << ");\n"
    << "  }\n\n"
    ;

  output_stack_vars (b);
  b << "\n";
  output_arg_references (b);
  b << "\n";

  output_jump_tab (b);

  b.tosuio ()->output (fd);

  state.need_line_xlate ();

  element_list_t::output (fd);

}

void 
tame_fn_t::output (int fd)
{
  if ((_opts & STATIC_DECL) && !_class)
    output_static_decl (fd);
  output_generic (fd);
  output_closure (fd);
  output_fn (fd);
}

void
tame_fn_t::output_generic (int fd)
{
  strbuf b;
  for (u_int i = 0; i < _nbcbs.size (); i++) {
    _nbcbs[i]->output_generic (b);
  }
  b.tosuio ()->output (fd);
}

void
tame_fn_t::jump_out (strbuf &b, int id)
{
  b << "    " << TAME_CLOSURE_NAME << "->set_jumpto (" << id 
    << ");\n"
    << "\n";
}

void 
tame_block_t::output (int fd)
{
  my_strbuf_t b;
  str tmp;

  b << "  {\n"
    << "    " << TAME_CLOSURE_NAME << "->_block" << _id << " = 1;\n"
    ;

  _fn->jump_out (b, _id);

  b.tosuio ()->output (fd);
  b.tosuio ()->clear ();

  // now we are returning to mainly pass-through code, but with some
  // callbacks thrown in (which won't change the line-spacing)
  state.need_line_xlate ();

  for (tame_el_t *el = _lst.first; el; el = _lst.next (el)) {
    el->output (fd);
  }

  b << "\n"
    << "    if (--" << TAME_CLOSURE_NAME << "->_block" << _id << ")\n"
    << "      ";

  b.mycat (_fn->return_expr ());

  b << ";\n"
    << "  }\n"
    << " " << _fn->label (_id) << ":\n"
    << "    ;\n"
    ;

  // XXX: Workaround to bug, in which static checking that cceoc called once,
  // and Duff's device don't interact well. Eventually we should have
  // a real solution or maybe deactivate 
  if (_fn->did_cceoc_call ()) 
    b << "  SET_CCEOC_STACK_SENTINEL();\n";

  state.need_line_xlate ();
  b.tosuio ()->output (fd);
}

str
tame_fn_t::return_expr () const
{
  if (_default_return) {
    strbuf b;
    b << "do { " << _default_return << "} while (0)";
    return b;
  } else {
    return "return";
  }
}

void
parse_state_t::output_cceoc_argname (int fd)
{
  strbuf b;
  b << "\n#define CCEOC_ARGNAME  " << cceoc_argname << "\n";
  b.tosuio ()->output (fd);
  need_line_xlate ();
}

void
parse_state_t::output (int fd)
{
  output_line_xlate (fd, 1);
  output_cceoc_argname (fd);
  element_list_t::output (fd);
}

bool
expr_list_t::output_vars (strbuf &b, bool first, const str &prfx, 
			  const str &sffx)
{
  for (u_int i = 0; i < size (); i++) {
    if (!first) b << ", ";
    else first = false;
    if (prfx) b << prfx;
    b << (*this)[i].name ();
    if (sffx) b << sffx;
  }
  return first;
}

size_t 
tame_nonblock_callback_t::n_args () const 
{
  combine_lists ();
  return _wrap_in_combined->size () - 1;
}

var_t
tame_nonblock_callback_t::arg (u_int i) const
{
  combine_lists ();
  return (*_wrap_in_combined)[i+1];
}

var_t
tame_nonblock_callback_t::join_group () const
{
  combine_lists ();
  return (*_wrap_in_combined)[0];
}

void
tame_nonblock_callback_t::combine_lists () const
{
#define N_LISTS 2
  if (_wrap_in_combined)
    return;

  _wrap_in_combined = New refcounted<expr_list_t> ();
  ptr<expr_list_t> l[N_LISTS];

  if (_nonblock) l[0] = _nonblock->args ();
  l[1] = _wrap_in;

  for (u_int i = 0; i < N_LISTS; i++) 
    for (size_t j = 0; l[i] && j < l[i]->size (); j++) 
      _wrap_in_combined->push_back ( (*l[i])[j] );

  assert (_wrap_in_combined->size () > 0);

#undef N_LISTS
}

bool
tame_nonblock_callback_t::output_vars (strbuf &b, bool first, 
				       const str &prfx,
				       const str &sffx) const
{
  combine_lists ();
  for (u_int i = 0; i < n_args (); i++) {
    if (!first)  b << ", ";
    else first = false;
    if (prfx) b << prfx;
    b << arg (i).name ();
    if (sffx) b << sffx;
  }
  return first;
}

void
tame_block_callback_t::output (int fd)
{
  int bid = _block->id ();
  my_strbuf_t b;
  b << "(++" << TAME_CLOSURE_NAME << "->_block" << bid << ", "
    << "++" << TAME_CLOSURE_NAME << "->_cb_num_calls" << _cb_ind << ", "
    ;
  if (_call_with->size ()) {
    b << "wrap (__block_cb" << _call_with->size () << "<";

    _call_with->output_vars (b, true, "TTT(", ")");
    b << ">, pointer_set" << _call_with->size () << "_t<";
    _call_with->output_vars (b, true, "TTT(", ")");
    b << "> (";
    _call_with->output_vars (b, true, "&(", ")");
    b << "), ";
  }
  b << "wrap (" << CLOSURE_RFCNT << ", &";
  b.mycat (_parent_fn->closure ().type ().type_without_pointer ())
    << "::cb" << _cb_ind
    << "))";

  // if there are parameters to set, then there is 1 extra callback
  if (_call_with->size ()) 
    b << ")";

  b.tosuio ()->output (fd);
}

void
tame_nonblock_callback_t::output (int fd)
{
  my_strbuf_t b;

  strbuf tmp;
  tmp << "(" << join_group ().name () << ")";
  str jgn = tmp;
  str loc = state.loc (_line_number);
  
  b << "(" << jgn << ".launch_one (" CLOSURE_GENERIC "), "
    << "wrap (";
  b.mycat (cb_name ());

  if (_call_with->size () || n_args ()) {
    b << " <";

    bool first = true;

    first = _call_with->output_vars (b, first, "typeof (", ")");
    first = output_vars (b, first, "typeof (", ")");
    
    b << ">";
  }
  b << ", " CLOSURE_GENERIC ", " << jgn 
    << ".make_joiner (\"" << loc << "\")";

  if (_call_with->size ()) {
    b << ", pointer_set" << _call_with->size () << "_t<";
    _call_with->output_vars (b, true, "typeof (", ")");
    b << "> (";
    _call_with->output_vars (b, true, "&(", ")");
    b << ")";
  }

  // note we ouput an empty value set if there are no values
  // to wrap in.
  b << ", value_set_t<";
  if (n_args ()) 
    output_vars (b, true, "typeof (", ")");
  b << "> (";
  if (n_args ()) 
    output_vars (b, true, NULL, NULL);

  b << ")))";

  b.tosuio ()->output (fd);

}

#define JOIN_VALUE "__v"
void
tame_join_t::output (int fd)
{
  strbuf tmp;
  tmp << "(" << join_group ().name () << ")";
  str jgn = tmp;

  my_strbuf_t b;
  b << "  ";
  b.mycat (_fn->label (_id)) << ":\n";
  b << "    typeof (" << jgn << ".to_vs ()) "
    << JOIN_VALUE << ";\n";
  b << "    if (" <<  jgn << ".pending (&" 
    << JOIN_VALUE << ")) {\n";
  
  for (u_int i = 0; i < n_args (); i++) {
    b << "      typeof (" << JOIN_VALUE << ".v" << i+1 << ") &"
      << arg (i).name ()  << " = " JOIN_VALUE << ".v" << i+1 << ";\n";
  }
  b << "\n";
  b.tosuio ()->output (fd);

  b.tosuio ()->clear ();
  state.need_line_xlate ();
  element_list_t::output (fd);

  b << "    } else {\n"
    ;
  
  _fn->jump_out (b, _id);
  
  b << "      " << jgn
    << ".set_join_cb (wrap (" << CLOSURE_RFCNT
    << ", &" << _fn->reenter_fn () << "));\n"
    << "      ";
  b.mycat (_fn->return_expr ());
  b << ";\n"
    << "  }\n\n";

  b.tosuio ()->output (fd);
  state.need_line_xlate ();
}

void
parse_state_t::output_line_xlate (int fd, int ln)
{
  if (_xlate_line_numbers && _need_line_xlate) {
    strbuf b;
    b << "# " << ln << " \"" << _infile_name << "\"\n";
    b.tosuio ()->output (fd);
    _need_line_xlate = false;
  }
}

//
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
// handle return semantics
//

void
tame_ret_t::output (int fd)
{
  my_strbuf_t b;
  str loc = state.loc (_line_number);

  if (_fn->do_cceoc ()) {
    b << "  END_OF_SCOPE (\"" << loc << "\");\n";
    state.need_line_xlate ();
  }
  
  b << "return ";
  if (_params)
    b << _params;
  b.tosuio ()->output (fd);
  tame_env_t::output (fd);
}

void
tame_unblock_t::output (int fd)
{
  my_strbuf_t b;
  str loc = state.loc (_line_number);
  str n = macro_name ();
  b << "  " << n << " (\"" << loc << "\", ";
  b.mycat (_fn->cceoc_typename ());
  if (_params) {
    b << ", " << _params;
  }
  b << ");\n";
  _fn->do_cceoc_call ();
  do_return_statement (b);
  b.tosuio ()->output (fd);
}

void
tame_fn_resume_t::do_return_statement (my_strbuf_t &b) const
{
  b.mycat (_fn->return_expr ());
}

void
tame_fn_return_t::output (int fd)
{
  my_strbuf_t b;
  if (_fn->do_cceoc ()) {
    str loc = state.loc (_line_number);
    b << "  END_OF_SCOPE(\"" << loc << "\");\n";
  
    b.tosuio ()->output (fd);
  }
  b << "  ";
  b.mycat (_fn->return_expr ());
  b << ";\n";
  b.tosuio ()->output (fd);
  state.need_line_xlate ();
}

str
parse_state_t::loc (u_int l) const
{
  strbuf b;
  b << _infile_name << ":" << l << ": in function " 
    << function_const ().name ();
  return b;
}

//
//-----------------------------------------------------------------------
