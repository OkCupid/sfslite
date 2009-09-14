
#include "tame_event_ag.h"

event<>::ref
_mkevent_rs (ptr<closure_t> c, const char *loc, const char *ctn,
	     const _tame_slot_set<> &rs, 
	     rendezvous_t<> &rv)
{
  event<>::ref ret = rv._ti_mkevent (c, loc, value_set_t<> (), rs);
  ret->set_gdb_info (ctn, c);
  return ret;
}

event<>::ref
_mkevent (ptr<closure_t> c, const char *loc, const char *ctn, 
	  rendezvous_t<> &rv)
{
  event<>::ref ret = _mkevent_rs (c, loc, ctn, _tame_slot_set<> (), rv); 
  return ret;
}
