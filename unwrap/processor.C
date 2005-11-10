
#include "unwrap.h"

str
mangle (const str &in)
{
  const char *i;
  char *o;
  mstr m (in.len ());
  for (c = in.cstr (), o = m.cstr (); c; c++, o++) {
    *o = (*c == ':' || *c == '<' || *c == '>') ? '_' : *c;
  }
  return m;
}

bool
vartab_t::add (var_t v)
{
  if (_tab[v->name ()]) {
    return false;
  }

  _vars.push_back (v);
  _tab.insert (v->name (), _vars.size () - 1);

  return true;
}
