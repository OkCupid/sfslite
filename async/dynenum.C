
#include "dynenum.h"

int
dynenum_t::lookup (const str &s, bool dowarn) const
{
  int ret;
  const int *i = s ? _tab[s] : NULL;
  if (i) {
    ret = *i;
  } else {
    ret = _def_val;
    if (dowarn) {
      warn_not_found (s);
    }
  }
  return ret;
}

void
dynenum_t::warn_not_found (str s) const
{
  if (!s)
    s = "(null)";

  str n = _enum_name;
  if (!n)
    n = "anonymous";

  warn << "XX dynamic enum (" << n << "): no value for key=" << s << "\n";
}


void
dynenum_t::init (const pair_t pairs[], bool chk)
{
  for (const pair_t *p = pairs; p->n; p++) {
    _tab.insert (p->n, p->v);
  }

  if (chk) {
    // Do a spot check!
    for (const pair_t *p = pairs; p->n; p++) {
      assert ((*this)[p->n] == p->v);   
    }
  }
}
