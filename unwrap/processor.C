
#include "unwrap.h"
#include "rxx.h"

var_t::var_t (const str &t, ptr<declarator_t> d, vartyp_t a)
  : _type (t, d->pointer ()), _name (d->name ()), _asc (a), 
    _initializer (d->initializer ()) {}

unwrap_callback_t::unwrap_callback_t (ptr<expr_list_t> t, 
				      unwrap_fn_t *f, 
				      unwrap_shotgun_t *g, 
				      ptr<expr_list_t> e) 
  : _call_with (t), _parent_fn (f), _shotgun (g), _wrap_in (e), _cb_ind (0),
    _last_wrap_ind (g->advance_wrap_ind (_call_with)),
    _local_cb_ind (g->add_callback (this))
{}

#define CLOSURE               "__cls"
#define CLOSURE_DATA_INCLASS  "_data"
#define CLOSURE_DATA          CLOSURE "->" CLOSURE_DATA_INCLASS
#define CLOSURE_TMP           "__cls_tmp"
#define CLOSURE_TMP_G         "__cls_tmp_g"
#define CLOSURE_TMP_DATA      CLOSURE_TMP "->" CLOSURE_DATA_INCLASS

str
unwrap_fn_t::closure_tmp_data () const
{
  return need_tmp_closure () ? str (CLOSURE_TMP_DATA) : closure_data_nm ();
}

str
unwrap_fn_t::closure_tmp () const 
{
  return need_tmp_closure () ? str (CLOSURE_TMP) : closure_nm ();
}

str
unwrap_fn_t::closure_tmp_g () const
{
  return need_tmp_closure () ? str (CLOSURE_TMP_G) : 
    closure_generic ().name ();
}

str
type_t::mk_ptr () const
{
  strbuf b;
  b << "ptr<" << _base_type << " >";
  return b;
}

