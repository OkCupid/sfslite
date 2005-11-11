
#include "unwrap.h"
#include "rxx.h"

var_t::var_t (const str &t, ptr<declarator_t> d)
  : _type (t, d->pointer ()), _name (d->name ()), _stack_var (true) {}

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
unwrap_fn_t::freezer () const
{
  strbuf b;
  b << _name_mangled << "__freezer_t";

  return var_t (b, "*", "__frz");
}

str
unwrap_fn_t::decl_casted_freezer () const
{
  strbuf b;
  b << "  " << _freezer.decl ()  << " =\n"
    << "    reinterpret_cast<" << _freezer.type ().to_str () 
    << "> (" << freezer_generic ().name () << ");";
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
unwrap_callback_t::output (int fd)
{
  if (_outputted)
    return;
  _outputted = true;

}

void
unwrap_fn_t::output_reenter (int fd)
{
  strbuf b;
  b << "static void\n"
    << reenter_fn () << " (" << freezer_generic ().decl () << ")\n"
    << "{\n"
    << decl_casted_freezer () << "\n"
    << "  " << frznm () <<  "->_self->" << _method_name << " (";

  for (u_int i = 0; _args && i < _args->_vars.size (); i++) {
    if (i != 0) b << ", ";
    b << frozen_arg (_args->_vars[i].name ());
  }
  b << ");\n"
    << "}\n\n";

  b.tosuio ()->output (fd);
}

void
unwrap_fn_t::output_freezer (int fd)
{

}

void
unwrap_fn_t::output_callbacks (int fd)
{



}



void 
unwrap_fn_t::output (int fd)
{
  output_freezer (fd);
  output_reenter (fd);
  output_callbacks (fd);
}

void 
unwrap_shotgun_t::output (int fd)
{

}

void
parse_state_t::output (int fd)
{
  for (unwrap_el_t *el = _elements.first; el; el = _elements.next (el)) {
    el->output (fd);
  }
}



//
//-----------------------------------------------------------------------
