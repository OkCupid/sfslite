// -*-c++-*-
/* $Id$ */

#include "tame.h"
#include "parseopt.h"
#include "arpc.h"

bool fix_bug;

static void usage ()
{
  fatal << "usage: " << progname << " [-f]\n";
}

vec<cbv::ptr> holding_area;

tamed static
void leaky_callee (cbv cb)
{
  holding_area.push_back (cb);
  cwait { delaycb (1, 0, mkevent ()); }
  if (fix_bug)
    cb->trigger ();
  else
    (*cb) ();
}


tamed static
void block_with_cb_hog (cbv cb)
{
  warn << "block_with_cb_hog...\n";
  cwait { leaky_callee (mkevent ()); }
  cb->trigger ();
}

tamed static
void nonblock_with_cb_hog (cbv cb)
{
  cvars {
    rendezvous_t<> rv (__FILE__, __LINE__);
  }
  warn << "nonblock_with_cb_hog...\n";
  leaky_callee (mkevent (rv));
  cwait (rv);
  cb->trigger ();
}

tamed static
void nonblock_with_unwaited_cvs (cbv cb)
{
  cvars {
    rendezvous_t<> rv (__FILE__, __LINE__);
  }
  warn << "nonblock_with_unwaited_cvs...\n";
  delaycb (1, 0, mkevent (rv) );
  if (fix_bug) {
    cwait (rv); 
  }
  cb->trigger ();
}

typedef void (* run_me_t) (cbv, ptr<closure_t>);

tamed static 
void main_T ()
{
  cvars { 
    run_me_t *p (NULL); 
  }
  static run_me_t fns[] = { block_with_cb_hog,
			    nonblock_with_cb_hog,
			    nonblock_with_unwaited_cvs,
			    NULL };

  for ( p = fns; *p; p++) {
    cwait { (*p) ( mkevent (), NULL ); }
    cwait { delaycb (2, 0, mkevent () ); }
  }
  warn << "delaying for 2 seconds, then exiting..\n";
  delaycb (2, 0, wrap (exit, 0));
}
 
int main (int argc, char *argv[])
{
  int ch;
  fix_bug = false;
  setprogname (argv[0]);
  while ((ch = getopt (argc, argv, "f")) != -1) {
    switch (ch) {
    case 'f':
      fix_bug = true;
      break;
    default:
      usage ();
    }
  }
  main_T ();
  amain ();
}
