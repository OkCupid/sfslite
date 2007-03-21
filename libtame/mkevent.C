
#include "tame_event_ag.h"

event_t<>::ref
_mkevent (implicit_rendezvous_t *c, const char *loc)
{
  return New refcounted<event_t<> > 
    (c->make_reenter (loc), refset_t<> (), loc);
}

event_t<>::ref
_mkevent (ptr<closure_t> c, const char *loc, rendezvous_t<> rv)
{
  return New refcounted<event_t<> > 
    (rv.make_joiner (c, loc, value_set_t<> ()), 
     refset_t<> (), 
     loc);
}
