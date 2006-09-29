
// -*-c++-*-
/* $Id: ex1.T 2236 2006-09-29 00:00:22Z max $ */

#include "tame.h"

tamed static void
sleeper_ev (cbv cb)
{
  warn << "++ sleeper_ev: to sleep!\n";
  cwait { delaycb (3, 0, mkevent ()); }
  warn << "-- sleeper_ev: woke up!\n";
  cb->signal ();
}

static void
sleeper_thr ()
{
  warn << "++ sleeper_thr: to sleep!\n"; 
  sleep (2);
  warn << "-- sleeper_thr: woke up!\n";
}

static void
run_thr ()
{
  warn << "+ run_thr\n";
  cwait {
    cfork (wrap (sleeper_ev, mkevent (), null_closure));
    cfork (wrap (sleeper_thr));
  }
  warn << "- run_thr\n";

}

tamed static void
run_ev (cbv cb)
{
  warn << "+ run_ev\n";
  cwait {
    sleeper_ev (mkevent ());
    cfork (wrap (sleeper_thr));
  }
  warn << "- run_ev\n";
  cb->signal ();
}

tamed static void
run ()
{
  cwait { run_ev (mkevent ()); }
  cwait { cfork (wrap (run_thr)); }
  cwait { run_ev (mkevent ()); }
  exit (0);
}

int main (int argc, char *argv[])
{
  run ();
  amain ();
}
