// -*-c++-*-
/* $Id$ */

#include "tame.h"
#include "parseopt.h"
#include "arpc.h"

static void fn1 (cbv cb)
{
  SIGNAL (cb);
  SIGNAL (cb);
}

static void fn2 (cbv cb)
{
  SIGNAL (cb);
  (*cb) ();
}

TAMED static void
main_T ()
{
  BLOCK { fn1 (@()); }
  BLOCK { fn2 (@()); }
  exit (0);
}

int 
main (int argc, char *argv[])
{
  main_T ();
  amain ();
}
