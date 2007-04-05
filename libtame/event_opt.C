
#include "tame_recycle.h"
#include "tame_event_opt.h"

static recycle_bin_t<event_v_opt_t> *_evv_recycle_bin;

//-----------------------------------------------------------------------
// optimized events


inline static recycle_bin_t<event_v_opt_t> *
rb () 
{
  if (!_evv_recycle_bin) {
    _evv_recycle_bin = New recycle_bin_t<event_v_opt_t> (0x100000);
  }
  return _evv_recycle_bin;
}

void
event_v_opt_t::finalize ()
{
  clear_action ();
  warn << "recyling...\n";
  rb ()->recycle (this);
}

event_v_opt_ptr_t
event_v_opt_t::alloc (closure_ptr_t c, const char *loc)
{
  event_v_opt_ptr_t ret = rb ()->get ();
  if (ret) {
    ret->reinit (c, loc);
    warn << "reuse....\n";
  } else {
    ret = New refcounted<event_v_opt_t> (c, loc);
    warn << "recreate...\n";
  }
  c->block_inc_count ();
  c->add (ret);
  return ret;
}

//
//-----------------------------------------------------------------------
