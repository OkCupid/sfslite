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

TAMED static
void leaky_callee (cbv cb)
{
  holding_area.push_back (cb);
  BLOCK { delaycb (1, 0, @()); }
  if (fix_bug)
    cb->signal ();
  else
    (*cb) ();
}


TAMED static
void block_with_cb_hog (cbv cb)
{
  warn << "block_with_cb_hog...\n";
  BLOCK { leaky_callee (@()); }
  cb->signal ();
}

TAMED static
void nonblock_with_cb_hog (cbv cb)
{
  VARS {
    coordgroup_t<> G (__FILE__, __LINE__);
  }
  warn << "nonblock_with_cb_hog...\n";
  leaky_callee (@[G] ());
  WAIT (G);
  cb->signal ();
}

TAMED static
void nonblock_with_unwaited_cvs (cbv cb)
{
  VARS {
    coordgroup_t<> G (__FILE__, __LINE__);
  }
  warn << "nonblock_with_unwaited_cvs...\n";
  delaycb (1, 0, @[G]() );
  if (fix_bug) {
    WAIT (G); 
  }
  cb->signal ();
}

typedef void (* run_me_t) (cbv, ptr<closure_t>);

TAMED static 
void main_T ()
{
  VARS { 
    run_me_t *p (NULL); 
  }
  static run_me_t fns[] = { block_with_cb_hog,
			    nonblock_with_cb_hog,
			    nonblock_with_unwaited_cvs,
			    NULL };

  for ( p = fns; *p; p++) {
    BLOCK { (*p) ( @(), NULL ); }
    BLOCK { delaycb (2, 0, @() ); }
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
