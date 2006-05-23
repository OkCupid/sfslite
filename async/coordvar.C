// -*-c++-*-
/* $Id$ */

#include "coordvar.h"
#include "async.h"

//-----------------------------------------------------------------------
// Global flags for toggling coordvar behavior
//

bool coordvar_strict_mode;

//
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------
// recycle bin for ref flags, used in both callback.h, and also
// tame.h/.C

static recycle_bin_t<ref_flag_t> *rfrb;
recycle_bin_t<ref_flag_t> * ref_flag_t::get_recycle_bin () { return rfrb; }

void
ref_flag_t::recycle (ref_flag_t *p)
{
  if (get_recycle_bin ()->add (p)) {
    p->set_can_recycle (false);
  } else {
    delete p;
  }
}

ptr<ref_flag_t>
ref_flag_t::alloc (const bool &b)
{
  ptr<ref_flag_t> ret = get_recycle_bin ()->get ();
  if (ret) {
    ret->set_can_recycle (true);
    ret->set (b);
  } else {
    ret = New refcounted<ref_flag_t> (b);
  }
  assert (ret);
  return ret;
}


//
//-----------------------------------------------------------------------

void
coordvar_second_signal (const char *file, const char *line)
{
  warn << "signal after deallaction";
  if (file && line) {
    warnx << " (on coordvar allocated from " << file << ":" << line << ")";
  } else {
    warnx << " (no further information without debugging build)";
  }
  warnx << "\n";

  if (coordvar_strict_mode) 
    panic ("Aborting due to coordvar strict mode\n");
}

int ref_flag_init::count;

void
ref_flag_init::start ()
{
  static bool initialized;
  if (initialized)
	  panic ("ref_flag_init::start called twice");
  initialized = true;
  rfrb = New recycle_bin_t<ref_flag_t> ();
}

void
ref_flag_init::stop () {}
