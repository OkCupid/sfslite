
#include "unwrap.h"

var_t::var_t (const str &t, ptr<declarator_t> d)
  : _type (t, d->pointer ()), _name (d->name ()), _stack_var (true) {}

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
