// -*-c++-*-
/* $Id$ */

#include "tame.h"
#include "parseopt.h"
#include "arpc.h"

static void fn1 (cbv cb)
{
  cb->trigger ();
  cb->trigger ();
}

static void fn2 (cbv cb)
{
  cb->trigger ();
  (*cb) ();
}

tamed static void
main_T ()
{
  twait { fn1 (mkevent()); }
  twait { fn2 (mkevent()); }
  exit (0);
}

int 
main (int argc, char *argv[])
{
  main_T ();
  amain ();
}
