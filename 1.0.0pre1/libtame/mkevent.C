
#include "tame_mkevent.h"

void
_mkevent_cb_0 (ptr<reenterer_t> c, refset_t<> rs)
{
  rs.assign ();
  c->maybe_reenter ();
}

callback<void>::ref
_mkevent (implicit_rendezvous_t *c, const char *loc)
{
  return wrap (_mkevent_cb_0, c->make_reenter (loc), refset_t<> ());
}

void
_mkevent_cb_0_0 (ptr<closure_t> hold, ptr<joiner_t<> > j, 
		 refset_t<> rs, value_set_t<> w)
{
  rs.assign ();
  j->join (w);
}

callback<void>::ref
_mkevent (ptr<closure_t> c, const char *loc, rendezvous_t<> rv)
{
  return wrap (_mkevent_cb_0_0, c, rv.make_joiner (loc, c),
               refset_t<> (), value_set_t<> ());
}
