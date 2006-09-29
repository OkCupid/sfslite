
// -*-c++-*-
/* $Id: ex1.T 2236 2006-09-29 00:00:22Z max $ */

#include "tame.h"

tamed static void
sleeper_ev (cbi cb)
{
  warn << "++ sleeper_ev: to sleep!\n";
  cwait { delaycb (3, 0, mkevent ()); }
  warn << "-- sleeper_ev: woke up!\n";
  cb->signal (40);
}

static int
sleeper_thr ()
{
  warn << "++ sleeper_thr: to sleep!\n"; 
  sleep (4);
  warn << "-- sleeper_thr: woke up!\n";
  return 10;
}

static void
run_thr ()
{
  int r1,r2;
  warn << "+ run_thr\n";
  cwait {
    sleeper_ev (mkevent (r1));
    cfork (r2, wrap (sleeper_thr));
  }
  warn << "- run_thr (" << r1 << "," << r2 << ")\n";

}

tamed static void
run_ev (cbv cb)
{
  cvars {
    int r1, r2;
  }
  warn << "+ run_ev\n";
  cwait {
    sleeper_ev (mkevent (r1));
    cfork (r2, wrap (sleeper_thr));
  }
  warn << "- run_ev (" << r1 << "," << r2 << ")\n";
  cb->signal ();
}

tamed static void
run ()
{
  cwait { run_ev (mkevent ()); }
  cwait { cfork (wrap (run_thr)); }
  cwait { run_ev (mkevent ()); }
  cwait { cfork (wrap (run_thr)); }
  exit (0);
}

int main (int argc, char *argv[])
{
  run ();
  amain ();
}