str
type_t::alloc_ptr (const str &n, const str &args) const
{
  my_strbuf_t b;
  b.mycat (mk_ptr ()) << " " << n << " = New refcounted<"
		      << _base_type << " > (" << args << ")";
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
parse_state_t::new_crcc_star (unwrap_crcc_star_t *c)
{
  _crcc_star = c;
}

void
parse_state_t::new_shotgun (unwrap_shotgun_t *g)
{
  _shotgun = g;
  _elements.insert_tail (g);
}

void
unwrap_fn_t::add_shotgun (unwrap_shotgun_t *g)
{
  _shotguns.push_back (g); 
  g->set_id (_shotguns.size ());
}

int
unwrap_crcc_star_t::advance_wrap_ind (ptr<expr_list_t> l)
{
  int r = _nxt_var_ind;
  if (l) 
    _nxt_var_ind += l->size ();
  return r;
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

var_t
unwrap_fn_t::mk_closure_data () const
{
  strbuf b;
  b << _name_mangled << "__closure_data_t";

  return var_t (b, "*", "_data");
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

bool
unwrap_crcc_star_t::check_backref (int i)
{
  if (i == 0) 
    yyerror ("Back references to wrapped-in args are 1-indexed "
	     "(not 0-indexed)");

  // i can be equal to _nxt_var_ind, since we are 1-indexed
  if (i > _nxt_var_ind)
    yyerror ("Back reference to wrapped-in arg is out of range");
  return true;
}


//
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// Output Routines


str
unwrap_crcc_star_t::make_resume_ref (bool inclass)
{
  strbuf b;
  if (!inclass)
    b << CLOSURE_DATA;
  else
    b << CLOSURE_DATA_INCLASS;
  b << "->_resume"  << _id;
  return b;
}

str
unwrap_crcc_star_t::make_backref (int i, bool inclass)
{
  strbuf b;
  str p = make_resume_ref (inclass);
  b << p << "._a" << i;
  return b;
}

str
unwrap_crcc_star_t::make_cb_ind_ref (bool inclass)
{
  strbuf b;
  str p = make_resume_ref (inclass);
  b << p << "._cb_ind";
  return b;
}

void 
unwrap_passthrough_t::output (int fd)
{
  _buf.tosuio ()->output (fd);
}

bool
unwrap_shotgun_t::output_trig (strbuf &b)
{
  b << unwrap_fn_t::trig ().decl () ;
  return true;
}

bool
unwrap_crcc_star_t::output_trig (strbuf &b)
{
  return false;
}

  
void 
unwrap_callback_t::output_in_class (strbuf &b, int n)
{
  bool first = true;
  _cb_ind = n;

  b << "  void cb" << n  << " (";

  // Regular unwrap_shotgun_t's need triggers wrapped in, whereas
  // CRCC*'s don't.
  if (_shotgun->output_trig (b))
    first = false;

  if (_wrap_in) {
    for (u_int i = 0; i < _wrap_in->size (); i++) {

      if (first) first = false;
      else b << ", ";
      
      b << (*_wrap_in)[i].decl ("_a", i+1);
    }
  }

  if (_call_with) {
    for (u_int i = 0; i < _call_with->size (); i++) {

      if (first) first = false;
      else b << ", ";

      b << (*_call_with)[i].decl ("_b", i+1);
    }
  }
  b << ")\n"
    << "  {\n";

  if (_call_with) {
    for (u_int i = 0; i < _call_with->size (); i++) {
      const var_t &v = (*_call_with)[i];
      b << "    _data->";
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

  _shotgun->output_continuation (this, b);

  b  << "  }\n\n";
}

void
unwrap_crcc_star_t::output_continuation (unwrap_callback_t *cb, strbuf &b)
{
  int base = cb->last_wrap_ind ();
  
  //
  // output
  // 
  // _data->_resume4.a9 = _a3;
  //
  for (u_int i = 0; cb->wrap_in () && i< cb->wrap_in ()->size (); i++) {
    int ind = i + base +1;
    if (_backrefs[ind]) {
      str r = make_backref (ind, true);
      b << "    ";
      b.cat (r.cstr (), true);
      b << " = _a" << (i+1) << ";\n";
    }
  }

  // output
  //
  //  _resume4._cb_ind = 10;
  //
  b << "    ";
  str r = make_cb_ind_ref (true);
  b.cat (r.cstr (), true);
  b << " = " << cb->local_cb_ind () << ";\n";

  // delaycb (0,0) to avoid nasty race conditions with setting counters
  // and immediate return
  b << "    delaycb (0, 0, wrap (mkref (this), &"
    << _fn->closure ().type ().base_type ()
    << "::reenter));\n";
}


void
unwrap_fn_t::output_reenter (strbuf &b)
{
  b << "  void reenter (";
  str tmp = closure_generic ().decl ();
  b.cat (tmp.cstr (), true);
  b << ") {\n"
    << "    ";

  if (_class)
    b <<  "_self->";

  b << _method_name << " (";

  for (u_int i = 0; _args && i < _args->_vars.size (); i++) {
    b << "_args." << _args->_vars[i].name ();
    b << ", ";
  }
  tmp = closure_generic ().name ();
  b.cat (tmp.cstr (), true);
  b << ");\n"
    << "  }\n\n";
}

void
unwrap_crcc_star_t::output_resume_struct_member (strbuf &b)
{
  if (_backrefs.size ()) {
    b << "  resume_" << _id << "_t _resume" << _id << ";\n";
  }
}

void
unwrap_crcc_star_t::output_resume_struct (strbuf &b)
{
  int j = 1;
  bool first = false;
  b << "  struct resume_" << _id << "_t {\n";
  for (u_int i = 0; i < _callbacks.size (); i++) {
    unwrap_callback_t *cb = _callbacks[i];
    for (u_int k = 0; cb->wrap_in () && k < cb->wrap_in ()->size (); k++) {
      if (_backrefs[j]) {
	var_t v = (*cb->wrap_in ())[k];

	str t = v.type ().to_str ();
	b << "   ";
	b.cat (t.cstr (), true);
	b << " _a" << j << ";\n";
      }
      j++;
    }
  }
  b << "    int _cb_ind;\n"
    << "  };\n\n";
}

void
unwrap_fn_t::output_closure (int fd)
{
  strbuf b;

  b << "class " << _closure.type ().base_type () 
    << " : public closure_t "
    << "{\n"
    << "public:\n"
    << "  " << _closure.type ().base_type () 
    << " (ptr<" << _closure_data.type ().base_type () 
    << "> d) : _data (d) {}\n\n";

  int i = 1;
  for (unwrap_callback_t *cb = _cbs.first ; cb; cb = _cbs.next (cb)) {
    cb->output_in_class (b, i++);
  }
  b << "\n"
    << "  void reenter () { " << CLOSURE_DATA_INCLASS 
    << "->reenter (mkref (this)); }\n"
    << "  ptr<" << _closure_data.type ().base_type () << "> _data;\n"
    << "};\n\n";

  b.tosuio ()->output (fd);
}

void
unwrap_fn_t::output_closure_data (int fd)
{
  strbuf b;
  b << "class " << _closure_data.type ().base_type ()  
    << " {\n"
    << "public:\n"
    << "  " << _closure_data.type ().base_type () << " ("
    ;
  _args->paramlist (b);

  b << ") : _stack ("
    ;
  _args->paramlist (b, false);

  b << "), _args ("
    ;
  _args->paramlist (b, false);
  b << ") {}\n";

  output_reenter (b);

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

  for (u_int j = 0; j < _shotguns.size (); j++) {
    _shotguns[j]->output_resume_struct (b);
  }


  b << "  stack_t _stack;\n"
    << "  class_tmp_t _class_tmp;\n"
    << "  args_t _args;\n" ;

  if (_class) {
    b << "  " << _class << " *_self;\n";
  }

  for (u_int j = 0; j < _shotguns.size (); j++) {
    _shotguns[j]->output_resume_struct_member (b);
  }

  b << "};\n\n";

  b.tosuio ()->output (fd);
}

str
unwrap_fn_t::closure_data_nm () const
{
  strbuf b;
  str s;
  s = closure_nm ();
  b.cat (s.cstr (), true);
  b << "->";
  s = _closure_data.name ();
  b.cat (s.cstr (), true);
  return b;
}

void
unwrap_fn_t::output_stack_vars (strbuf &b)
{
  for (u_int i = 0; i < _stack_vars.size (); i++) {
    const var_t &v = _stack_vars._vars[i];
    b << "  " << v.ref_decl () << " = " 
      << closure_data_nm () << "->_stack." << v.name () << ";\n" ;
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
  }
  b << "  default:\n"
    << "    break;\n"
    << "  }\n";
}

str
unwrap_fn_t::signature (bool d) const
{
  strbuf b;
  b << _ret_type.to_str () << "\n"
    << _name << "(";
  if (_args) {
    _args->paramlist (b);
    b << ", ";
  }
  b << closure_generic ().decl ();
  if (d)
    b << " = NULL";
  b << ")";

  return b;
}

void
unwrap_fn_t::output_static_decl (int fd)
{
  strbuf b;
  b << "static " << signature (true) << ";\n\n";
  
  b.tosuio ()->output (fd);
}

void
unwrap_fn_t::output_fn_header (int fd)
{
  my_strbuf_t b;

  b << signature (false)  << "\n"
    << "{\n"
    << "  " << _closure.decl () << ";\n"
    << "  if (!" << closure_generic ().name() << ") {\n"
    ;

  strbuf dat;
  dat << "New refcounted<" << _closure_data.type().base_type () << "> (";
  _args->paramlist (dat, false);
  dat << ")";

  b << "  ";
  b.mycat (_closure.type ().alloc_ptr (CLOSURE_TMP,  str (dat)));
  b << ";\n"
    << "    " << closure_generic (). name () << " = "
    << CLOSURE_TMP << ";\n"
    << "    " << closure_nm () << " = " << CLOSURE_TMP << ";\n"
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
  if (_opts & STATIC_DECL)
    output_static_decl (fd);
  output_closure_data (fd);
  output_closure (fd);
  output_fn_header (fd);
}

void 
unwrap_shotgun_t::output (int fd)
{
  my_strbuf_t b;
  str tmp;

  b << "  {\n";
  const vartab_t *args = _fn->args ();
  for (u_int i = 0; i < args->size (); i++) {
    b << "    " <<  CLOSURE_DATA << "->_args." << args->_vars[i].name ()
      << " = " << args->_vars[i].name () << ";\n";
  }

  if (_fn->need_tmp_closure ()) {
    // copy the closure to a temporary place
    b << "    ";
    b.mycat (_fn->closure ().type ().alloc_ptr (CLOSURE_TMP, CLOSURE_DATA)) 
      << ";\n";
    
    // also make a temporary generic version of it
    b << "    ";
    b.mycat (_fn->closure_generic ().type ().to_str ())
      << " " <<  CLOSURE_TMP_G <<  " = " CLOSURE_TMP << ";\n";
  }
      
  if (_fn->classname ()) 
    b << "    " << CLOSURE_TMP_DATA << "->_self = this;\n";

  b << "    " << CLOSURE_TMP << "->set_jumpto (" << _id 
    << ");\n"
    << "\n";

  if (use_trig ()) {
    b << "    " << _fn->trig ().decl () 
      << " = trig_t::alloc (wrap (";
    b.mycat ( _fn->closure_tmp_g ())
      << ", &closure_t::reenter));\n";
  }
  
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
      << CLOSURE_DATA << "->_class_tmp." << v.name () << ";\n";
  }

  b.tosuio ()->output (fd);
}

void
unwrap_crcc_star_t::output (int fd)
{
  unwrap_shotgun_t::output (fd);
  strbuf b (_resume);
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
  b << "wrap (" << _parent_fn->closure_tmp () << ", "
    << "&" << _parent_fn->closure ().type ().base_type () 
    << "::cb" << _cb_ind;

  if (_shotgun->use_trig ())
    b << ", " << _parent_fn->trig (). name ();

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
