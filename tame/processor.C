
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

static void output_el (outputter_t *o, tame_el_t *e) { e->output (o); }
void element_list_t::output (outputter_t *o) 
{ _lst.traverse (wrap (output_el, o)); }

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
  static rxx double_colon ("::");
  vec<str> pieces;
  int n = split (&pieces, double_colon, in);
  if (n > 1) {
    return pieces.back ();
  } else {
    return in;
  }
}

str 
strip_off_method (const str &in)
{
  static rxx double_colon ("::");
  vec<str> pieces;
  int n = split (&pieces, double_colon, in);
  if (n > 1) {
    strbuf b;
    for (size_t i = 0; i < pieces.size () - 1; i++) {
      if (i != 0) b << "::";
      b << pieces[i];
    }
    return b;
  } else {
    return NULL;
  }
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
tame_passthrough_t::output (outputter_t *o)
{
  if (_strs.size ()) {
    int ln = _strs[0].lineno ();
    output_mode_t old = o->switch_to_mode (OUTPUT_PASSTHROUGH, ln);
    o->output_str (_buf);
    o->switch_to_mode (old);
  }
}

str
tame_nonblock_callback_t::cb_name () const
{
  strbuf b;
  size_t N_w = n_args ();
  u_int N_p = _call_with->size ();
  b << "__nonblock_cb_" << N_w << "_" << N_p;
  return b;
}

void
tame_nonblock_callback_t::output_generic (strbuf &b)
{
  size_t N_w = n_args ();
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
    b << "\t\trefset_t<";
    for (u_int i = 1; i <= N_p; i++) {
      if (i != 1) b << ", ";
      b << "P" << i;
    }
    b << "> rs";
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
  if (N_p) {
    b << "  rs.assign (";
    for (u_int i = 1; i <= N_p; i++) {
      if (i != 1) b << ", ";
      b << "v" << i;
    }
    b << ");\n";
  }
  b << "  j->join (w);\n"
    << "}\n\n";
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
  b << "ptr<closure_t>)";
  if (_isconst)
    b << " const";
  b << ";\n";

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
tame_fn_t::output_closure (outputter_t *o)
{
  my_strbuf_t b;
  output_mode_t om = o->switch_to_mode (OUTPUT_TREADMILL);

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

  b << ") : closure_t (\"" << state.infile_name () << "\", \"" 
    << _name << "\"), "
    ;

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

  b << " {}\n\n";


  if (_class) {
    output_set_method_pointer (b);
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

  output_is_onstack (b);

  b << "};\n\n";

  o->output_str (b);
  o->switch_to_mode (om);
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
    << "  case 0: break;\n"
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
    << "    panic (\"unexpected case.\\n\");\n"
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
  if (_isconst) 
    b << " const";

  return b;
}

void
tame_fn_t::output_static_decl (outputter_t *o)
{
  my_strbuf_t b;
  output_mode_t om = o->switch_to_mode (OUTPUT_TREADMILL);
  b.mycat (signature (true, NULL, true)) << ";\n\n";
  o->output_str (b);
  o->switch_to_mode (om);
}

void
tame_fn_t::output_fn (outputter_t *o)
{
  my_strbuf_t b;
  state.set_fn (this);

  output_mode_t om = o->switch_to_mode (OUTPUT_PASSTHROUGH);
  b << signature (false, TAME_PREFIX)  << "\n"
    << "{";

  o->output_str (b);

  // If no vars section was specified, do it now.
  if (!_vars)
    output_vars (o, _lbrace_lineno);

  element_list_t::output (o);

  o->switch_to_mode (om);
}

void
tame_vars_t::output (outputter_t *o)
{
  _fn->output_vars (o, _lineno);
}

void
tame_fn_t::output_vars (outputter_t *o, int ln)
{
  my_strbuf_t b;

  output_mode_t om = o->switch_to_mode (OUTPUT_TREADMILL, ln);

  b << "  " << _closure.decl () << ";\n"
    << "  "
    ;
  b.mycat (_closure.type ().mk_ptr ());
  b << " " << CLOSURE_RFCNT << ";\n"
    << "  if (!" << closure_generic ().name() << ") {\n"
    ;

  b << "    if (tame_check_leaks ()) start_join_group_collection ();\n"
    << "    " << CLOSURE_RFCNT << " = New refcounted<"
    << _closure.type().type_without_pointer() << "> (";

  if (need_self ()) {
    b << "this";
    if (_args)
      b << ", ";
  }

  if (_args)
    _args->paramlist (b, NAMES, TAME_PREFIX);

  b << ");\n"
    << "    if (tame_check_leaks ()) " 
    << CLOSURE_RFCNT << "->collect_join_groups ();\n"
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
  o->output_str (b);

  // will switch modes as appropriate (internally)
  o->switch_to_mode (om);
}

void 
tame_fn_t::output (outputter_t *o)
{
  if ((_opts & STATIC_DECL) && !_class)
    output_static_decl (o);
  output_generic (o);
  output_closure (o);
  output_fn (o);
}

void
tame_fn_t::output_generic (outputter_t *o)
{
  strbuf b;
  output_mode_t om = o->switch_to_mode (OUTPUT_TREADMILL);
  for (u_int i = 0; i < _nbcbs.size (); i++) {
    _nbcbs[i]->output_generic (b);
  }
  o->output_str (b);
  o->switch_to_mode (om);
}

void
tame_fn_t::jump_out (strbuf &b, int id)
{
  b << "    " << TAME_CLOSURE_NAME << "->set_jumpto (" << id 
    << ");\n"
    << "\n";
}

void 
tame_block_t::output (outputter_t *o)
{
  my_strbuf_t b;
  str tmp;

  output_mode_t om = o->switch_to_mode (OUTPUT_TREADMILL);

  b << "  do {\n"
    << "    " << TAME_CLOSURE_NAME << "->init_block (" 
    << _id << ", " << _lineno << ");\n"
    ;

  _fn->jump_out (b, _id);
  
  o->output_str (b);
  b.tosuio ()->clear ();
  o->switch_to_mode (om);

  // now we are returning to mainly pass-through code, but with some
  // callbacks thrown in (which won't change the line-spacing)

  for (tame_el_t *el = _lst.first; el; el = _lst.next (el)) {
    el->output (o);
  }

  om = o->switch_to_mode (OUTPUT_TREADMILL);
  b << "\n"
    << "    if (!" << TAME_CLOSURE_NAME << "->block_dec_count (" 
    << _lineno << "))\n"
    << "      ";

  b.mycat (_fn->return_expr ());

  b << ";\n"
    << "  } while (0);\n"
    << " " << _fn->label (_id) << ":\n"
    << "    ;\n"
    ;


  o->output_str (b);
  o->switch_to_mode (om);
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
parse_state_t::output (outputter_t *o)
{
  o->start_output ();
  element_list_t::output (o);
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
  for (size_t i = 0; i < n_args (); i++) {
    if (!first)  b << ", ";
    else first = false;
    if (prfx) b << prfx;
    b << arg (i).name ();
    if (sffx) b << sffx;
  }
  return first;
}

void
tame_block_callback_t::output (outputter_t *o)
{
  my_strbuf_t b;
  output_mode_t om = o->switch_to_mode (OUTPUT_PASSTHROUGH);
  b << "wrap ("
    << TAME_CLOSURE_NAME << "->make_wrapper ("
    << _block->id ()  << ", " << _line_number << "), "
    << "&closure_wrapper_t::block_cb" << _call_with->size ();
  if (_call_with->size ()) {
    b << "<";
    _call_with->output_vars (b, true, "TTT(", ")");
    b << ">";
  }
  if (_call_with->size ()) {
    b << ", refset_t<";
    _call_with->output_vars (b, true, "TTT(", ")");
    b << "> (";
    _call_with->output_vars (b, true, "", "");
    b << ")";
  }
  b << ")";
  o->output_str (b);
  o->switch_to_mode (om);
}

void
tame_nonblock_callback_t::output (outputter_t *o)
{
  my_strbuf_t b;
  output_mode_t om = o->switch_to_mode (OUTPUT_PASSTHROUGH);

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
    b << ", refset_t<";
    _call_with->output_vars (b, true, "TTT (", ")");
    b << "> (";
    _call_with->output_vars (b, true, "", "");
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

  o->output_str (b);
  o->switch_to_mode (om);
}

#define JOIN_VALUE "__v"
void
tame_join_t::output (outputter_t *o)
{
  output_mode_t om = o->switch_to_mode (OUTPUT_TREADMILL);

  strbuf tmp;
  tmp << "(" << join_group ().name () << ")";
  str jgn = tmp;

  my_strbuf_t b;
  b << "  ";
  b.mycat (_fn->label (_id)) << ":\n";
  b << "  do {\n"
    << "    typeof (" << jgn << ".to_vs ()) "
    << JOIN_VALUE << ";\n";
  b << "    if (" <<  jgn << ".pending (&" 
    << JOIN_VALUE << ")) {\n";
  
  for (size_t i = 0; i < n_args (); i++) {
    b << "      typeof (" << JOIN_VALUE << ".v" << i+1 << ") &"
      << arg (i).name ()  << " = " JOIN_VALUE << ".v" << i+1 << ";\n";
  }
  b << "\n";
  o->output_str (b);
  b.tosuio ()->clear ();
  o->switch_to_mode (om);

  element_list_t::output (o);

  om = o->switch_to_mode (OUTPUT_TREADMILL);
  b << "    } else {\n"
    ;

  output_blocked (b, jgn);
  
  b << "   }\n"
    << " } while (0);\n";

  o->output_str (b);
  o->switch_to_mode (om);
}

void
tame_join_t::output_blocked (my_strbuf_t &b, const str &jgn)
{
  _fn->jump_out (b, _id);
  
  b << "      " << jgn
    << ".set_join_cb (wrap (" << CLOSURE_RFCNT
    << ", &" << _fn->reenter_fn () << "));\n"
    << "      ";
  b.mycat (_fn->return_expr ());
  b << ";\n";

}

void
tame_wait_t::output (outputter_t *o)
{
  strbuf tmp;
  tmp << "(" << join_group ().name () << ")";
  str jgn = tmp;

  output_mode_t om = o->switch_to_mode (OUTPUT_TREADMILL);
  my_strbuf_t b;
  b.mycat (_fn->label (_id)) << ":\n";
  b << "do {\n"
    << "   if (!" << jgn << ".next_var (";
  for (size_t i = 0; i < n_args (); i++) {
    if (i > 0) b << ", ";
    b << "" << arg (i).name () << "";
  }
  b << ")) {\n";
  output_blocked (b, jgn);
  b << "  }\n"
    << "} while (0);\n";

  o->output_str (b);
  o->switch_to_mode (om);
}

//
//-----------------------------------------------------------------------

//-----------------------------------------------------------------------
// handle return semantics
//

void
tame_ret_t::output (outputter_t *o)
{
  output_mode_t om = o->switch_to_mode (OUTPUT_TREADMILL);
  my_strbuf_t b;

  // always do end of scope checks
  b << "  " << TAME_CLOSURE_NAME << "->end_of_scope_checks (" 
    << _line_number << ");\n";
  o->output_str (b);
  b.tosuio ()->clear ();

  o->switch_to_mode (OUTPUT_PASSTHROUGH, _line_number);
  
  b << "return ";
  if (_params)
    b << _params;

  o->output_str (b);
  tame_env_t::output (o);
  o->switch_to_mode (om);

}

void
tame_unblock_t::output (outputter_t *o)
{
  const str tmp ("__cb_tmp");
  my_strbuf_t b;
  output_mode_t om = o->switch_to_mode (OUTPUT_TREADMILL);
  str loc = state.loc (_line_number);
  b << "  do {\n";
  
  str n = macro_name ();
  b << n << " (\"" << loc << "\", ";
  b.mycat (tmp);
  if (_params) {
    b << ", " << _params;
  }
  b << "); ";
  do_return_statement (b);
  b << "  } while (0);\n";

  o->output_str (b);
  o->switch_to_mode (om);
}

void
tame_resume_t::do_return_statement (my_strbuf_t &b) const
{
  b.mycat (_fn->return_expr ()) << "; ";
}

void
tame_fn_return_t::output (outputter_t *o)
{
  my_strbuf_t b;
  output_mode_t om = o->switch_to_mode (OUTPUT_TREADMILL);

  b << "  " << TAME_CLOSURE_NAME << "->end_of_scope_checks (" 
    << _line_number << ");\n";
  b << "  ";
  b.mycat (_fn->return_expr ());
  b << ";\n";
  o->output_str (b);
  o->switch_to_mode (om);
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
