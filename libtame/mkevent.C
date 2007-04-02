
#include "tame_event_ag.h"

event<>::ref
_mkevent_rs (ptr<closure_t> c, const char *loc, const refset_t<> &rs, 
	     rendezvous_t<> &rv)
{
  return rv._ti_mkevent (c, loc, value_set_t<> (), rs);
}

event<>::ref
_mkevent (ptr<closure_t> c, const char *loc, rendezvous_t<> &rv)
{
  return _mkevent_rs (c, loc, refset_t<> (), rv); 
}
