// -*-c++-*-
/* $Id$ */

#include "tame.h"
#include "parseopt.h"
#include "arpc.h"

static void fn1 (cbv cb)
{
  cb->signal ();
  cb->signal ();
}

static void fn2 (cbv cb)
{
  cb->signal ();
  (*cb) ();
}

TAMED static void
main_T ()
{
  BLOCK { fn1 (mkevent()); }
  BLOCK { fn2 (mkevent()); }
  exit (0);
}

int 
main (int argc, char *argv[])
{
  main_T ();
  amain ();
}
