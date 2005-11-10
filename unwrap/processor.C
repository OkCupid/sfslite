
#include "unwrap.h"

var_t::var_t (const str &t, ptr<declarator_t> d)
  : _type (t, d->pointer ()), _name (d->name ()) {}

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
