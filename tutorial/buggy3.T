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

tamed static void
main_T ()
{
  cwait { fn1 (mkevent()); }
  cwait { fn2 (mkevent()); }
  exit (0);
}

int 
main (int argc, char *argv[])
{
  main_T ();
  amain ();
}
