
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
  sleep (4);
  warn << "-- sleeper_thr: woke up!\n";
}

static void
run_thr ()
{
  rendezvous_t<int> rv;
  int i;
  warn << "+ run_thr\n";

  sleeper_ev (mkevent (rv, 0));
  cfork (rv, 1, wrap (sleeper_thr));

  rv.cwait (i);
  warn << "oo event return: " << i << "\n";
  rv.cwait (i);
  warn << "oo event return: " << i << "\n";

  warn << "- run_thr\n";
}

tamed static void
run_ev (cbv cb)
{
  cvars {
    rendezvous_t<int> rv;
    int i;
  }
  warn << "+ run_ev\n";

  sleeper_ev (mkevent (rv, 0));
  cfork (rv, 1, wrap (sleeper_thr));

  cwait (rv, i);
  warn << "oo event return: " << i << "\n";
  cwait (rv, i);
  warn << "oo event return: " << i << "\n";

  warn << "- run_ev\n";
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
