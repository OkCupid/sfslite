
#include "tame_event_ag.h"

event_t<>::ref
_mkevent (implicit_rendezvous_t *c, const char *loc)
{
  return New refcounted<event<> > 
    (c->make_reenter (loc), refset_t<> (), loc);
}

event_t<>::ref
_mkevent_rs (ptr<closure_t> c, const char *loc, const refset_t<> &rs, 
	     rendezvous_t<> rv)
{
  return rv._mkevent (c, loc, value_set_t<> (), rs);
}

event_t<>::ref
_mkevent (ptr<closure_t> c, const char *loc, rendezvous_t<> rv)
{
  return _mkevent_rs (c, loc, refset_t<> (), rv); 
}
